/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
 
#include "platform/learner/learner.h"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <mutex>

#include <glog/logging.h>
#include <nlohmann/json.hpp>

#include "chain/storage/leveldb.h"
#include "platform/common/network/socket.h"
#include "platform/common/network/tcp_socket.h"
#include "platform/proto/resdb.pb.h"
#include "proto/kv/kv.pb.h"
#include "platform/statistic/stats.h"
#include "common/crypto/hash.h"

namespace {

std::string DefaultConfigPath() { return "platform/learner/learner.config"; }

LearnerConfig ParseLearnerConfig(const std::string& config_path) {
    std::ifstream config_stream(config_path);
    if (!config_stream.is_open()) {
        throw std::runtime_error("Unable to open learner config: " + config_path);
    }

    nlohmann::json config_json =
        nlohmann::json::parse(config_stream, nullptr, true, true);
    LearnerConfig config;
    config.ip = config_json.value("ip", config.ip);
    config.port = config_json.value("port", 0);
    config.block_size = config_json.value("block_size", 0);
    config.db_path = config_json.value("db_path", "");
    if (config.db_path.empty() && config.port > 0) {
        config.db_path = std::to_string(config.port) + "_db/";
    }

    if (config_json.contains("replicas") && config_json["replicas"].is_array()) {
        for (const auto& replica_json : config_json["replicas"]) {
            resdb::ReplicaInfo replica_info;
            replica_info.set_id(replica_json.value("id", 0));
            replica_info.set_ip(replica_json.value("ip", "127.0.0.1"));
            replica_info.set_port(replica_json.value("port", 0));
            if (replica_info.port() != 0) {
                config.replicas.push_back(replica_info);
            }
        }
        config.total_replicas = config.replicas.size();
        config.min_needed = (config.total_replicas - 1) / 3 + 2;
    }

    if (config.port <= 0) {
        throw std::runtime_error("Learner config must include a valid port");
    }
    if (config.block_size <= 0) {
        throw std::runtime_error("Learner config must include a positive block_size");
    }
    return config;
}

}  // namespace

Learner::Learner(const std::string& config_path)
    : config_(LoadConfig(config_path)) {
    InitializeStorage();
    resdb::Stats::GetGlobalStats()->Stop();
}

LearnerConfig Learner::LoadConfig(const std::string& config_path) {
    const std::string path = config_path.empty() ? DefaultConfigPath() : config_path;
    return ParseLearnerConfig(path);
}

void Learner::Run() {
    resdb::TcpSocket server;
        if (server.Listen(config_.ip, config_.port) != 0) {
            throw std::runtime_error("Learner failed to bind to " + config_.ip + ":" +
                                    std::to_string(config_.port));
    }

    is_running_.store(true);
    metrics_thread_ = std::thread(&Learner::MetricsLoop, this);

    while (is_running_.load()) {
        auto client = server.Accept();
        if (!client) {
            continue;
        }
        std::thread(&Learner::HandleClient, this, std::move(client)).detach();
    }
}

void Learner::HandleClient(std::unique_ptr<resdb::Socket> socket) {
    // std::cout << "[Learner] replica connected" << std::endl;
    while (true) {
        void* buffer = nullptr;
        size_t len = 0;
        int ret = socket->Recv(&buffer, &len);
        if (ret <= 0) {
            if (buffer != nullptr) {
                free(buffer);
            }
            break;
        }

        std::string payload(static_cast<char*>(buffer), len);
        free(buffer);
        if (!ProcessBroadcast(socket.get(), payload)) {
            break;
        }
    }
    // std::cout << "[Learner] replica disconnected" << std::endl;
}

bool Learner::ProcessBroadcast(resdb::Socket* socket,
                               const std::string& payload) {    

    resdb::ResDBMessage envelope;
    if (!envelope.ParseFromString(payload)) {

        resdb::KVRequest kv_request;
        if (kv_request.ParseFromString(payload)) {
            if (HandleReadOnlyRequest(socket, kv_request)) {
                LOG(INFO) << "served read-only key=" << kv_request.key();
                return false;
            }
            ++total_messages_;
            total_bytes_.fetch_add(payload.size());
            last_type_.store(kv_request.cmd());
            last_seq_.store(0);
            last_sender_.store(-1);
            last_payload_bytes_.store(payload.size());
            return false;
        }
        ++total_messages_;
        total_bytes_.fetch_add(payload.size());
        last_type_.store(-1);
        last_seq_.store(0);
        last_sender_.store(-1);
        last_payload_bytes_.store(payload.size());
        return true;
    }

    resdb::LearnerUpdate request;
    if (request.ParseFromString(envelope.data())) {

        if (request.sender_id() == 0) return true;

        HandleLearnerUpdate(request);

        ++total_messages_;
        total_bytes_.fetch_add(payload.size());
        
        

        return true;
    }

    ++total_messages_;
    total_bytes_.fetch_add(envelope.data().size());
    last_type_.store(-1);
    last_seq_.store(0);
    last_sender_.store(-1);
    last_payload_bytes_.store(envelope.data().size());
    return true;
}

void Learner::HandleLearnerUpdate(resdb::LearnerUpdate learnerUpdate) {

    std::lock_guard<std::mutex> lock(m);

    std::string block_hash = learnerUpdate.block_hash();
    int c_seq = learnerUpdate.seq();
    int sender_id = learnerUpdate.sender_id();
    int excess_bytes = learnerUpdate.excess_bytes();

    int blockIndex = c_seq / config_.block_size - 1;

    // fill sequence status with "havent started" if variable doesnt exist yet
    while (sequence_status.size() < blockIndex + 1) {
        sequence_status.push_back(0);
    }
    
    switch (sequence_status[blockIndex]) {
        case 0: {
            hashCounts.push_back(std::tuple(blockIndex, block_hash, 1));
            learnerUpdates.push_back(learnerUpdate);
            sequence_status[blockIndex] = 1;

            if (1 >= config_.min_needed) {
                sequence_status[blockIndex] = 2;
            }

            break;
        }
        case 1: {
        }
        case 2: {
            std::tuple<int, std::string, int> *valid_hc = nullptr;

            if (sequence_status[blockIndex] == 1) {
                learnerUpdates.push_back(learnerUpdate);

                std::tuple<int, std::string, int> *curr_hc = nullptr;
                for (std::tuple<int, std::string, int>& hc : hashCounts) {
                    if (std::get<0>(hc) != blockIndex) continue;

                    if (std::get<1>(hc) == block_hash) {
                        std::get<2>(hc) = std::get<2>(hc) + 1;
                        curr_hc = &hc;
                        break;
                    }
                }

                if (curr_hc == nullptr) {
                    hashCounts.push_back(std::tuple<int, std::string, int>(blockIndex, block_hash, 1));
                    curr_hc = &hashCounts.back();
                }

                if (std::get<2>(*curr_hc) >= config_.min_needed) {
                    sequence_status[blockIndex] = 2;
                    valid_hc = curr_hc;
                } else {
                    break;
                }
            }

            if (valid_hc == nullptr) { // will run if input case: 2
                for (std::tuple<int, std::string, int>& hc : hashCounts) {
                    if (std::get<0>(hc) != blockIndex) continue;

                    if (std::get<2>(hc) >= config_.min_needed && std::get<1>(hc) == block_hash) { // only set if hash is valid and hash matches received hash
                        learnerUpdates.push_back(learnerUpdate);
                        valid_hc = &hc;
                        break;
                    }
                }
            }
            
            if (std::get<1>(*valid_hc) == block_hash) {

                std::vector<resdb::LearnerUpdate> lus;
                for (resdb::LearnerUpdate lu : learnerUpdates) {
                    if (lu.block_hash() == block_hash) {
                        lus.push_back(lu);
                    }
                }

                bool first = true;
                std::vector<bool> choice(lus.size(), false);
                for (int i = 0; i < config_.min_needed; i++) {
                    choice[i] = true;
                }

                while (GetNextCombination(choice, first)) {
                    first = false;
                    if (!choice[choice.size()-1]) continue; // include the current received message

                    std::vector<int> inds;
                    std::vector<int> rep_ids;
                    for (int i = 0; i < choice.size(); i++) {
                        if (choice[i]) {
                            inds.push_back(i);
                            rep_ids.push_back(lus[i].sender_id());
                        }
                    }

                    std::vector<std::vector<uint16_t>> A = gen_A(rep_ids);

                    std::vector<std::vector<uint16_t>> Ainv = invertMatrix(A, 257);

                    std::string raw_bytes;
                    for (auto i_iter = 0; i_iter < learnerUpdate.data().size(); i_iter++) { // iterate through each block of m bytes (assumption, lengths are equal)
                        
                        for (auto b_iter = 0; b_iter < config_.min_needed; b_iter++) { // iterate through each byte

                            uint32_t mid = 0;
                            for (int m_iter = 0; m_iter < config_.min_needed; m_iter++) { // do dot product

                                mid = (mid + Ainv[b_iter][m_iter] * lus[inds[m_iter]].data()[i_iter]) % 257;

                            }

                            raw_bytes.push_back(static_cast<char>(mid));

                        }

                    }

                    for (int i = 0; i < excess_bytes; i++) {
                        raw_bytes.pop_back();
                    }

                    // std::ofstream out("learner_resTEST.txt");
                    // for (int i = 0; i < raw_bytes.size(); i++) {
                    //     for (int j=1; j < 256; j*=2) {
                    //         out << (raw_bytes[i] & j) << " ";
                    //     }
                    // }

                    std::string final_hash = resdb::utils::CalculateSHA256Hash(raw_bytes);

                    if (final_hash == block_hash) { // we have reconstructed the batch

                        LOG(INFO) << "RECONSTRUCTED " << c_seq << " FROM " << rep_ids[0] << " " << rep_ids[1] << " " << rep_ids[2];

                        resdb::RequestBatch batch;
                        if (batch.ParseFromArray(raw_bytes.data(), static_cast<int>(raw_bytes.size()))) {
                            std::vector<resdb::Request> out;
                            for (int i = 0; i < batch.requests_size(); ++i) {
                                out.push_back(batch.requests(i)); // copies each Request
                                LOG(INFO) << out[i].seq();
                            }
                            
                        }

                        sequence_status[blockIndex] = 3;
                        return;
                    }                  

                }

            }
            break;
        }
    
        }

}

uint32_t Learner::modpow(uint32_t a, uint32_t e, uint32_t p) {
    uint32_t r = 1;
    while (e > 0) {
        if (e & 1) r = (r * a) % p;
        a = (a * a) % p;
        e >>= 1;
    }
    return r;
}

uint32_t Learner::modinv(uint32_t x, uint32_t p) {
    return modpow(x, p - 2, p);
}

std::vector<std::vector<uint16_t>> Learner::invertMatrix(std::vector<std::vector<uint16_t>> A, int p) {
    int n = A.size();

    // Form augmented matrix [A | I]
    std::vector<std::vector<int32_t>> aug(n, std::vector<int32_t>(2*n));
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            aug[i][j] = A[i][j] % p;

            aug[i][n+j] = 0;
            if (i == j) aug[i][n+j] = 1;
        }
            
    }

    // Gauss-Jordan
    for (int col = 0; col < n; col++) {

        // Find pivot row
        int pivot = col;
        while (pivot < n && aug[pivot][col] == 0) pivot++;
        if (pivot == n) throw std::runtime_error("Matrix is not invertible mod p");

        swap(aug[col], aug[pivot]);

        // Normalize pivot row
        int32_t inv = modinv(aug[col][col], p);
        for (int j = 0; j < 2*n; j++)
            aug[col][j] = (aug[col][j] * inv) % p;

        // Eliminate other rows
        for (int i = 0; i < n; i++) {
            if (i == col) continue;
            int32_t factor = aug[i][col];
            for (int j = 0; j < 2*n; j++) {
                int32_t temp = (aug[i][j] - factor * aug[col][j]) % p;
                if (temp < 0) temp += p;
                aug[i][j] = temp;
            }
        }

    }

    // Extract inverse matrix
    std::vector<std::vector<uint16_t>> invA(n, std::vector<uint16_t>(n));
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            invA[i][j] = aug[i][n+j];

    return invA;
}

std::vector<std::vector<uint16_t>> Learner::gen_A(std::vector<int> inds) { 
    
    int n = inds.size();
    int m = config_.min_needed;
    int p = 257;

    std::vector<std::vector<uint16_t>> A(n,std::vector<uint16_t>(m));

    for (int i = 0; i < n; i++) {

        for (int j = 0; j < m; j++) {

            int exp = j;
            int base = inds[i];
            int mod = p;
            int result = 1;

            while (exp > 0) {
                if (exp & 1) {
                    result = (result * base) % mod;
                }

                base = (base * base) % mod;
                exp >>= 1;
            }

            A[i][j] = result;

        }

    }

    return A;

}

bool Learner::GetNextCombination(std::vector<bool> &choice, bool first) {
    if (first) return true;

    int stage = 0;
    int counter = 0;
    for (int i = choice.size() - 1; i >= 0; i--) {
        if (stage == 0) {
            if (choice[i] == 1) {
                counter++;
                choice[i] = 0;
            } else {
                stage = 1;
            }
        } else if (stage == 1) {
            if (choice[i] == 1) {
                counter++;
                choice[i] = 0;
                stage = 2;

                while (counter > 0) {
                    choice[i+counter] = 1;
                    counter--;
                }
            }
        }
    }

    return stage == 2;
}

void Learner::MetricsLoop() const {
    uint64_t last_messages = 0;
    uint64_t last_bytes = 0;
    while (is_running_.load()) {
        uint64_t current_messages = total_messages_.load();
        uint64_t current_bytes = total_bytes_.load();
        if (current_messages != last_messages || current_bytes != last_bytes) {
            PrintMetrics();
            last_messages = current_messages;
            last_bytes = current_bytes;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

bool Learner::HandleReadOnlyRequest(resdb::Socket* socket,
                                    const resdb::KVRequest& request) const {
    if (request.cmd() != resdb::KVRequest::GET_READ_ONLY || storage_ == nullptr) {
        return false;
    }

    if (known_keys_.find(request.key()) == known_keys_.end()) {
        return false;
    }

    std::string value = storage_->GetValue(request.key());
    if (value.empty()) {
        return false;
    }

    resdb::KVResponse response;
    response.set_key(request.key());
    response.set_value(value);

    std::string resp_str;
    if (!response.SerializeToString(&resp_str)) {
        LOG(ERROR) << "[Learner] failed to serialize KVResponse";
        return false;
    }

    if (socket->Send(resp_str) < 0) {
        LOG(ERROR) << "[Learner] failed to send read-only response";
        return false;
    }

    return true;
}

void Learner::PrintMetrics() const {
    LOG(INFO) << "msgs=" << total_messages_.load()
              << " bytes=" << total_bytes_.load()
              << " sets=" << total_sets_.load()
              << " reads=" << total_reads_.load()
              << " deletes=" << total_deletes_.load()
              << " last_type=" << last_type_.load()
              << " last_seq=" << last_seq_.load()
              << " sender=" << last_sender_.load()
              << " payload=" << last_payload_bytes_.load() << "B";
}

void Learner::InitializeStorage() {
    if (config_.db_path.empty()) {
        config_.db_path = std::to_string(config_.port) + "_db/";
    }
    storage_ = resdb::storage::NewResLevelDB(config_.db_path);
    if (!storage_) {
        throw std::runtime_error("Learner failed to initialize storage at " +
                                 config_.db_path);
    }
    known_keys_.clear();
    storage_->SetValue("test", "from learner db");
    storage_->Flush();
    known_keys_.insert("test");
    VLOG(1) << "[Learner] Initialized local DB at " << config_.db_path
            << " with test key";
}

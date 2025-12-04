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

#include <glog/logging.h>
#include <nlohmann/json.hpp>

#include "chain/storage/leveldb.h"
#include "platform/common/network/socket.h"
#include "platform/common/network/tcp_socket.h"
#include "platform/proto/resdb.pb.h"
#include "proto/kv/kv.pb.h"
#include "platform/statistic/stats.h"

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

void Learner::HandleClient(std::unique_ptr<resdb::Socket> socket) const {
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
                               const std::string& payload) const {    

    resdb::ResDBMessage envelope;
    if (!envelope.ParseFromString(payload)) {

        LOG(INFO) << "HERE 1";
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

    LOG(INFO) << "HERE 2";
    resdb::LearnerUpdate request;
    if (request.ParseFromString(envelope.data())) {

        if (request.sender_id() == 0) return true;

        LOG(INFO) << "FFFFFFFFFFFFFFFFFFFFFFFFF";
        // LOG(INFO) << "DATA: " << request.data();
        LOG(INFO) << "HASH: " << request.block_hash();
        LOG(INFO) << "SEQ: " << request.seq();
        LOG(INFO) << "S_ID: " << request.sender_id();

        HandleLearnerUpdate(request);

        ++total_messages_;
        // total_bytes_.fetch_add(payload.size());
        // last_type_.store(request.type());
        // last_seq_.store(request.seq());
        // last_sender_.store(request.sender_id());
        // last_payload_bytes_.store(request.data().size());
        // resdb::BatchUserResponse batch_response;
        // if (batch_response.ParseFromString(request.data())) {
        //     total_sets_.fetch_add(batch_response.set_count());
        //     total_reads_.fetch_add(batch_response.read_count());
        //     total_deletes_.fetch_add(batch_response.delete_count());
        // }
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

    std::string block_hash = request.block_hash();
    int seq = request.seq();
    int sender_id = request.sender_id();

    int blockIndex = seq / config_.block_size() - 1;

    // fill sequence status with "havent started" if variable doesnt exist yet
    while (sequence_status.size() < blockIndex + 1) {
        sequence_status.push_back(0);
    }
    
    switch (sequence_status[blockIndex]) {
        case 0:
            hashCounts.push_back(std::tuple(blockIndex, block_hash, 1));
            learnerUpdates.push_back(learnerUpdate);
            sequence_status[blockIndex] = 1;

            if (1 >= TEMP_VAR_MIN_NEEDED) {
                sequence_status[blockIndex] = 2;
            }

            break;
        case 1:
            learnerUpdates.push_back(learnerUpdate);

            std::tuple<int, std::string, int> *curr_hc = nullptr;
            for (std::tuple<int, std::string, int>& hc : hashCounts) {
                if (std::get<0>(hc) != blockIndex) continue;

                if (std::get<1>(hc) == block_hash) {
                    curr_hc = &hc;
                    break;
                }
            }

            if (curr_hc == nullptr) {
                hashCounts.push_back(std::tuple<int, std::string, int>(blockIndex, block_hash, 1));
                curr_hc = &hashCounts.back();
            }

            if (std::get<2>(curr_hc) >= TEMP_VAR_MIN_NEEDED) {
                sequence_status[blockIndex] = 2;
            }
            break;

        case 2:
            std::tuple<int, std::string, int> *valid_hc = nullptr;
            for (std::tuple<int, std::string, int>& hc : hashCounts) {
                if (std::get<0>(hc) != blockIndex) continue;

                if (std::get<2>(hc) >= TEMP_VAR_MIN_NEEDED) {
                    valid_hc = &hc;
                    break;
                }
            }

            if (std::get<1>(valid_hc) == block_hash) {

                std::vector<resdb::LearnerUpdate> lus;
                for (resdb::LearnerUpdate lu : learnerUpdates) {
                    if (lu.block_hash == block_hash) {
                        lus.push_back(lu);
                    }
                }

                vector<bool> choice(lus.size(), false);
                for (int i = 0; i < TEMP_VAR_MIN_NEEDED-1; i++) {
                    choice[i] = true;
                }

                while (GetNextCombination(choice)) {

                    

                }

            }

            break;


    }

}

bool Learner::GetNextCombination(vector<bool> &choice) {
    int stage = 0;
    int counter = 0;
    for (int i = choice.size() - 1; i >= 0; i--) {
        if (stage == 0) {
            if (choice[i] == 1) {
                stage = 1;
                counter = 1;
                choice[i] = 0;
            }
        } else if (stage == 1) {
            if (choice[i] == 1) {
                counter++;
                choice[i] = 0;
            } else {
                stage = 2;
            }
        } else if (stage == 2) {
            if (choice[i] = 1) {
                choice[i] = 0;
                counter++;

                while (counter > 0) {
                    choice[i+counter] = 1;
                    counter--;
                }

                stage = 3;
            }
        }
    }

    return stage == 3;
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

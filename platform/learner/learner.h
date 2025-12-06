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
 
#pragma once
#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>
#include <tuple>

#include "chain/storage/storage.h"
#include "proto/kv/kv.pb.h"
#include "platform/proto/resdb.pb.h"
#include "platform/proto/replica_info.pb.h"
#include "common/crypto/hash.h"

namespace resdb {
class Socket;
class Request;
}

struct LearnerConfig {
    std::string ip = "127.0.0.1";
    int port = 0;
    int block_size = 0;
    std::string db_path;
    std::vector<resdb::ReplicaInfo> replicas;
    int total_replicas = 0;
    int min_needed = 0;
};

class Learner {
    public:
        explicit Learner(const std::string& config_path);
        void Run();

private:
    static LearnerConfig LoadConfig(const std::string& config_path);
    void HandleClient(std::unique_ptr<resdb::Socket> socket);
    // Returns true if the connection should remain open, false if it should be closed.
    bool ProcessBroadcast(resdb::Socket* socket,
                          const std::string& payload);
    bool HandleReadOnlyRequest(resdb::Socket* socket,
                               const resdb::KVRequest& request) const;
    void MetricsLoop() const;
    void PrintMetrics() const;
    void InitializeStorage();

    void HandleLearnerUpdate(resdb::LearnerUpdate learnerUpdate);
    uint32_t modpow(uint32_t a, uint32_t e, uint32_t p);
    uint32_t modinv(uint32_t x, uint32_t p);
    std::vector<std::vector<uint16_t>> invertMatrix(std::vector<std::vector<uint16_t>> A, int p);
    std::vector<std::vector<uint16_t>> gen_A(std::vector<int> inds);
    bool GetNextCombination(std::vector<bool> &choice, bool first);

private:
    LearnerConfig config_;
    std::atomic<bool> is_running_{false};
    mutable std::atomic<uint64_t> total_messages_{0};
    mutable std::atomic<uint64_t> total_bytes_{0};
    mutable std::atomic<uint64_t> total_sets_{0};
    mutable std::atomic<uint64_t> total_reads_{0};
    mutable std::atomic<uint64_t> total_deletes_{0};
    mutable std::atomic<int> last_type_{-1};
    mutable std::atomic<int64_t> last_seq_{0};
    mutable std::atomic<int> last_sender_{-1};
    mutable std::atomic<size_t> last_payload_bytes_{0};
    mutable std::thread metrics_thread_;
    mutable std::unique_ptr<resdb::Storage> storage_;
    std::unordered_set<std::string> known_keys_;

    std::vector<int> sequence_status; // 0 = not started, 1 = unknown valid hash, 2 = known valid hash, 3 = completed
    std::vector<resdb::LearnerUpdate> learnerUpdates;
    std::vector<std::tuple<int, std::string, int>> hashCounts;
};

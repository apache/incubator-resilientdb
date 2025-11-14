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
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "executor/common/custom_query.h"
#include "executor/common/transaction_manager.h"
#include "platform/consensus/ordering/raft/proto/raft.pb.h"
#include "platform/consensus/ordering/raft/raft_node.h"
#include "platform/consensus/ordering/raft/raft_log.h"
#include "platform/consensus/ordering/raft/raft_message_type.h"
#include "platform/consensus/ordering/raft/raft_persistent_state.h"
#include "platform/consensus/ordering/raft/raft_rpc.h"
#include "platform/consensus/ordering/raft/raft_snapshot_manager.h"
#include "platform/networkstrate/consensus_manager.h"

namespace resdb {

// ConsensusManagerRaft provides the glue layer between the generic
// ServiceNetwork entry points and the RAFT core (log replication, state
// machine apply, etc).  The actual RAFT implementation can register
// callbacks through Set*Handler()/SetHeartbeatTask() so that this class can
// stay focused on wiring and lifecycle management.
class ConsensusManagerRaft : public ConsensusManager {
 public:
  using RequestHandler =
      std::function<int(std::unique_ptr<Context>, std::unique_ptr<Request>)>;
  using HeartbeatTask = std::function<void()>;

  ConsensusManagerRaft(const ResDBConfig& config,
                       std::unique_ptr<TransactionManager> executor,
                       std::unique_ptr<CustomQuery> query_executor = nullptr);
  ~ConsensusManagerRaft() override;

  int ConsensusCommit(std::unique_ptr<Context> context,
                      std::unique_ptr<Request> request) override;

  std::vector<ReplicaInfo> GetReplicas() override;
  uint32_t GetPrimary() override;
  uint32_t GetVersion() override;

  void Start() override;

  TransactionManager* GetTransactionManager() const {
    return transaction_manager_.get();
  }

  CustomQuery* GetCustomQueryExecutor() const {
    return query_executor_.get();
  }

  void SetClientRequestHandler(RequestHandler handler);
  void SetCustomConsensusHandler(RequestHandler handler);
  void SetHeartbeatTask(HeartbeatTask task);

  void UpdateLeadership(uint32_t leader_id, uint64_t term);

  RaftLog* GetRaftLog() { return raft_log_.get(); }
  RaftPersistentState* GetPersistentState() {
    return persistent_state_.get();
  }
  RaftSnapshotManager* GetSnapshotManager() {
    return snapshot_manager_.get();
  }
  RaftRpc* GetRpc() { return raft_rpc_.get(); }

 private:
  int HandleClientRequest(std::unique_ptr<Context> context,
                          std::unique_ptr<Request> request);
  int HandleCustomQuery(std::unique_ptr<Context> context,
                        std::unique_ptr<Request> request);
  int HandleCustomConsensus(std::unique_ptr<Context> context,
                            std::unique_ptr<Request> request);

  void StartHeartbeatThread();
  void StopHeartbeatThread();
  void HeartbeatLoop();

 private:
  std::unique_ptr<TransactionManager> transaction_manager_;
  std::unique_ptr<CustomQuery> query_executor_;
  RequestHandler client_request_handler_;
  RequestHandler consensus_message_handler_;
  HeartbeatTask heartbeat_task_;

  std::thread raft_heartbeat_thread_;
  std::mutex heartbeat_mutex_;
  std::condition_variable heartbeat_cv_;
  std::atomic<bool> heartbeat_running_{false};
  std::atomic<uint32_t> leader_id_{0};
  std::atomic<uint64_t> current_term_{0};

  std::unique_ptr<RaftLog> raft_log_;
  std::unique_ptr<RaftPersistentState> persistent_state_;
  std::unique_ptr<RaftSnapshotManager> snapshot_manager_;
  std::unique_ptr<RaftRpc> raft_rpc_;
  std::unique_ptr<RaftNode> raft_node_;
};

}  // namespace resdb

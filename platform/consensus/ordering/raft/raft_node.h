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
#include <cstdint>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>

#include "absl/status/statusor.h"
#include "executor/common/transaction_manager.h"
#include "platform/config/resdb_config.h"
#include "platform/consensus/ordering/raft/raft_log.h"
#include "platform/consensus/ordering/raft/raft_persistent_state.h"
#include "platform/consensus/ordering/raft/raft_rpc.h"
#include "platform/consensus/ordering/raft/raft_snapshot_manager.h"
#include "platform/networkstrate/consensus_manager.h"

namespace resdb {

class ConsensusManagerRaft;

class RaftNode {
 public:
  RaftNode(const ResDBConfig& config, ConsensusManagerRaft* manager,
           TransactionManager* transaction_manager, RaftLog* log,
           RaftPersistentState* persistent_state,
           RaftSnapshotManager* snapshot_manager, RaftRpc* rpc);
  ~RaftNode();

  void Start();
  void Stop();

  int HandleClientRequest(std::unique_ptr<Context> context,
                          std::unique_ptr<Request> request);
  int HandleConsensusMessage(std::unique_ptr<Context> context,
                             std::unique_ptr<Request> request);
  void HeartbeatTick();

 private:
  enum class Role { kFollower, kCandidate, kLeader };

  struct PendingRequest {
    std::unique_ptr<Context> context;
    std::unique_ptr<Request> request;
  };

  void RunElectionLoop();
  void ResetElectionDeadline();
  void StartElection();
  void BecomeFollower(uint64_t term, uint32_t leader_id);
  void BecomeCandidate();
  void BecomeLeader();
  void SendHeartbeats();
  void BroadcastAppendEntries(bool send_all_entries);
  void MaybeAdvanceCommitIndex();
  void ApplyEntries();
  // Apply a committed log entry to the state machine and return the
  // serialized response (if any) from the TransactionManager.
  std::unique_ptr<std::string> ApplyEntry(uint64_t index,
                                          const raft::LogEntry& entry);

  int HandleAppendEntriesRequest(const Request& envelope,
                                 const raft::AppendEntriesRequest& request);
  int HandleAppendEntriesResponse(const raft::AppendEntriesResponse& response);
  int HandleRequestVoteRequest(const Request& envelope,
                               const raft::RequestVoteRequest& request);
  int HandleRequestVoteResponse(const raft::RequestVoteResponse& response);
  int HandleInstallSnapshotRequest(const Request& envelope,
                                   const raft::InstallSnapshotRequest& request);

  std::optional<ReplicaInfo> ReplicaById(uint32_t node_id) const;
  uint64_t LastLogTerm() const;
  uint64_t GetLogTerm(uint64_t index) const;

  bool IsMajority(uint64_t match_index) const;
  uint64_t RequiredQuorum() const;

  void PersistTermAndVote();

 private:
  const ResDBConfig& config_;
  ConsensusManagerRaft* manager_;
  TransactionManager* transaction_manager_;
  RaftLog* raft_log_;
  RaftPersistentState* persistent_state_;
  RaftSnapshotManager* snapshot_manager_;
  RaftRpc* raft_rpc_;

  std::vector<ReplicaInfo> replicas_;
  std::unordered_map<uint32_t, ReplicaInfo> replica_map_;
  uint32_t self_id_;

  mutable std::mutex state_mutex_;
  Role role_ ABSL_GUARDED_BY(state_mutex_) = Role::kFollower;
  uint64_t current_term_ ABSL_GUARDED_BY(state_mutex_) = 0;
  uint32_t voted_for_ ABSL_GUARDED_BY(state_mutex_) = 0;
  uint32_t leader_id_ ABSL_GUARDED_BY(state_mutex_) = 0;
  uint64_t commit_index_ ABSL_GUARDED_BY(state_mutex_) = 0;
  uint64_t last_applied_ ABSL_GUARDED_BY(state_mutex_) = 0;

  std::unordered_map<uint32_t, uint64_t> match_index_
      ABSL_GUARDED_BY(state_mutex_);
  std::unordered_map<uint32_t, uint64_t> next_index_
      ABSL_GUARDED_BY(state_mutex_);
  std::unordered_map<uint64_t, PendingRequest> pending_requests_
      ABSL_GUARDED_BY(state_mutex_);

  size_t votes_granted_ ABSL_GUARDED_BY(state_mutex_) = 0;
  std::unordered_map<uint32_t, bool> vote_record_ ABSL_GUARDED_BY(state_mutex_);

  std::thread election_thread_;
  std::atomic<bool> stop_{false};
  std::mutex election_mutex_;
  std::condition_variable election_cv_;
  std::chrono::steady_clock::time_point next_election_deadline_
      ABSL_GUARDED_BY(election_mutex_);
};

}  // namespace resdb

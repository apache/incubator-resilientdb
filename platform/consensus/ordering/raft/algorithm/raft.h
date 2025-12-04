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

#include <sys/types.h>
#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <queue>
#include <thread>
#include <chrono>

#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "platform/consensus/ordering/raft/proto/proposal.pb.h"
#include "platform/statistic/stats.h"
#include "platform/consensus/ordering/raft/algorithm/leaderelection_manager.h"
#include "platform/networkstrate/replica_communicator.h"

namespace resdb {
namespace raft {

enum class Role { FOLLOWER, CANDIDATE, LEADER };
enum class TermRelation { STALE, CURRENT, NEW };

class Raft : public common::ProtocolBase {
 public:
  Raft(int id, int f, int total_num,
    SignatureVerifier* verifier,
    LeaderElectionManager* leaderelection_manager,
    ReplicaCommunicator* replica_communicator
  );
  ~Raft();

  bool ReceiveTransaction(std::unique_ptr<AppendEntries> txn);
  bool ReceivePropose(std::unique_ptr<AppendEntries> txn);
  bool ReceiveAppendEntriesResponse(std::unique_ptr<AppendEntriesResponse> response);
  void ReceiveRequestVote(std::unique_ptr<RequestVote> rv);
  void ReceiveRequestVoteResponse(std::unique_ptr<RequestVoteResponse> rvr);


  raft::Role GetRoleSnapshot() const;
  void StartElection();
  void SendHeartBeat();

 private:
  TermRelation TermCheckLocked(uint64_t term) const;  // Must be called under mutex
  bool DemoteSelfLocked(uint64_t term); // Must be called under mutex
  uint64_t getLastLogTermLocked() const; // Must be called under mutex
  bool IsStop();

 private:
  mutable std::mutex mutex_;
  
  // Persistent state on all servers:
  uint64_t currentTerm_; // Protected by mutex_
  int votedFor_; // Protected by mutex_
  std::vector<std::unique_ptr<AppendEntries>> log_; // Protected by mutex_

  // Volatile state on leaders:
  std::vector<uint64_t> nextIndex_; // Protected by mutex_
  std::vector<uint64_t> matchIndex_; // Protected by mutex_
  uint64_t heartBeatsSentThisTerm_; // Protected by mutex_

  // Volatile state on all servers:
  uint64_t commitIndex_; // Protected by mutex_
  uint64_t lastApplied_; // Protected by mutex_
  Role role_; // Protected by mutex_
  int LeaderId; // Protected by mutex_
  std::vector<int> votes_; // Protected by mutex_
  std::chrono::steady_clock::time_point last_ae_time_;
  std::chrono::steady_clock::time_point last_heartbeat_time_; // Protected by mutex_

  uint64_t lastLogIndex_;
  bool is_stop_;
  const uint64_t quorum_;
  SignatureVerifier* verifier_;
  LeaderElectionManager* leader_election_manager_;
  Stats* global_stats_;
  ReplicaCommunicator* replica_communicator_;
};

}  // namespace raft
}  // namespace resdb

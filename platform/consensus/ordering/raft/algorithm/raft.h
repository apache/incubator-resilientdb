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

#include <deque>
#include <map>
#include <queue>
#include <thread>

#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "platform/consensus/ordering/raft/proto/proposal.pb.h"
#include "platform/statistic/stats.h"
#include "platform/consensus/ordering/raft/algorithm/leaderelection_manager.h"

namespace resdb {
namespace raft {

enum class Role { FOLLOWER, CANDIDATE, LEADER };

class Raft : public common::ProtocolBase {
 public:
  Raft(int id, int f, int total_num,
    SignatureVerifier* verifier,
    LeaderElectionManager* leaderelection_manager);
  ~Raft();

  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  bool ReceivePropose(std::unique_ptr<Transaction> txn);
  bool ReceivePrepare(std::unique_ptr<Proposal> proposal);

  raft::Role GetRoleSnapshot() const;
  void StartElection();
  void SendHeartBeat();

 private:
  bool IsStop();

 private:
  int64_t seq_;
  bool is_stop_;
  SignatureVerifier* verifier_;
  LeaderElectionManager* leader_election_manager_;
  Stats* global_stats_;
  std::map<std::string, std::set<int32_t> > received_;
  
  mutable std::mutex raft_mutex_;
  Role role_; // Protected by raft_mutex_
  uint64_t currentTerm_; // Protected by raft_mutex_
  int votedFor_; // Protected by raft_mutex_
  std::map<std::string, std::unique_ptr<Transaction>> log_; // Protected by raft_mutex_
  uint64_t commit_index_; // Protected by raft_mutex_
  uint64_t last_applied_; // Protected by raft_mutex_
  std::map<int, uint64_t> next_index_; // Protected by raft_mutex_
  std::map<int, uint64_t> match_index_; // Protected by raft_mutex_
  int LeaderId; // Protected by raft_mutex_
};

}  // namespace raft
}  // namespace resdb

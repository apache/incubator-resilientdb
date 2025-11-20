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

#include "platform/consensus/ordering/raft/algorithm/raft.h"

#include <glog/logging.h>

#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"
#include "platform/proto/resdb.pb.h"

namespace resdb {
namespace raft {

Raft::Raft(int id, int f, int total_num, SignatureVerifier* verifier,
  LeaderElectionManager* leaderelection_manager)
    : ProtocolBase(id, f, total_num), 
    verifier_(verifier),
    leader_election_manager_(leaderelection_manager),
    role_(raft::Role::FOLLOWER) {
  LOG(ERROR) << "get proposal graph";
  id_ = id;
  total_num_ = total_num;
  f_ = f;
  is_stop_ = false;
  seq_ = 0;
}

Raft::~Raft() { is_stop_ = true; }

bool Raft::IsStop() { return is_stop_; }

bool Raft::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  // LOG(ERROR)<<"recv txn:";
  txn->set_create_time(GetCurrentTime());
  txn->set_seq(seq_++);
  txn->set_proposer(id_);

  Broadcast(MessageType::Propose, *txn);
  return true;
}

bool Raft::ReceivePropose(std::unique_ptr<Transaction> txn) {
  std::string hash = txn->hash();
  int64_t seq = txn->seq();
  int proposer = txn->proposer();
  {
    std::unique_lock<std::mutex> lk(raft_mutex_);
    log_[txn->hash()] = std::move(txn);
  }

  Proposal proposal;
  proposal.set_hash(hash);
  proposal.set_seq(seq);
  proposal.set_proposer(id_);
  Broadcast(MessageType::Prepare, proposal);
  return true;
}

bool Raft::ReceivePrepare(std::unique_ptr<Proposal> proposal) {
  std::unique_ptr<Transaction> txn = nullptr;
  {
    std::unique_lock<std::mutex> lk(raft_mutex_);
    received_[proposal->hash()].insert(proposal->proposer());
    auto it = log_.find(proposal->hash());
    if (it != log_.end()) {
      if (received_[proposal->hash()].size() >= 2 * f_ + 1) {
        txn = std::move(it->second);
        log_.erase(it);
      }
    }
  }
  if (txn != nullptr) {
    commit_(*txn);
  }
  else {
  }
  return true;
}

raft::Role Raft::GetRoleSnapshot() const {
  std::lock_guard<std::mutex> lk(raft_mutex_);
  return role_;
}

// TODO SET LASTLOGINDEX AND LASTLOGTERM UPON MERGE
// Called from LeaderElectionManager::StartElection when timeout
void Raft::StartElection() {
  LOG(INFO) << "JIM -> " << __FUNCTION__;
  uint64_t currentTerm;
  int candidateId;
  uint64_t lastLogIndex;
  uint64_t lastLogTerm;
  bool roleChanged = false;

  {
    std::lock_guard<std::mutex> lk(raft_mutex_);
    if (role_ == raft::Role::LEADER) {
      LOG(WARNING) << __FUNCTION__ << ": Leader tried to start election";
      return;
    }
    if (role_ == raft::Role::FOLLOWER) {
      LOG(INFO) << __FUNCTION__ << ": FOLLOWER->CANDIDATE";
      role_ = raft::Role::CANDIDATE;
      roleChanged = true;
    }
    currentTerm_++;
    votedFor_ = id_;

    currentTerm = currentTerm_;
    candidateId = id_;

    // TODO
    lastLogIndex = 0;
    lastLogTerm = 0;
  }
  if (roleChanged) {
    leader_election_manager_->OnRoleChange();
  }

  RequestVote requestVote;
  requestVote.set_term(currentTerm);
  requestVote.set_candidateid(candidateId);
  requestVote.set_lastlogindex(lastLogIndex);
  requestVote.set_lastlogterm(lastLogTerm);
  Broadcast(MessageType::RequestVoteMsg, requestVote);
}

// TODO
// ON MERGE FIX VALUES
void Raft::SendHeartBeat() {
  LOG(INFO) << "JIM -> " << __FUNCTION__;
  uint64_t currentTerm;
  int leaderId = id_;
  uint64_t prevLogIndex;
  uint64_t prevLogTerm;
  std::string entries;
  uint64_t leaderCommit;
  {
    std::lock_guard<std::mutex> lk(raft_mutex_);
    if (role_ != raft::Role::LEADER) {
      LOG(WARNING) << __FUNCTION__ << ": Non-Leader tried to start HeartBeat";
      return;
    }
    currentTerm = currentTerm_;
    prevLogIndex = 0;
    prevLogTerm = 0;
    entries = "";
    leaderCommit = 0;  
  }
  AppendEntries appendEntries;
  appendEntries.set_term(currentTerm);
  appendEntries.set_proposer(leaderId);
  appendEntries.set_leadercommitindex(prevLogIndex); // wrong function
  appendEntries.set_prevlogterm(prevLogTerm);
  appendEntries.set_data(entries);
  appendEntries.set_leadercommitindex(leaderCommit);
  // TODO Need to make sure leader no-ops their own heartbeats
  Broadcast(MessageType::AppendEntriesMsg, appendEntries);
}

}  // namespace raft
}  // namespace resdb

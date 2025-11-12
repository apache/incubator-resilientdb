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

namespace resdb {
namespace raft {

Raft::Raft(int id, int f, int total_num, SignatureVerifier* verifier)
    : ProtocolBase(id, f, total_num), verifier_(verifier) {
  LOG(ERROR) << "get proposal graph";
  id_ = id;
  total_num_ = total_num;
  f_ = f;
  is_stop_ = false;
  seq_ = 0;
  currentTerm_ = 0;
  votedFor_ = -1;
  commitIndex_ = 0;
  lastApplied_ = 0;
  nextIndex_.assign(total_num_, 0);
  matchIndex_.assign(total_num_, 0);
}

Raft::~Raft() { is_stop_ = true; }

bool Raft::IsStop() { return is_stop_; }

bool Raft::ReceiveTransaction(std::unique_ptr<AppendEntries> txn) {
  // LOG(ERROR)<<"recv txn:";
  LOG(INFO) << "Received Transaction to primary id: " << id_;
  LOG(INFO) << "seq: " << seq_;
  txn->set_create_time(GetCurrentTime());
  txn->set_seq(seq_++);
  txn->set_proposer(id_);
  
  // For now just set this to currentTerm_, but is wrong if it just became leader
  txn->set_prevlogterm(currentTerm_);

  // leader sends out highest seq that is committed
  txn->set_leadercommitindex(commitIndex_);

  // This should be a term for each entry, but assuming no failure at first
  txn->set_term(currentTerm_); 

  Broadcast(MessageType::AppendEntriesMsg, *txn);
  return true;
}

bool Raft::ReceivePropose(std::unique_ptr<AppendEntries> txn) {
  auto leader_id = txn->id();
  AppendEntriesResponse appendEntriesResponse;
  appendEntriesResponse.set_term(currentTerm_);
  appendEntriesResponse.set_id(id_);
  appendEntriesResponse.set_lastapplied(lastApplied_);
  appendEntriesResponse.set_nextentry(data_.size());
  if (txn->term() < currentTerm_) {
    appendEntriesResponse.set_success(false);
    SendMessage(MessageType::AppendEntriesResponseMsg, appendEntriesResponse, leader_id);
    return true;
  }
  auto prevSeq = txn->seq() - 1;
  // This should be the same as checking if it has an entry
  // with this prevLogIndex and term
  if (prevSeq != -1 && 
    (prevSeq >= static_cast<int64_t>(data_.size()) ||
      txn->prevlogterm() != data_[dataIndexMapping_[prevSeq]]->term())) {
    appendEntriesResponse.set_success(false);
    SendMessage(MessageType::AppendEntriesResponseMsg, appendEntriesResponse, leader_id);
    return true;
  }
  // Implement an entry existing but with a different term
  // delete that entry and all after it

  std::string hash = txn->hash();
  int64_t seq = txn->seq();
  {
    std::unique_lock<std::mutex> lk(mutex_);
    data_[txn->hash()] = std::move(txn);
    dataIndexMapping_.push_back(txn->hash());
  }
  auto leaderCommit = txn->leadercommitindex();
  while (leaderCommit > commitIndex_ && lastApplied_ + 1 <= static_cast<int64_t>(data_.size())) {
    std::unique_ptr<AppendEntries> txnToCommit = nullptr;
    txnToCommit = std::move(data_[dataIndexMapping_[lastApplied_]]);
    commit_(*txnToCommit);
    lastApplied_++;
  }
  // I don't quite know if this needs to be conditional, but that's how the paper says it
  if (leaderCommit > commitIndex_)
    // not 100% certain if this second variable should be seq
    commitIndex_ = std::min(leaderCommit, seq);

  appendEntriesResponse.set_lastapplied(lastApplied_);
  appendEntriesResponse.set_nextentry(data_.size());
  appendEntriesResponse.set_success(true);
  SendMessage(MessageType::AppendEntriesResponseMsg, appendEntriesResponse, leader_id);
  return true;
}

bool Raft::ReceiveAppendEntriesResponse(std::unique_ptr<AppendEntriesResponse> response) {
  auto followerId = response->id();
  if (response->success()) {
    nextIndex_[followerId] = response->nextentry();
    matchIndex_[followerId] = response->lastapplied();
    return true;
  }
  
  // handling for if leader is out of date and term is wrong

  // handling for if term is correct, but follower is just out of date
  --nextIndex_[followerId];
  // send message
  assert(false);
  return true;
}

}  // namespace raft
}  // namespace resdb

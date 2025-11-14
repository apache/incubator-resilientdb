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

static std::string ToHex(const std::string& input, size_t max_len = 16) {
  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  for (size_t i = 0; i < std::min(input.size(), max_len); ++i) {
    oss << std::setw(2) << static_cast<unsigned>(static_cast<unsigned char>(input[i]));
  }
  return oss.str();
}

static void printAppendEntries(const std::unique_ptr<AppendEntries>& txn) {
  if (!txn) {
    LOG(INFO) << "AppendEntries: nullptr";
    return;
  }

  LOG(INFO) << "=== AppendEntries ===";
  LOG(INFO) << "id: " << txn->id();
  LOG(INFO) << "term: " << txn->term();
  LOG(INFO) << "seq: " << txn->seq();
  LOG(INFO) << "prevLogTerm: " << txn->prevlogterm();
  LOG(INFO) << "leaderCommitIndex: " << txn->leadercommitindex();
  LOG(INFO) << "proxy_id: " << txn->proxy_id();
  LOG(INFO) << "proposer: " << txn->proposer();
  LOG(INFO) << "uid: " << txn->uid();
  LOG(INFO) << "create_time: " << txn->create_time();

  // bytes fields (print as hex or limited string to avoid binary garbage)
  const std::string& data = txn->data();
  const std::string& hash = txn->hash();

  LOG(INFO) << "data size: " << data.size();
  if (!data.empty()) {
    LOG(INFO) << "data (first 32 bytes): "
              << data.substr(0, std::min<size_t>(32, data.size()));
  }

  LOG(INFO) << "hash size: " << hash.size();
  if (!hash.empty()) {
    LOG(INFO) << "hash (hex first 16 bytes): "
              << ToHex(hash);
  }

  LOG(INFO) << "=====================";
}

Raft::Raft(int id, int f, int total_num, SignatureVerifier* verifier)
    : ProtocolBase(id, f, total_num), verifier_(verifier) {
  LOG(INFO) << "get proposal graph";
  id_ = id;
  total_num_ = total_num;
  f_ = f;
  is_stop_ = false;
  seq_ = 0;
  currentTerm_ = 0;
  votedFor_ = -1;
  commitIndex_ = 0;
  lastApplied_ = 0;
  nextIndex_.assign(total_num_ + 1, 0);
  matchIndex_.assign(total_num_ + 1, 0);
}

Raft::~Raft() { is_stop_ = true; }

bool Raft::IsStop() { return is_stop_; }

void Raft::Dump() {
  LOG(INFO) << "=== Replica Dump ===";
  LOG(INFO) << "id_: " << id_;
  LOG(INFO) << "currentTerm_: " << currentTerm_;
  LOG(INFO) << "votedFor_: " << votedFor_;
  LOG(INFO) << "commitIndex_: " << commitIndex_;
  LOG(INFO) << "lastApplied_: " << lastApplied_;
}

bool Raft::ReceiveTransaction(std::unique_ptr<AppendEntries> txn) {
  // LOG(INFO)<<"recv txn:";

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
  LOG(INFO) << "Before";
  printAppendEntries(txn);
  LOG(INFO) << "After";
  
  Broadcast(MessageType::AppendEntriesMsg, *txn);
  return true;
}

bool Raft::ReceivePropose(std::unique_ptr<AppendEntries> txn) {
  Dump();
  auto leader_id = txn->proposer();
  auto leaderCommit = txn->leadercommitindex();
  LOG(INFO) << "Received AppendEntries to replica id: " << id_;
  LOG(INFO) << "static_cast<int64_t>(data_.size()): " << static_cast<int64_t>(data_.size());
  printAppendEntries(txn);
  AppendEntriesResponse appendEntriesResponse;
  appendEntriesResponse.set_term(currentTerm_);
  appendEntriesResponse.set_id(id_);
  appendEntriesResponse.set_lastapplied(lastApplied_);
  appendEntriesResponse.set_nextentry(data_.size());
  if (txn->term() < currentTerm_) {
    LOG(INFO) << "AppendEntriesMsg Fail1";
    appendEntriesResponse.set_success(false);
    SendMessage(MessageType::AppendEntriesResponseMsg, appendEntriesResponse, leader_id);
    return true;
  }
  auto prevSeq = txn->seq() - 1;
  // This should be the same as checking if it has an entry
  // with this prevLogIndex and term
  if (prevSeq != 0 && prevSeq > static_cast<int64_t>(dataIndexMapping_.size()) &&
    (prevSeq >= static_cast<int64_t>(dataIndexMapping_.size()) ||
      txn->prevlogterm() != data_[dataIndexMapping_[prevSeq]]->term())) {
    LOG(INFO) << "AppendEntriesMsg Fail2";
    LOG(INFO) << "prevSeq: " << prevSeq << " data size: " << static_cast<int64_t>(data_.size());
    if (prevSeq < static_cast<int64_t>(dataIndexMapping_.size())){
      LOG(INFO) << "txn->prevlogterm(): " << txn->prevlogterm() 
              << " last entry term: " << data_[dataIndexMapping_[prevSeq]]->term();
    }
    appendEntriesResponse.set_success(false);
    SendMessage(MessageType::AppendEntriesResponseMsg, appendEntriesResponse, leader_id);
    return true;
  }
  // Implement an entry existing but with a different term
  // delete that entry and all after it
  LOG(INFO) << "Before AppendEntriesMsg Added to Log";
  std::string hash = txn->hash();
  int64_t seq = txn->seq();
  {
    std::unique_lock<std::mutex> lk(mutex_);
    std::string hash = txn->hash();
    LOG(INFO) << "Before adding to data";
    data_[txn->hash()] = std::move(txn);
    LOG(INFO) << "After adding to data";
    dataIndexMapping_.push_back(hash);
  }
  LOG(INFO) << "AppendEntriesMsg Added to Log";
  
  LOG(INFO) << "leaderCommit: " << leaderCommit;
  LOG(INFO) << "commitIndex_: " << commitIndex_;
  LOG(INFO) << "lastApplied_: " << lastApplied_;
  LOG(INFO) << "static_cast<int64_t>(data_.size()): " << static_cast<int64_t>(data_.size());
  LOG(INFO) << "leaderCommit > commitIndex_: " << (leaderCommit > commitIndex_ ? "true" : "false");
  LOG(INFO) << "lealastApplied_ + 1 <= static_cast<int64_t>(data_.size()) " << ((lastApplied_ + 1 <= static_cast<int64_t>(data_.size())) ? "true" : "false");
  while ((leaderCommit != 0) && leaderCommit > commitIndex_ && lastApplied_ + 1 <= static_cast<int64_t>(data_.size())) {
    // assert(false);
    LOG(INFO) << "AppendEntriesMsg Committing";
    std::unique_ptr<AppendEntries> txnToCommit = nullptr;
    txnToCommit = std::move(data_[dataIndexMapping_[lastApplied_]]);
    commit_(*txnToCommit);
    lastApplied_++;
  }
  LOG(INFO) << "before commit index check";
  // I don't quite know if this needs to be conditional, but that's how the paper says it
  if (leaderCommit > commitIndex_)
    // not 100% certain if this second variable should be seq
    commitIndex_ = std::min(leaderCommit, seq);

  LOG(INFO) << "after commit index check";
  appendEntriesResponse.set_lastapplied(lastApplied_);
  appendEntriesResponse.set_nextentry(data_.size());
  appendEntriesResponse.set_success(true);
  appendEntriesResponse.set_hash(hash);
  appendEntriesResponse.set_seq(seq);
  LOG(INFO) << "Leader_id: " << leader_id;
  SendMessage(MessageType::AppendEntriesResponseMsg, appendEntriesResponse, leader_id);
  // Broadcast(MessageType::AppendEntriesResponseMsg, appendEntriesResponse);
  return true;
}

bool Raft::ReceiveAppendEntriesResponse(std::unique_ptr<AppendEntriesResponse> response) {
  if (id_ != 1)
    return true;
  LOG(INFO) << "ReceiveAppendEntriesResponse";
  auto followerId = response->id();
  LOG(INFO) << "followerId: " << followerId;
  if (response->success()) {
    {
      std::unique_lock<std::mutex> lk(mutex_);
      received_[response->hash()].insert(response->id());
      auto it = data_.find(response->hash());
      if (it != data_.end()) {
        LOG(INFO) << "Transaction: " << response->seq() <<  " has gotten " << received_[response->hash()].size() << " responses";
        if (static_cast<int64_t>(received_[response->hash()].size()) >= f_ + 1) {
          commitIndex_ = response->seq();

          // pretty sure this should always be in order with no gaps
          while (lastApplied_ + 1 <= static_cast<int64_t>(data_.size()) &&
                  lastApplied_ <= commitIndex_) {
            LOG(INFO) << "Leader Committing";
            std::unique_ptr<AppendEntries> txnToCommit = nullptr;
            txnToCommit = std::move(data_[dataIndexMapping_[lastApplied_]]);
            commit_(*txnToCommit);
            lastApplied_++;
          }
        }
      }
    }
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

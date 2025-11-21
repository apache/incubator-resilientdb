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
#include <algorithm>
#include <cstdint>
#include <memory>

#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"
#include "platform/consensus/ordering/raft/proto/proposal.pb.h"
#include "platform/proto/resdb.pb.h"

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
  LOG(INFO) << "term: " << txn->term();
  LOG(INFO) << "prevLogIndex: " << txn->prevlogindex();
  LOG(INFO) << "prevLogTerm: " << txn->prevlogterm();
  LOG(INFO) << "leaderCommitIndex: " << txn->leadercommitindex();
  LOG(INFO) << "proxy_id: " << txn->proxy_id();
  LOG(INFO) << "leaderId: " << txn->leaderid();
  LOG(INFO) << "uid: " << txn->uid();
  LOG(INFO) << "create_time: " << txn->create_time();

  // bytes fields (print as hex or limited string to avoid binary garbage)
  const std::string& entries = txn->entries();
  const std::string& hash = txn->hash();

  LOG(INFO) << "entries size: " << entries.size();
  if (!entries.empty()) {
    LOG(INFO) << "entries (first 32 bytes): "
              << entries.substr(0, std::min<size_t>(32, entries.size()));
  }

  LOG(INFO) << "hash size: " << hash.size();
  if (!hash.empty()) {
    LOG(INFO) << "hash (hex first 16 bytes): "
              << ToHex(hash);
  }

  LOG(INFO) << "=====================";
}

Raft::Raft(int id, int f, int total_num, SignatureVerifier* verifier,
  LeaderElectionManager* leaderelection_manager)
    : ProtocolBase(id, f, total_num), 
    role_(raft::Role::FOLLOWER),
    verifier_(verifier),
    leader_election_manager_(leaderelection_manager) {
  LOG(ERROR) << "get proposal graph";
  id_ = id;
  total_num_ = total_num;
  f_ = f;
  is_stop_ = false;
  prevLogIndex_ = 0;
  currentTerm_ = 0;
  votedFor_ = -1;
  commitIndex_ = 0;
  lastApplied_ = 0;

  AppendEntries ae;
  ae.set_leaderid(0);
  ae.set_prevlogindex(0);
  ae.set_prevlogterm(0);
  ae.set_leadercommitindex(0);
  ae.set_term(0);
  const std::string key = "7622832959";
  logIndexMapping_.push_back(key);
  log_[key] = std::make_unique<AppendEntries>(ae);

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
  LOG(INFO) << "prevLogIndex: " << prevLogIndex_;
  txn->set_create_time(GetCurrentTime());
  txn->set_prevlogindex(prevLogIndex_++);
  txn->set_leaderid(id_);
  
  // For now just set this to currentTerm_, but is wrong if it just became leader
  txn->set_prevlogterm(currentTerm_);

  // leader sends out highest prevLogIndex that is committed
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
  if (txn->leaderid() == id_) { 
    LOG(INFO) << "JIM -> " << __FUNCTION__ << ": discarding message from self";
    return false;
  }
  Dump();
  auto leader_id = txn->leaderid();
  auto leaderCommit = txn->leadercommitindex();
  LOG(INFO) << "Received AppendEntries to replica id: " << id_;
  LOG(INFO) << "static_cast<int64_t>(log_.size()): " << static_cast<int64_t>(log_.size());
  printAppendEntries(txn);
  AppendEntriesResponse appendEntriesResponse;
  appendEntriesResponse.set_term(currentTerm_);
  appendEntriesResponse.set_id(id_);
  appendEntriesResponse.set_lastapplied(lastApplied_);
  appendEntriesResponse.set_nextentry(log_.size());
  if (txn->term() < currentTerm_) {
    LOG(INFO) << "AppendEntriesMsg Fail1";
    appendEntriesResponse.set_success(false);
    SendMessage(MessageType::AppendEntriesResponseMsg, appendEntriesResponse, leader_id);
    return true;
  }
  auto prevprevLogIndex = txn->prevlogindex() - 1;
  // This should be the same as checking if it has an entry
  // with this prevLogIndex and term
  if (prevprevLogIndex != 0 && prevprevLogIndex > static_cast<int64_t>(logIndexMapping_.size()) &&
    (prevprevLogIndex >= static_cast<int64_t>(logIndexMapping_.size()) ||
      txn->prevlogterm() != log_[logIndexMapping_[prevprevLogIndex]]->term())) {
    LOG(INFO) << "AppendEntriesMsg Fail2";
    LOG(INFO) << "prevprevLogIndex: " << prevprevLogIndex << " entries size: " << static_cast<int64_t>(log_.size());
    if (prevprevLogIndex < static_cast<int64_t>(logIndexMapping_.size())){
      LOG(INFO) << "txn->prevlogterm(): " << txn->prevlogterm() 
              << " last entry term: " << log_[logIndexMapping_[prevprevLogIndex]]->term();
    }
    appendEntriesResponse.set_success(false);
    SendMessage(MessageType::AppendEntriesResponseMsg, appendEntriesResponse, leader_id);
    return true;
  }
  // Implement an entry existing but with a different term
  // delete that entry and all after it
  LOG(INFO) << "Before AppendEntriesMsg Added to Log";
  std::string hash = txn->hash();
  int64_t prevLogIndex = txn->prevlogindex();
  {
    std::unique_lock<std::mutex> lk(mutex_);
    std::string hash = txn->hash();
    LOG(INFO) << "Before adding to entries";
    log_[txn->hash()] = std::move(txn);
    LOG(INFO) << "After adding to entries";
    logIndexMapping_.push_back(hash);
  }
  LOG(INFO) << "AppendEntriesMsg Added to Log";
  
  LOG(INFO) << "leaderCommit: " << leaderCommit;
  LOG(INFO) << "commitIndex_: " << commitIndex_;
  LOG(INFO) << "lastApplied_: " << lastApplied_;
  LOG(INFO) << "static_cast<int64_t>(log_.size()): " << static_cast<int64_t>(log_.size());
  LOG(INFO) << "leaderCommit > commitIndex_: " << (leaderCommit > commitIndex_ ? "true" : "false");
  LOG(INFO) << "lastApplied_ + 1 <= static_cast<int64_t>(log_.size()) " << ((lastApplied_ + 1 <= static_cast<int64_t>(log_.size())) ? "true" : "false");
  while (leaderCommit > lastApplied_ && lastApplied_ + 1 <= static_cast<int64_t>(log_.size())) {
    // assert(false);
    LOG(INFO) << "AppendEntriesMsg Committing";
    std::unique_ptr<AppendEntries> txnToCommit = nullptr;
    txnToCommit = std::move(log_[logIndexMapping_[lastApplied_]]);
    commit_(*txnToCommit);
    lastApplied_++;
  }
  LOG(INFO) << "before commit index check";
  // I don't quite know if this needs to be conditional, but that's how the paper says it
  if (leaderCommit > commitIndex_)
    // not 100% certain if this second variable should be prevLogIndex
    commitIndex_ = std::min(leaderCommit, prevLogIndex);

  LOG(INFO) << "after commit index check";
  appendEntriesResponse.set_lastapplied(lastApplied_);
  appendEntriesResponse.set_nextentry(log_.size());
  appendEntriesResponse.set_success(true);
  appendEntriesResponse.set_hash(hash);
  appendEntriesResponse.set_prevlogindex(prevLogIndex);
  LOG(INFO) << "Leader_id: " << leader_id;
  leader_election_manager_->OnHeartBeat();
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
      auto it = log_.find(response->hash());
      if (it != log_.end()) {
        LOG(INFO) << "Transaction: " << response->prevlogindex() <<  " has gotten " << received_[response->hash()].size() << " responses";
        if (static_cast<int64_t>(received_[response->hash()].size()) >= f_ + 1) {
          commitIndex_ = response->prevlogindex();

          // pretty sure this should always be in order with no gaps
          while (lastApplied_ + 1 <= static_cast<int64_t>(log_.size()) &&
                  lastApplied_ <= commitIndex_) {
            LOG(INFO) << "Leader Committing";
            std::unique_ptr<AppendEntries> txnToCommit = nullptr;
            txnToCommit = std::move(log_[logIndexMapping_[lastApplied_]]);
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


void Raft::ReceiveRequestVote(std::unique_ptr<RequestVote> rv) {
  LOG(INFO) << "JIM -> " << __FUNCTION__;
  int rvSender = rv->candidateid();
  uint64_t rvTerm = rv->term();

  uint64_t term;
  bool voteGranted = false;
  bool roleChanged = false;

  if (rvSender == id_) { 
    LOG(INFO) << "JIM -> " << __FUNCTION__ << ": discarding message from self";
    return;
  }

  LOG(INFO) << "JIM -> " << __FUNCTION__ << ": before lock";
  [&]() {
    std::lock_guard<std::mutex> lk(mutex_);
    // If their term is higher than ours, we accept new term, reset votedFor
    // and convert to follower
    TermRelation tr = TermCheckLocked(rvTerm);
    if (tr == TermRelation::STALE) {
      term = currentTerm_;
      return;
    }
    else if (tr == TermRelation::NEW) { roleChanged = DemoteSelfLocked(rvTerm); }
    // Then we continue voting process
    term = currentTerm_;
    uint64_t lastLogTerm = getPrevLogTermLocked();
    LOG(INFO) << "JIM -> " << __FUNCTION__ << ": prev terms at least equal";
    if (rv->lastlogterm() < lastLogTerm) { return; }
    LOG(INFO) << "JIM -> " << __FUNCTION__ << ": prev log terms at least equal";
    if (rv->lastlogterm() == lastLogTerm 
        && rv->lastlogindex() < getPrevLogIndexLocked()) { return; }
    LOG(INFO) << "JIM -> " << __FUNCTION__ << ": candidate is valid";
    if (votedFor_ == -1 || votedFor_ == rvSender) {
      votedFor_ = rvSender;
      voteGranted = true;
      LOG(INFO) << "JIM -> " << __FUNCTION__ << ": voted for " << rvSender;
    }
  }();
  if (roleChanged) { leader_election_manager_->OnRoleChange(); }
  if (voteGranted) { leader_election_manager_->OnHeartBeat(); }

  RequestVoteResponse rvr;
  rvr.set_term(term);
  rvr.set_voterid(id_);
  rvr.set_votegranted(voteGranted);
  SendMessage(MessageType::RequestVoteResponseMsg, rvr, rvSender);
}

void Raft::ReceiveRequestVoteResponse(std::unique_ptr<RequestVoteResponse> rvr) {
  uint64_t term = rvr->term();
  int voterId = rvr->voterid();
  bool votedYes = rvr->votegranted();
  bool demoted = false;
  bool elected = false;
  int votesNeeded = total_num_ - f_;

  [&]() {
    std::lock_guard<std::mutex> lk(mutex_);
    TermRelation tr = TermCheckLocked(term);
    if (tr == TermRelation::STALE) { return; }
    else if (tr == TermRelation::NEW) { 
      demoted = DemoteSelfLocked(term);
      return;
    }
    if (role_ != Role::CANDIDATE) { return; }
    if (!votedYes) { return; }
    bool dupe = (std::find(votes_.begin(), votes_.end(), voterId) != votes_.end());
    if (dupe) { return; }
    votes_.push_back(voterId);
    LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Replica " << voterId << " voted for me. Votes: " << votes_.size() << "/" << votesNeeded;
    if (votes_.size() >= votesNeeded) {
      elected = true;
      role_ = Role::LEADER;
      LOG(INFO) << "JIM -> " << __FUNCTION__ << ": CANDIDATE -> LEADER";
    }
  }();
    if (demoted || elected) { leader_election_manager_->OnRoleChange(); }
    if (elected) { SendHeartBeat(); }
}

raft::Role Raft::GetRoleSnapshot() const {
  std::lock_guard<std::mutex> lk(mutex_);
  return role_;
}

// Called from LeaderElectionManager::StartElection when timeout
void Raft::StartElection() {
  LOG(INFO) << "JIM -> " << __FUNCTION__;
  uint64_t currentTerm;
  int candidateId;
  uint64_t lastLogIndex;
  uint64_t lastLogTerm;
  bool roleChanged = false;

  {
    std::lock_guard<std::mutex> lk(mutex_);
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
    votes_.clear();
    votes_.push_back(id_);

    currentTerm = currentTerm_;
    candidateId = id_;
    lastLogIndex = getPrevLogIndexLocked();
    lastLogTerm = getPrevLogTermLocked();
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
    std::lock_guard<std::mutex> lk(mutex_);
    if (role_ != raft::Role::LEADER) {
      LOG(WARNING) << __FUNCTION__ << ": Non-Leader tried to start HeartBeat";
      return;
    }
    currentTerm = currentTerm_;
    prevLogIndex = getPrevLogIndexLocked();
    prevLogTerm = getPrevLogTermLocked();
    entries = "";
    leaderCommit = 0;  // TODO
  }
  AppendEntries appendEntries;
  appendEntries.set_term(currentTerm);
  appendEntries.set_leaderid(leaderId);
  appendEntries.set_prevlogindex(prevLogIndex);
  appendEntries.set_prevlogterm(prevLogTerm);
  appendEntries.set_entries(entries);
  appendEntries.set_leadercommitindex(leaderCommit); // TODO
  Broadcast(MessageType::AppendEntriesMsg, appendEntries);
}

// requires raft mutex to be held
// returns true if demoted
bool Raft::DemoteSelfLocked(uint64_t term) {
  currentTerm_ = term;
  votedFor_ = -1;
  if (role_ != raft::Role::FOLLOWER) {
    role_ = raft::Role::FOLLOWER;
    LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Demoted to FOLLOWER";
    return true;
  }
  return false;
}

// requires raft mutex to be held
TermRelation Raft::TermCheckLocked(uint64_t term) const {
  if (term < currentTerm_) { return TermRelation::STALE; }
  else if (term == currentTerm_) { return TermRelation::CURRENT; }
  else { return TermRelation::NEW; }
}

// requires raft mutex to be held
uint64_t Raft::getPrevLogIndexLocked() const {
  return logIndexMapping_.size() - 1;
}

// requires raft mutex to be held
uint64_t Raft::getPrevLogTermLocked() const {
  if (logIndexMapping_.empty()) { return 0; }
  const std::string& key = logIndexMapping_.back();
  auto it = log_.find(key);
  if (it == log_.end() || !it->second) {
    LOG(FATAL) << __FUNCTION__ << ": inconsistency found between log vector and log map";
  }
  return it->second->term();
}

}  // namespace raft
}  // namespace resdb

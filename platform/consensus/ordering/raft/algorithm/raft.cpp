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

Raft::Raft(int id, int f, int total_num, SignatureVerifier* verifier,
  LeaderElectionManager* leaderelection_manager)
    : ProtocolBase(id, f, total_num), 
    role_(raft::Role::FOLLOWER),
    verifier_(verifier),
    leader_election_manager_(leaderelection_manager) {
  id_ = id;
  total_num_ = total_num;
  f_ = (total_num-1)/2;
  is_stop_ = false;
  prevLogIndex_ = 0;
  currentTerm_ = 0;
  votedFor_ = -1;
  commitIndex_ = 0;
  lastApplied_ = 0;
  last_ae_time_ = std::chrono::steady_clock::now();
  last_heartbeat_time_ = std::chrono::steady_clock::now();

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

bool Raft::ReceiveTransaction(std::unique_ptr<AppendEntries> txn) {
  // LOG(INFO)<<"recv txn:";
  return false;
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
  
  Broadcast(MessageType::AppendEntriesMsg, *txn);
  return true;
}

bool Raft::ReceivePropose(std::unique_ptr<AppendEntries> txn) {
  if (txn->leaderid() == id_) { return false; }
  uint64_t term;
  bool success = false;
  bool demoted = false;
  TermRelation tr;
  Role initialRole;

  auto now = std::chrono::steady_clock::now();
  std::chrono::steady_clock::duration delta;
  delta = now - last_ae_time_;
  last_ae_time_ = now;
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(delta).count();
  LOG(INFO) << "JIM -> " << __FUNCTION__ << ": AE received after " << ms << "ms";
  {
    initialRole = role_;
    std::lock_guard<std::mutex> lk(mutex_);
    tr = TermCheckLocked(txn->term());
    if (tr == TermRelation::NEW) { demoted = DemoteSelfLocked(txn->term()); }
    else if (role_ == Role::CANDIDATE && tr == TermRelation::CURRENT) { demoted = DemoteSelfLocked(txn->term()); }
    if (tr != TermRelation::STALE) {
      uint64_t i = txn->prevlogindex();
      if (i < logIndexMapping_.size()) {
          const std::string& key = logIndexMapping_[i];
          if (txn->prevlogterm() == log_[key]->term()) { success = true; }
      }
    }
    term = currentTerm_;
  }
  if (demoted) { 
    leader_election_manager_->OnRoleChange();
    LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Demoted from " 
                << (initialRole == Role::LEADER ? "LEADER" : "CANDIDATE") << "->FOLLOWER in term " << term;
  }
  if (tr != TermRelation::STALE) { leader_election_manager_->OnHeartBeat(); }
    AppendEntriesResponse aer;
    aer.set_term(term);
    aer.set_success(success);
    if (success) { LOG(INFO) << "JIM -> " << __FUNCTION__ << ": responded success"; }
    else { LOG(INFO) << "JIM -> " << __FUNCTION__ << ": responded failure"; }
    SendMessage(MessageType::AppendEntriesResponseMsg, aer, txn->leaderid());
  return false;
}

bool Raft::ReceiveAppendEntriesResponse(std::unique_ptr<AppendEntriesResponse> response) {
uint64_t term;
bool demoted = false;
TermRelation tr;
Role initialRole;
{
  initialRole = role_;
  std::lock_guard<std::mutex> lk(mutex_);
  tr = TermCheckLocked(response->term());
  if (tr == TermRelation::NEW) { demoted = DemoteSelfLocked(response->term()); }
  term = currentTerm_;
}
  if (demoted) {
    leader_election_manager_->OnRoleChange();
    LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Demoted from " 
                << (initialRole == Role::LEADER ? "LEADER" : "CANDIDATE") << "->FOLLOWER in term " << term;
  }
  return false;
}


void Raft::ReceiveRequestVote(std::unique_ptr<RequestVote> rv) {
  int rvSender = rv->candidateid();
  uint64_t rvTerm = rv->term();

  uint64_t term;
  bool voteGranted = false;
  bool demoted = false;
  bool validCandidate = false;
  int votedFor = -1;
  Role initialRole;

  if (rvSender == id_) { return; }

  [&]() {
    std::lock_guard<std::mutex> lk(mutex_);
    initialRole = role_;
    // If their term is higher than ours, we accept new term, reset votedFor
    // and convert to follower
    TermRelation tr = TermCheckLocked(rvTerm);
    if (tr == TermRelation::STALE) {
      term = currentTerm_;
      return;
    }
    else if (tr == TermRelation::NEW) { demoted = DemoteSelfLocked(rvTerm); }
    // Then we continue voting process
    term = currentTerm_;
    votedFor = votedFor_;
    uint64_t lastLogTerm = getPrevLogTermLocked();
    //LOG(INFO) << "JIM -> " << __FUNCTION__ << ": prev terms at least equal";
    if (rv->lastlogterm() < lastLogTerm) { return; }
    //LOG(INFO) << "JIM -> " << __FUNCTION__ << ": prev log terms at least equal";
    if (rv->lastlogterm() == lastLogTerm 
        && rv->lastlogindex() < getPrevLogIndexLocked()) { return; }
    validCandidate = true;
    //LOG(INFO) << "JIM -> " << __FUNCTION__ << ": candidate is valid";
    if (votedFor_ == -1 || votedFor_ == rvSender) {
      votedFor_ = rvSender;
      voteGranted = true;
    }
  }();
  if (demoted) { 
    leader_election_manager_->OnRoleChange();
    LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Demoted from " 
              << (initialRole == Role::LEADER ? "LEADER" : "CANDIDATE") << "->FOLLOWER in term " << term;
  }
  if (voteGranted) {
    leader_election_manager_->OnHeartBeat(); 
    LOG(INFO) << "JIM -> " << __FUNCTION__ << ": voted for " << rvSender<< " in term " << term;
  }
  else if (validCandidate) { 
    LOG(INFO) << "JIM -> " << __FUNCTION__ << ": did not vote for "
              << rvSender<< " on term " << term << ". I already voted for " << votedFor
              << ((votedFor == id_) ? " (myself)" : "");
  }

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
  Role initialRole;

  [&]() {
    std::lock_guard<std::mutex> lk(mutex_);
    initialRole = role_;
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
    LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Replica " << voterId << " voted for me. Votes: " 
              << votes_.size() << "/" << votesNeeded << " in term " << currentTerm_;
    if (votes_.size() >= votesNeeded) {
      elected = true;
      role_ = Role::LEADER;
      LOG(INFO) << "JIM -> " << __FUNCTION__ << ": CANDIDATE->LEADER in term " << currentTerm_;
    }
  }();
    if (demoted || elected) { leader_election_manager_->OnRoleChange(); }
    if (demoted) {
      LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Demoted from " 
                << (initialRole == Role::LEADER ? "LEADER" : "CANDIDATE") << "->FOLLOWER in term " << term;
    }
    if (elected) { SendHeartBeat(); }
}

raft::Role Raft::GetRoleSnapshot() const {
  std::lock_guard<std::mutex> lk(mutex_);
  return role_;
}

// Called from LeaderElectionManager::StartElection when timeout
void Raft::StartElection() {
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
      role_ = raft::Role::CANDIDATE;
      roleChanged = true;
    }
    heartBeatsSentThisTerm_ = 0;
    currentTerm_++;
    votedFor_ = id_;
    votes_.clear();
    votes_.push_back(id_);
    LOG(INFO) << "JIM -> " << __FUNCTION__ << ": I voted for myself. Votes: " 
              << votes_.size() << "/" << (total_num_ - f_) << " in term " << currentTerm_;

    currentTerm = currentTerm_;
    candidateId = id_;
    lastLogIndex = getPrevLogIndexLocked();
    lastLogTerm = getPrevLogTermLocked();
  }
  if (roleChanged) {
    leader_election_manager_->OnRoleChange();
    LOG(INFO) << __FUNCTION__ << ": FOLLOWER->CANDIDATE in term " << currentTerm;
  }

  RequestVote requestVote;
  requestVote.set_term(currentTerm);
  requestVote.set_candidateid(candidateId);
  requestVote.set_lastlogindex(lastLogIndex);
  requestVote.set_lastlogterm(lastLogTerm);
  Broadcast(MessageType::RequestVoteMsg, requestVote);
}

// TODOjim
// ON MERGE FIX VALUES
void Raft::SendHeartBeat() {
  uint64_t currentTerm;
  int leaderId = id_;
  uint64_t prevLogIndex;
  uint64_t prevLogTerm;
  std::string entries;
  uint64_t leaderCommit;
  uint64_t heartBeatNum;

  auto now = std::chrono::steady_clock::now();
  std::chrono::steady_clock::duration delta;

  {
    std::lock_guard<std::mutex> lk(mutex_);
    if (role_ != raft::Role::LEADER) {
      LOG(WARNING) << __FUNCTION__ << ": Non-Leader tried to start HeartBeat";
      return;
    }
    heartBeatsSentThisTerm_++;
    heartBeatNum = heartBeatsSentThisTerm_;
    currentTerm = currentTerm_;
    prevLogIndex = getPrevLogIndexLocked();
    prevLogTerm = getPrevLogTermLocked();
    entries = "";
    leaderCommit = 0;  // TODO

    delta = now - last_heartbeat_time_;
    last_heartbeat_time_ = now;
  }
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(delta).count();
  LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Heartbeat sent after " << ms << "ms";
  AppendEntries appendEntries;
  appendEntries.set_term(currentTerm);
  appendEntries.set_leaderid(leaderId);
  appendEntries.set_prevlogindex(prevLogIndex);
  appendEntries.set_prevlogterm(prevLogTerm);
  appendEntries.set_entries(entries);
  appendEntries.set_leadercommitindex(leaderCommit); // TODO
  Broadcast(MessageType::AppendEntriesMsg, appendEntries);

  LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Heartbeat " << heartBeatNum << " for term " << currentTerm;
}

// requires raft mutex to be held
// returns true if demoted
bool Raft::DemoteSelfLocked(uint64_t term) {
  currentTerm_ = term;
  votedFor_ = -1;
  if (role_ != raft::Role::FOLLOWER) {
    role_ = raft::Role::FOLLOWER;
    //LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Demoted to FOLLOWER";
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

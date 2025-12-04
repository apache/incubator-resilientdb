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
  LeaderElectionManager* leaderelection_manager, ReplicaCommunicator* replica_communicator)
    : ProtocolBase(id, f, total_num), 
    role_(raft::Role::FOLLOWER),
    verifier_(verifier),
    leader_election_manager_(leaderelection_manager),
    replica_communicator_(replica_communicator) {
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

  nextIndex_.assign(total_num_ + 1, logIndexMapping_.size());
  matchIndex_.assign(total_num_ + 1, 0);
}

Raft::~Raft() { is_stop_ = true; }

bool Raft::IsStop() { return is_stop_; }

bool Raft::ReceiveTransaction(std::unique_ptr<AppendEntries> txn) {
  uint64_t term;
  uint64_t prevLogIndex;
  uint64_t prevLogTerm;
  uint64_t leaderCommit;
  AppendEntries ae;
  {
    std::lock_guard<std::mutex> lk(mutex_);
    if (role_ != Role::LEADER) {
      // Inform client proxy of new leader?
      // Redirect transaction to a known leader?
      LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Replica is not leader, returning early";
      return false; 
    }
      // prepare fields for appendEntries message
      term = currentTerm_;
      prevLogIndex = getPrevLogIndexLocked();
      prevLogTerm = getPrevLogTermLocked();
      leaderCommit = commitIndex_;

      // append new transaction to log
      txn->set_term(term);
      ae.CopyFrom(*txn);
      logIndexMapping_.push_back(txn->hash());
      log_[txn->hash()] = std::move(txn);
      prevLogIndex_++; // ??

      // durably store the new entry somehow
      // otherwise it is a safety violation to treat it as "appended"
      // should not be sending AEs before durable.
  }

  LOG(INFO) << "Received Transaction to primary id: " << id_;
  LOG(INFO) << "prevLogIndex: " << prevLogIndex;
  ae.set_create_time(GetCurrentTime());
  // This should be a term for each entry, but assuming no failure at first
  ae.set_leaderid(id_);
  ae.set_prevlogindex(prevLogIndex);
  ae.set_prevlogterm(prevLogTerm);
  ae.set_leadercommitindex(leaderCommit);
  
  // Broadcast probably shouldnt be happening here.
  // Ideally a loop is sending AEs to followers based on index feedback
  Broadcast(MessageType::AppendEntriesMsg, ae);
  return true;
}

bool Raft::ReceivePropose(std::unique_ptr<AppendEntries> ae) {
  if (ae->leaderid() == id_) { return false; }
  uint64_t term;
  bool success = false;
  bool demoted = false;
  TermRelation tr;
  Role initialRole;
  uint64_t prevLogIndex = ae->prevlogindex();
  auto leaderCommit = ae->leadercommitindex();
  auto leaderId = ae->leaderid();
  std::string hash = ae->hash();
  std::vector<AppendEntries*> eToApply;

  [&]() {
    std::lock_guard<std::mutex> lk(mutex_);
    initialRole = role_;
    tr = TermCheckLocked(ae->term());
    if (tr == TermRelation::NEW) { demoted = DemoteSelfLocked(ae->term()); }
    else if (role_ == Role::CANDIDATE && tr == TermRelation::CURRENT) { demoted = DemoteSelfLocked(ae->term()); }
    if (tr != TermRelation::STALE && role_ == Role::FOLLOWER) {
      uint64_t i = ae->prevlogindex();
      if (i < logIndexMapping_.size()) {
          const std::string& key = logIndexMapping_[i];
          if (ae->prevlogterm() == log_[key]->term()) { success = true; }
      }
    }
    term = currentTerm_;
    if (!success) { return; }

    // Only append entries to log if "entries" field is non-empty.
    // heartbeats contain a entry of "" (empty string),  they should not be appended

    // Non heartbeat case
    if (ae->entries() != "") {
      // TODO Implement an entry existing but with a different term
      // delete that entry and all after it
      logIndexMapping_.push_back(hash);
      log_[hash] = std::move(ae);
      LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Appended to log at index" << getPrevLogIndexLocked();

      // have to actually store the entry durably before it can be considered "appended"
    }
    // heartbeat case
    else {
      LOG(INFO) << "JIM -> " << __FUNCTION__ << ": This is a heartbeat, should not append";
    }
    // common case
    uint64_t prevCommitIndex = commitIndex_;
    if (leaderCommit > commitIndex_) {
       commitIndex_ = std::min(leaderCommit, getPrevLogIndexLocked());
       LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Raised commitIndex_ from "
                 << prevCommitIndex << " to " << commitIndex_;
    }

    // apply any newly committed entries to state machine
    while (commitIndex_ > lastApplied_) {
      lastApplied_++;
      eToApply.push_back(log_[logIndexMapping_[lastApplied_]].get());
      LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Applying index entry " << lastApplied_;
    }
  }();

  //auto now = std::chrono::steady_clock::now();
  //std::chrono::steady_clock::duration delta;
  //delta = now - last_ae_time_;
  //last_ae_time_ = now;
  //auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(delta).count();
  //LOG(INFO) << "JIM -> " << __FUNCTION__ << ": AE received after " << ms << "ms";

  if (demoted) {
    leader_election_manager_->OnRoleChange();
    LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Demoted from "
                << (initialRole == Role::LEADER ? "LEADER" : "CANDIDATE") << "->FOLLOWER in term " << term;
  }

  if (tr != TermRelation::STALE) { leader_election_manager_->OnHeartBeat(); }

  for (auto e : eToApply) {
    commit_(*e);
  }

  /*
  LOG(INFO) << "AppendEntriesMsg Added to Log";
  LOG(INFO) << "leaderCommit: " << leaderCommit;
  LOG(INFO) << "commitIndex_: " << commitIndex_;
  LOG(INFO) << "lastApplied_: " << lastApplied_;
  LOG(INFO) << "static_cast<int64_t>(log_.size()): " << static_cast<int64_t>(log_.size());
  LOG(INFO) << "leaderCommit > commitIndex_: " << (leaderCommit > commitIndex_ ? "true" : "false");
  LOG(INFO) << "lastApplied_ + 1 <= static_cast<int64_t>(log_.size()) " << ((lastApplied_ + 1 <= static_cast<int64_t>(log_.size())) ? "true" : "false");
  */

  AppendEntriesResponse aer;
  aer.set_term(term);
  aer.set_success(success);
  aer.set_id(id_);
  //aer.set_nextentry(log_.size());
  aer.set_hash(hash); // this seems odd
  aer.set_prevlogindex(prevLogIndex);
  SendMessage(MessageType::AppendEntriesResponseMsg, aer, leaderId);
  if (success) { LOG(INFO) << "JIM -> " << __FUNCTION__ << ": responded success"; }
  else { LOG(INFO) << "JIM -> " << __FUNCTION__ << ": responded failure"; }
  return true;
}

bool Raft::ReceiveAppendEntriesResponse(std::unique_ptr<AppendEntriesResponse> aer) {
uint64_t term;
bool demoted = false;
TermRelation tr;
Role initialRole;
  {
    std::lock_guard<std::mutex> lk(mutex_);
    initialRole = role_;
    tr = TermCheckLocked(aer->term());
    if (tr == TermRelation::NEW) { demoted = DemoteSelfLocked(aer->term()); }
    term = currentTerm_;
  }
  if (demoted) {
    leader_election_manager_->OnRoleChange();
    LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Demoted from " 
                << (initialRole == Role::LEADER ? "LEADER" : "CANDIDATE") << "->FOLLOWER in term " << term;
    return false;
  }
  // ===================== SUCCESS CASE =====================
  else if (aer->success()) {
    {
      std::unique_lock<std::mutex> lk(mutex_);
      received_[aer->hash()].insert(aer->id());
      auto it = log_.find(aer->hash());
      if (it != log_.end()) {
        LOG(INFO) << "Transaction: " << aer->prevlogindex()
                  << " has gotten " << received_[aer->hash()].size()
                  << " responses";
        if (static_cast<uint64_t>(received_[aer->hash()].size()) >= static_cast<uint64_t>(total_num_ - f_)) {
          commitIndex_ = aer->prevlogindex();

          // pretty sure this should always be in order with no gaps
          while (lastApplied_ + 1 <= static_cast<uint64_t>(log_.size()) &&
                 lastApplied_ <= commitIndex_) {
            LOG(INFO) << "Leader Committing";
            std::unique_ptr<AppendEntries> txnToCommit = nullptr;
            txnToCommit = std::move(log_[logIndexMapping_[lastApplied_]]); // suspicious?
            commit_(*txnToCommit);
            lastApplied_++;
          }
        }
      }
    }
    nextIndex_[aer->id()] = aer->nextentry();
    matchIndex_[aer->id()] = aer->lastapplied();
    return true;
  }

  // ===================== FAILURE CASE =====================
  else if (!aer->success()) {
    LOG(INFO) << "AppendEntriesResponse indicates FAILURE from follower " << aer->id();
    {
      std::unique_lock<std::mutex> lk(mutex_);

      // Move nextIndex one step back, but don't go below 1
      if (nextIndex_[aer->id()] > 1) {
        --nextIndex_[aer->id()];
      } else {
        nextIndex_[aer->id()] = 1;
      }

      uint64_t resendIndex = nextIndex_[aer->id()];
      LOG(INFO) << "Updated nextIndex_ for follower " << aer->id()
                << " to " << resendIndex;

      // Check that we actually have an entry at this index
      if (resendIndex < 0 ||
          resendIndex >= static_cast<uint64_t>(logIndexMapping_.size())) { // check for off by 1
        LOG(INFO) << "No log entry at index " << resendIndex
                  << " to resend; logIndexMapping_.size() = "
                  << logIndexMapping_.size();
        return false;
      }

      const std::string& key = logIndexMapping_[resendIndex];
      auto it = log_.find(key);
      if (it == log_.end() || !it->second) {
        LOG(WARNING) << "Log entry missing in map for key at index "
                    << resendIndex;
        return false;
      }

      // Build a new AppendEntries message based on the stored log entry
      AppendEntries resend;
      resend.CopyFrom(*(it->second));  // copies hash, entries, uid, proxy_id, etc.

      // Make sure RAFT fields are consistent with our current state
      //resend.set_term(currentTerm_);
      resend.set_leaderid(id_);

      // prevLogIndex = index immediately before resendIndex
      uint64_t prevIdx = (resendIndex == 0 ? 0 : resendIndex - 1);
      resend.set_prevlogindex(prevIdx);

      // prevLogTerm = term of the entry at prevIdx (or 0 if none)
      uint64_t prevTerm = 0;
      if (prevIdx >= 0 && prevIdx < static_cast<uint64_t>(logIndexMapping_.size())) { // off by 1 check
        const std::string& prevKey = logIndexMapping_[prevIdx];
        auto itPrev = log_.find(prevKey);
        if (itPrev != log_.end() && itPrev->second) {
          prevTerm = itPrev->second->term();
        }
      }
      resend.set_prevlogterm(prevTerm);

      // leaderCommitIndex
      resend.set_leadercommitindex(commitIndex_);

      LOG(INFO) << "Resending AppendEntries for index " << resendIndex
                << " (prevIdx=" << prevIdx
                << ", prevTerm=" << prevTerm
                << ") to follower " << aer->id();

      SendMessage(MessageType::AppendEntriesMsg, resend, aer->id());
    }

    return true;
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
  uint64_t votesNeeded = static_cast<uint64_t>(total_num_ - f_);
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
      nextIndex_.assign(total_num_ + 1, logIndexMapping_.size());
      matchIndex_.assign(total_num_ + 1, 0);
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
  //uint64_t heartBeatNum;

  //auto now = std::chrono::steady_clock::now();
  //std::chrono::steady_clock::duration delta;

  {
    std::lock_guard<std::mutex> lk(mutex_);
    if (role_ != raft::Role::LEADER) {
      LOG(WARNING) << __FUNCTION__ << ": Non-Leader tried to start HeartBeat";
      return;
    }
    //heartBeatsSentThisTerm_++;
    //heartBeatNum = heartBeatsSentThisTerm_;
    currentTerm = currentTerm_;
    prevLogIndex = getPrevLogIndexLocked();
    prevLogTerm = getPrevLogTermLocked();
    entries = "";
    leaderCommit = 0;  // TODO

  //  delta = now - last_heartbeat_time_;
  //  last_heartbeat_time_ = now;
  }
  //auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(delta).count();
  //LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Heartbeat sent after " << ms << "ms";
  AppendEntries appendEntries;
  appendEntries.set_term(currentTerm);
  appendEntries.set_leaderid(leaderId);
  appendEntries.set_prevlogindex(prevLogIndex);
  appendEntries.set_prevlogterm(prevLogTerm);
  appendEntries.set_entries(entries);
  appendEntries.set_leadercommitindex(leaderCommit);
  Broadcast(MessageType::AppendEntriesMsg, appendEntries);

  //LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Heartbeat " << heartBeatNum << " for term " << currentTerm;

  // Also ping client proxies that this is the leader
  DirectToLeader dtl;
  dtl.set_term(currentTerm);
  dtl.set_leaderid(id_);
  for (const auto& client : replica_communicator_->GetClientReplicas()) {
    int id = client.id();
    SendMessage(DirectToLeaderMsg, dtl, id);
    //LOG(INFO) << "JIM -> " << __FUNCTION__ << ": DirectToLeader " << id_ << " sent to proxy " << id;
  }
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
// TODO Reuse for other cases?
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

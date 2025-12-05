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
    currentTerm_(0),
    votedFor_(-1),
    lastLogIndex_(0),
    commitIndex_(0),
    lastApplied_(0),
    role_(raft::Role::FOLLOWER),
    is_stop_(false),
    quorum_((total_num/2) + 1),
    verifier_(verifier),
    leader_election_manager_(leaderelection_manager),
    replica_communicator_(replica_communicator) {
  
  id_ = id;
  total_num_ = total_num;
  f_ = (total_num-1)/2;
  last_ae_time_ = std::chrono::steady_clock::now();
  last_heartbeat_time_ = std::chrono::steady_clock::now();

  auto sentinel = std::make_unique<LogEntry>();
  sentinel->term = 0;
  sentinel->command = "COMMON_PREFIX";
  log_.push_back(std::move(sentinel));

  nextIndex_.assign(total_num_ + 1, lastLogIndex_ + 1);
  matchIndex_.assign(total_num_ + 1, lastLogIndex_);
}

Raft::~Raft() { is_stop_ = true; }

bool Raft::IsStop() { return is_stop_; }

void Raft::CreateAndSendAppendEntries(std::vector<uint64_t> nextIndexCopy, uint64_t term, uint64_t prevLogTerm, uint64_t leaderCommit, std::string cmd) {
  for (int i = 1; i <= total_num_; ++i) {
    AppendEntries ae;
    ae.set_term(term);
    ae.set_leaderid(id_);
    ae.set_prevlogindex(nextIndexCopy[i] - 1);
    ae.set_prevlogterm(prevLogTerm);
    auto* e = ae.add_entries();
    e->set_term(term);
    e->set_command(cmd); 
    ae.set_leadercommitindex(leaderCommit); 
    SendMessage(MessageType::AppendEntriesMsg, ae, i);
  }
}

bool Raft::ReceiveTransaction(std::unique_ptr<Request> req) {
  uint64_t term;
  uint64_t prevLogIndex;
  uint64_t prevLogTerm;
  uint64_t leaderCommit;
  std::string cmd;
  std::vector<uint64_t> nextIndexCopy = nextIndex_;
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
      prevLogIndex = lastLogIndex_;
      prevLogTerm = getLastLogTermLocked();
      leaderCommit = commitIndex_;

      // assign seq number as log index for the request or executing transactions fails.
      req->set_seq(lastLogIndex_ + 1);

      // append new transaction to log
      auto entry = std::make_unique<LogEntry>();
      entry->term = currentTerm_;
      if (!req->SerializeToString(&entry->command)) {
        LOG(INFO) << "JIM -> " << __FUNCTION__ << ": req could not be serialized";
        return false;
      }
      cmd = entry->command;
      log_.push_back(std::move(entry));

      // TODO
      // durably store the new entry somehow
      // otherwise it is a safety violation to treat it as "appended"
      // should not be sending AEs before durable.

      lastLogIndex_++;
      nextIndex_[id_] = lastLogIndex_ + 1;
      matchIndex_[id_] = lastLogIndex_;
      nextIndexCopy[id_] = nextIndex_[id_];
  }

  //LOG(INFO) << "Received Transaction to primary id: " << id_;
  // Broadcast probably shouldnt be happening here.
  // Ideally a loop is sending AEs to followers based on index feedback
  // Broadcast(MessageType::AppendEntriesMsg, ae);
  CreateAndSendAppendEntries(nextIndexCopy, term, prevLogTerm, leaderCommit, cmd);
  return true;
}

bool Raft::ReceiveAppendEntries(std::unique_ptr<AppendEntries> ae) {
  if (ae->leaderid() == id_) { return false; }
  uint64_t term;
  bool success = false;
  bool demoted = false;
  TermRelation tr;
  Role initialRole;
  uint64_t lastLogIndex;
  auto leaderCommit = ae->leadercommitindex();
  auto leaderId = ae->leaderid();
  std::vector<std::unique_ptr<Request>> eToApply;

  [&]() {
    std::lock_guard<std::mutex> lk(mutex_);
    initialRole = role_;
    lastLogIndex = lastLogIndex_;
    tr = TermCheckLocked(ae->term());
    if (tr == TermRelation::NEW) { demoted = DemoteSelfLocked(ae->term()); }
    else if (role_ != Role::FOLLOWER && tr == TermRelation::CURRENT) { demoted = DemoteSelfLocked(ae->term()); }
    
    if (tr != TermRelation::STALE && role_ == Role::FOLLOWER) {
      uint64_t i = ae->prevlogindex();
      if (i < static_cast<uint64_t>(log_.size()) && ae->prevlogterm() == log_[i]->term) { success = true; }
    }
    term = currentTerm_;
    if (!success) { return; }
      // TODO Implement an entry existing but with a different term
      // delete that entry and all after it

    for (const auto& e : ae->entries()) {
      auto entry = std::make_unique<LogEntry>();
      entry->term = e.term();
      entry->command = e.command();
      log_.push_back(std::move(entry));
      lastLogIndex_++;
      //LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Appended to log at index" << lastLogIndex_;
      // TODO: have to actually store the entry durably before it can be considered "appended"
    }
    lastLogIndex = lastLogIndex_;
    
    //uint64_t prevCommitIndex = commitIndex_;
    if (leaderCommit > commitIndex_) {
       commitIndex_ = std::min(leaderCommit, lastLogIndex_);
       //LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Raised commitIndex_ from "
       //          << prevCommitIndex << " to " << commitIndex_;
    }

    // apply any newly committed entries to state machine
    eToApply = PrepareCommitLocked();
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

  for (auto& e : eToApply) {
    commit_(*e);
  }

  AppendEntriesResponse aer;
  aer.set_term(term);
  aer.set_success(success);
  aer.set_id(id_);
  aer.set_lastlogindex(lastLogIndex);
  SendMessage(MessageType::AppendEntriesResponseMsg, aer, leaderId);
  //if (success) { LOG(INFO) << "JIM -> " << __FUNCTION__ << ": responded success"; }
  //else { LOG(INFO) << "JIM -> " << __FUNCTION__ << ": responded failure"; }
  return true;
}

bool Raft::ReceiveAppendEntriesResponse(std::unique_ptr<AppendEntriesResponse> aer) {
  uint64_t term;
  bool demoted = false;
  bool resending = false;
  TermRelation tr;
  Role initialRole;
  std::vector<std::unique_ptr<Request>> eToApply;
  AppendEntries resend;

  [&]() {
    std::lock_guard<std::mutex> lk(mutex_);
    initialRole = role_;
    tr = TermCheckLocked(aer->term());
    if (tr == TermRelation::NEW) { demoted = DemoteSelfLocked(aer->term()); }
    term = currentTerm_;

    if (role_ != Role::LEADER || tr == TermRelation::STALE) { return; }

    // ===================== SUCCESS CASE =====================
    if (aer->success()) {
      nextIndex_[aer->id()] = aer->lastlogindex() + 1;

      // need to ensure matchIndex never decreases even if followers lastLogIndex decreases
      matchIndex_[aer->id()] = std::max(matchIndex_[aer->id()], aer->lastlogindex());
      
      // use updated matchIndex to find new entries eligible for commit
      std::vector<uint64_t> sorted = matchIndex_;
      std::sort(sorted.begin(), sorted.end(), std::greater<uint64_t>());
      uint64_t lastReplicatedIndex = sorted[quorum_ - 1];

      // Need to check the lastReplicatedIndex contains entry from current term
      if (lastReplicatedIndex > commitIndex_ && log_[lastReplicatedIndex]->term == currentTerm_) {
        //LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Raised commitIndex_ from "
        //         << commitIndex_ << " to " << lastReplicatedIndex;
        commitIndex_ = lastReplicatedIndex;
      }

      // apply any newly committed entries to state machine
      eToApply = PrepareCommitLocked();
    }
    // ===================== FAILURE CASE =====================
    else {
      LOG(INFO) << "AppendEntriesResponse indicates FAILURE from follower " << aer->id();
      // Move nextIndex one step back, but don't go below 1
      if (nextIndex_[aer->id()] > 1) {
        --nextIndex_[aer->id()];
      } else {
        nextIndex_[aer->id()] = 1;
      }
      uint64_t resendIndex = nextIndex_[aer->id()];
      //LOG(INFO) << "Updated nextIndex_ for follower " << aer->id()
      //          << " to " << resendIndex;

      // Check that we actually have an entry at this index
      if (resendIndex == 0 || resendIndex > lastLogIndex_) {
        LOG(INFO) << "No log entry at index " << resendIndex
                  << " to resend; lastLogIndex_ = "
                  << lastLogIndex_;
        return;
      }
      uint64_t prevIdx = resendIndex - 1;
      uint64_t prevTerm = log_[prevIdx]->term;
      const LogEntry& resendEntry = *log_[resendIndex];
      resend.set_term(currentTerm_);
      resend.set_leaderid(id_);
      resend.set_prevlogindex(prevIdx);
      resend.set_prevlogterm(prevTerm);
      auto* e = resend.add_entries();
      e->set_term(resendEntry.term);
      e->set_command(resendEntry.command); 
      resend.set_leadercommitindex(commitIndex_);
      resending = true;

    //  LOG(INFO) << "Resending AppendEntries for index " << resendIndex
    //            << " (prevIdx=" << prevIdx
    //            << ", prevTerm=" << prevTerm
    //            << ") to follower " << aer->id();
    }
  }();
  if (demoted) {
    leader_election_manager_->OnRoleChange();
    LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Demoted from " 
                << (initialRole == Role::LEADER ? "LEADER" : "CANDIDATE") << "->FOLLOWER in term " << term;
    return false;
  }
  if (resending) { SendMessage(MessageType::AppendEntriesMsg, resend, aer->id()); }

  for (auto& e : eToApply) {
    commit_(*e);
  }
  return true;
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
    uint64_t lastLogTerm = getLastLogTermLocked();
    if (rv->lastlogterm() < lastLogTerm) { return; }
    if (rv->lastlogterm() == lastLogTerm 
        && rv->lastlogindex() < lastLogIndex_) { return; }
    validCandidate = true;
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
              << votes_.size() << "/" << quorum_ << " in term " << currentTerm_;
    if (votes_.size() >= quorum_) {
      elected = true;
      role_ = Role::LEADER;
      nextIndex_.assign(total_num_ + 1, lastLogIndex_ + 1);

      // make sure to set leaders own matchIndex entry to lastLogIndex
      matchIndex_.assign(total_num_ + 1, 0);
      matchIndex_[id_] = lastLogIndex_;
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
              << votes_.size() << "/" << quorum_ << " in term " << currentTerm_;

    currentTerm = currentTerm_;
    candidateId = id_;
    lastLogIndex = lastLogIndex_;
    lastLogTerm = getLastLogTermLocked();
  }
  if (roleChanged) {
    leader_election_manager_->OnRoleChange();
    LOG(INFO) << __FUNCTION__ << ": FOLLOWER->CANDIDATE in term " << currentTerm;
  }

  RequestVote rv;
  rv.set_term(currentTerm);
  rv.set_candidateid(candidateId);
  rv.set_lastlogindex(lastLogIndex);
  rv.set_lastlogterm(lastLogTerm);
  Broadcast(MessageType::RequestVoteMsg, rv);
}

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
    prevLogIndex = lastLogIndex_;
    prevLogTerm = getLastLogTermLocked();
    entries = "";
    leaderCommit = commitIndex_;

  //  delta = now - last_heartbeat_time_;
  //  last_heartbeat_time_ = now;
  }
  //auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(delta).count();
  //LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Heartbeat sent after " << ms << "ms";
  AppendEntries ae;
  ae.set_term(currentTerm);
  ae.set_leaderid(leaderId);
  ae.set_prevlogindex(prevLogIndex); // TODO
  ae.set_prevlogterm(prevLogTerm);
  ae.set_leadercommitindex(leaderCommit);
  Broadcast(MessageType::AppendEntriesMsg, ae);

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
  if (term > currentTerm_) {
    currentTerm_ = term;
    votedFor_ = -1;
  }
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
uint64_t Raft::getLastLogTermLocked() const {
  return log_[lastLogIndex_]->term;
}

std::vector<std::unique_ptr<Request>> Raft::PrepareCommitLocked() {
  std::vector<std::unique_ptr<Request>> v;
  while (commitIndex_ > lastApplied_) {
      lastApplied_++;
      auto command = std::make_unique<Request>();
      if (!command->ParseFromString(log_[lastApplied_]->command)) {
        LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Failed to parse command";
        continue;
      }
      v.push_back(std::move(command));
      //LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Applying index entry " << lastApplied_;
    }
  return v;
}



}  // namespace raft
}  // namespace resdb

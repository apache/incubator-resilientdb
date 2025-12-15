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
    role_(Role::FOLLOWER),
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

  inflight_.assign(total_num_ + 1, 0);
  nextIndex_.assign(total_num_ + 1, lastLogIndex_ + 1);
  matchIndex_.assign(total_num_ + 1, lastLogIndex_);
}

Raft::~Raft() { 
  is_stop_ = true;
}

bool Raft::IsStop() { 
  return is_stop_; 
}

bool Raft::ReceiveTransaction(std::unique_ptr<Request> req) {
  std::vector<AeFields> messages;
  {
    std::lock_guard<std::mutex> lk(mutex_);
    if (role_ != Role::LEADER) {
      // Inform client proxy of new leader?
      // Redirect transaction to a known leader?
      LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Replica is not leader, returning early";
      return false;
    }
      // append new transaction to log
      auto entry = std::make_unique<LogEntry>();
      entry->term = currentTerm_;
      if (!req->SerializeToString(&entry->command)) {
        LOG(INFO) << "JIM -> " << __FUNCTION__ << ": req could not be serialized";
        return false;
      }
      log_.push_back(std::move(entry));
      


      // TODO
      // durably store the new entry somehow
      // otherwise it is a safety violation to treat it as "appended"
      // should not be responding to RPCs before durable.

      lastLogIndex_++;
      nextIndex_[id_] = lastLogIndex_ + 1;
      matchIndex_[id_] = lastLogIndex_;

      if (replicationLoggingFlag_) {
        LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Leader appended entry at index " << lastLogIndex_;
      }

      // prepare fields for appendEntries message
      messages = GatherAeFieldsForBroadcastLocked();
  }
  for (const auto& msg : messages) {
      CreateAndSendAppendEntryMsg(msg);
  }
  leader_election_manager_->OnAeBroadcast();
  return true;
}

bool Raft::ReceiveAppendEntries(std::unique_ptr<AppendEntries> ae) {
  if (ae->leaderid() == id_) { 
    return false;
  }
  uint64_t term;
  bool success = false;
  bool demoted = false;
  TermRelation tr;
  Role initialRole;
  uint64_t lastLogIndex;
  auto leaderCommit = ae->leadercommitindex();
  auto leaderId = ae->leaderid();
  std::vector<std::unique_ptr<Request>> eToApply;

  const char* parent_fn = __FUNCTION__;
  [&]() {
    std::lock_guard<std::mutex> lk(mutex_);
    initialRole = role_;
    lastLogIndex = lastLogIndex_;
    tr = TermCheckLocked(ae->term());
    if (tr == TermRelation::NEW) {
      demoted = DemoteSelfLocked(ae->term());
    }
    else if (role_ != Role::FOLLOWER && tr == TermRelation::CURRENT) {
      demoted = DemoteSelfLocked(ae->term());
    }
    
    if (tr != TermRelation::STALE && role_ == Role::FOLLOWER) {
      uint64_t i = ae->prevlogindex();
      if (i < static_cast<uint64_t>(log_.size()) && ae->prevlogterm() == log_[i]->term) {
        success = true; 
      }
    }
    term = currentTerm_;
    if (!success) {
      return;
    }


    /*
    new logic concept:
    rather than checking idx > lastLogIndex_, check idx == lastLogIndex_ + 1 (should be equivalent, has semantic value)

    First, loop over entries before lastLogIndex + 1 and look for conflicts.
    If conflict occurs, wipe suffix and set LastLogIndex = idx
    If idx already = lastLogIndex_ + 1, this loop is skipped

    Second, batch append all remaining entries to log
    */

    uint64_t idx = ae->prevlogindex() + 1;
    for (const auto& entry : ae->entries()) {
      auto newEntry = std::make_unique<LogEntry>();
      newEntry->term = entry.term();
      newEntry->command = entry.command();

      // entry is at new position
      if (idx > lastLogIndex_) {
        log_.push_back(std::move(newEntry));
        lastLogIndex_ = idx;

        if (replicationLoggingFlag_) {
          LOG(INFO) << "JIM -> " << parent_fn << ": follower appended new entry at index " << lastLogIndex_;
        }

      }
      // entry is at an existing position && new term doesnt match old term
      else if (newEntry->term != log_[idx]->term) {
        auto first = log_.begin() + idx;
        auto last = log_.begin() + lastLogIndex_ + 1;
        log_.erase(first, last);
        log_.push_back(std::move(newEntry));
        lastLogIndex_ = idx;

        if (replicationLoggingFlag_) {
          LOG(INFO) << "JIM -> " << parent_fn << ": follower saw term mismatch at index " << lastLogIndex_ << ". Later entries erased";
        }

      }
      ++idx;
      // TODO: have to actually store the entry durably before it can be considered "appended"
    }
    lastLogIndex = lastLogIndex_;
    
    uint64_t prevCommitIndex = commitIndex_;
    if (leaderCommit > commitIndex_) {
       commitIndex_ = std::min(leaderCommit, lastLogIndex_);

      if (replicationLoggingFlag_ && commitIndex_ > prevCommitIndex) {
        LOG(INFO) << "JIM -> " << parent_fn << ": Raised commitIndex_ from "
                  << prevCommitIndex << " to " << commitIndex_;
      }

    }

    // apply any newly committed entries to state machine
    eToApply = PrepareCommitLocked();
  }();

  auto now = std::chrono::steady_clock::now();
  std::chrono::steady_clock::duration delta;
  delta = now - last_ae_time_;
  last_ae_time_ = now;
  

  if (replicationLoggingFlag_) {
    /*
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(delta).count();
    LOG(INFO) << "JIM -> " << __FUNCTION__ << ": AE received after " << ms << "ms";
    */
  }

  if (demoted) {
    leader_election_manager_->OnRoleChange();
    LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Demoted from "
                << (initialRole == Role::LEADER ? "LEADER" : "CANDIDATE") << "->FOLLOWER in term " << term;
  }

  if (tr != TermRelation::STALE) {
    leader_election_manager_->OnHeartBeat();
  }

  for (auto& entry : eToApply) {
    commit_(*entry);
  }

  AppendEntriesResponse aer;
  aer.set_term(term);
  aer.set_success(success);
  aer.set_id(id_);
  aer.set_lastlogindex(lastLogIndex);
  SendMessage(MessageType::AppendEntriesResponseMsg, aer, leaderId);

  if (replicationLoggingFlag_) {
    /*
    if (success) {
      LOG(INFO) << "JIM -> " << __FUNCTION__ << ": responded success";
    }
    else {
      LOG(INFO) << "JIM -> " << __FUNCTION__ << ": responded failure";
    }
    */
  }
  return true;
}

bool Raft::ReceiveAppendEntriesResponse(std::unique_ptr<AppendEntriesResponse> aer) {
  uint64_t term;
  bool demoted = false;
  bool resending = false;
  TermRelation tr;
  Role initialRole;
  std::vector<std::unique_ptr<Request>> eToApply;
  AeFields fields;
  int followerId = aer->id();
  const char* parent_fn = __FUNCTION__;
  [&]() {
    std::lock_guard<std::mutex> lk(mutex_);
    initialRole = role_;
    tr = TermCheckLocked(aer->term());
    if (tr == TermRelation::NEW) {
      demoted = DemoteSelfLocked(aer->term());
    }
    term = currentTerm_;

    if (role_ != Role::LEADER || tr == TermRelation::STALE) {
      return;
    }

    nextIndex_[followerId] = aer->lastlogindex() + 1;

    // if successful, update matchIndex and try to commit more entries
    if (aer->success()) {
      // need to ensure matchIndex never decreases even if followers lastLogIndex decreases
      matchIndex_[followerId] = std::max(matchIndex_[followerId], aer->lastlogindex());
      // use updated matchIndex to find new entries eligible for commit
      std::vector<uint64_t> sorted = matchIndex_;
      std::sort(sorted.begin(), sorted.end(), std::greater<uint64_t>());
      uint64_t lastReplicatedIndex = sorted[quorum_ - 1];
      // Need to check the lastReplicatedIndex contains entry from current term
      if (lastReplicatedIndex > commitIndex_ && log_[lastReplicatedIndex]->term == currentTerm_) {
        LOG(INFO) << "JIM -> " << parent_fn << ": Raised commitIndex_ from "
                 << commitIndex_ << " to " << lastReplicatedIndex;
        commitIndex_ = lastReplicatedIndex;
      }
      // apply any newly committed entries to state machine
      eToApply = PrepareCommitLocked();
    }
    // if failure, or if nextIndex[i] < lastLogIndex + 1 (follower isnt caught up)
    if (!aer->success() || (nextIndex_[followerId] < lastLogIndex_ + 1)) {
      if (!aer->success()) {
        LOG(INFO) << "AppendEntriesResponse indicates FAILURE from follower " << followerId;
      }
      fields = GatherAeFieldsLocked(followerId);
      resending = true;
    }
  }();
  if (demoted) {
    leader_election_manager_->OnRoleChange();
    LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Demoted from " 
                << (initialRole == Role::LEADER ? "LEADER" : "CANDIDATE") << "->FOLLOWER in term " << term;
    return false;
  }
  if (resending) {
    CreateAndSendAppendEntryMsg(fields);
  }

  for (auto& entry : eToApply) {
    commit_(*entry);
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

  if (rvSender == id_) {
    return;
  }

  //const char* parent_fn = __FUNCTION__;
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
    else if (tr == TermRelation::NEW) {
      demoted = DemoteSelfLocked(rvTerm);
    }
    // Then we continue voting process
    term = currentTerm_;
    votedFor = votedFor_;
    uint64_t lastLogTerm = getLastLogTermLocked();
    if (rv->lastlogterm() < lastLogTerm) {
      return;
    }
    if (rv->lastlogterm() == lastLogTerm && rv->lastlogindex() < lastLogIndex_) {
      return;
    }
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

  const char* parent_fn = __FUNCTION__;
  [&]() {
    std::lock_guard<std::mutex> lk(mutex_);
    initialRole = role_;
    TermRelation tr = TermCheckLocked(term);
    if (tr == TermRelation::STALE) {
      return;
    }
    else if (tr == TermRelation::NEW) { 
      demoted = DemoteSelfLocked(term);
      return;
    }
    if (role_ != Role::CANDIDATE) {
      return;
    }
    if (!votedYes) {
      return;
    }
    bool dupe = (std::find(votes_.begin(), votes_.end(), voterId) != votes_.end());
    if (dupe) {
      return;
    }
    votes_.push_back(voterId);
    LOG(INFO) << "JIM -> " << parent_fn << ": Replica " << voterId << " voted for me. Votes: " 
              << votes_.size() << "/" << quorum_ << " in term " << currentTerm_;
    if (votes_.size() >= quorum_) {
      elected = true;
      role_ = Role::LEADER;
      inflight_.assign(total_num_ + 1, 0);
      nextIndex_.assign(total_num_ + 1, lastLogIndex_ + 1);

      // make sure to set leaders own matchIndex entry to lastLogIndex
      matchIndex_.assign(total_num_ + 1, 0);
      matchIndex_[id_] = lastLogIndex_;
      LOG(INFO) << "JIM -> " << parent_fn << ": CANDIDATE->LEADER in term " << currentTerm_;
    }
  }();
    if (demoted || elected) {
      leader_election_manager_->OnRoleChange();
    }
    if (demoted) {
      LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Demoted from " 
                << (initialRole == Role::LEADER ? "LEADER" : "CANDIDATE") << "->FOLLOWER in term " << term;
    }
    if (elected) {
      SendHeartBeat();
    }
}

Role Raft::GetRoleSnapshot() const {
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
    if (role_ == Role::LEADER) {
      LOG(WARNING) << __FUNCTION__ << ": Leader tried to start election";
      return;
    }
    if (role_ == Role::FOLLOWER) {
      role_ = Role::CANDIDATE;
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
  auto functionStart = std::chrono::steady_clock::now();
  std::chrono::steady_clock::duration functionDelta;

  std::vector<AeFields> messages;
  uint64_t currentTerm;
  uint64_t heartBeatNum;
  {
    std::lock_guard<std::mutex> lk(mutex_);
    if (role_ != Role::LEADER) {
      LOG(WARNING) << __FUNCTION__ << ": Non-Leader tried to start HeartBeat";
      return;
    }
    currentTerm = currentTerm_;
    
    heartBeatsSentThisTerm_++;
    heartBeatNum = heartBeatsSentThisTerm_;
    bool heartbeat = true;
    messages = GatherAeFieldsForBroadcastLocked(heartbeat);
  }

  auto msgStart = std::chrono::steady_clock::now();
  std::chrono::steady_clock::duration msgDelta;
  
  for (const auto& msg : messages) {
    CreateAndSendAppendEntryMsg(msg);
  }
  
  auto msgEnd = std::chrono::steady_clock::now();
  msgDelta = msgEnd - msgStart;
  auto msgMs = std::chrono::duration_cast<std::chrono::milliseconds>(msgDelta).count();

  if (livenessLoggingFlag_) {
    LOG(INFO) << "JIM -> " << __FUNCTION__ << ": " << msgMs << " ms elapsed in CreateAndSend loop";
    LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Heartbeat " << heartBeatNum << " for term " << currentTerm;
  }
  
  
  auto redirectStart = std::chrono::steady_clock::now();
  std::chrono::steady_clock::duration redirectDelta;
  
  // Also ping client proxies that this is the leader
  DirectToLeader dtl;
  dtl.set_term(currentTerm);
  dtl.set_leaderid(id_);
  for (const auto& client : replica_communicator_->GetClientReplicas()) {
    int id = client.id();
    SendMessage(DirectToLeaderMsg, dtl, id);
    //LOG(INFO) << "JIM -> " << __FUNCTION__ << ": DirectToLeader " << id_ << " sent to proxy " << id;
  }
  
  auto redirectEnd = std::chrono::steady_clock::now();
  redirectDelta = redirectEnd - redirectStart;
  auto redirectMs = std::chrono::duration_cast<std::chrono::milliseconds>(redirectDelta).count();
  
  
  auto functionEnd = std::chrono::steady_clock::now();
  functionDelta = functionEnd - functionStart;
  auto functionMs = std::chrono::duration_cast<std::chrono::milliseconds>(functionDelta).count();

  if (livenessLoggingFlag_) {
    LOG(INFO) << "JIM -> " << __FUNCTION__ << ": " << redirectMs << " ms elapsed in redirect loop";
    LOG(INFO) << "JIM -> " << __FUNCTION__ << ": " << functionMs << " ms elapsed in function";
  }
}

// requires raft mutex to be held
// returns true if demoted
bool Raft::DemoteSelfLocked(uint64_t term) {
  if (term > currentTerm_) {
    currentTerm_ = term;
    votedFor_ = -1;
  }
  if (role_ != Role::FOLLOWER) {
    role_ = Role::FOLLOWER;
    //LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Demoted to FOLLOWER";
    return true;
  }
  return false;
}

// requires raft mutex to be held
TermRelation Raft::TermCheckLocked(uint64_t term) const {
  if (term < currentTerm_) {
    return TermRelation::STALE;
  }
  else if (term == currentTerm_) {
    return TermRelation::CURRENT;
  }
  else {
    return TermRelation::NEW;
  }
}

// requires raft mutex to be held
uint64_t Raft::getLastLogTermLocked() const {
  return log_[lastLogIndex_]->term;
}

// requires raft mutex to be held
std::vector<std::unique_ptr<Request>> Raft::PrepareCommitLocked() {
  std::vector<std::unique_ptr<Request>> commitVec;
  uint64_t begin = lastApplied_ + 1;
  bool applying = false;
  while (lastApplied_ < commitIndex_) {
    ++lastApplied_;
    auto command = std::make_unique<Request>();
    if (!command->ParseFromString(log_[lastApplied_]->command)) {
      LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Failed to parse command";
      continue;
    }
    // assign seq number as log index for the request or executing transactions fails.
    command->set_seq(lastApplied_);
    commitVec.push_back(std::move(command));
    applying = true;
  }

  if (applying && replicationLoggingFlag_) {
      if (lastApplied_ > begin) {
        LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Applying index entries " << begin << " to " << lastApplied_;
      }
      else {
        LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Applying index entry " << lastApplied_;
      }
  }

  return commitVec;
}

AeFields Raft::GatherAeFieldsLocked(int followerId, bool heartBeat) const {
  AeFields fields{};
  fields.term = currentTerm_;
  fields.leaderId = id_;
  fields.leaderCommit = commitIndex_;
  fields.prevLogIndex = nextIndex_[followerId] - 1;
  fields.prevLogTerm = log_[fields.prevLogIndex]->term;
  fields.followerId = followerId;
  if (heartBeat) {
    return fields;
  }
  const uint64_t firstNew = nextIndex_[followerId];
  const uint64_t limit = std::min(lastLogIndex_, (firstNew + maxEntries) - 1);
  for (uint64_t i = firstNew; i <= limit; ++i) {
    LogEntry entry;
    entry.term = log_[i]->term;
    entry.command = log_[i]->command;
    fields.entries.push_back(entry);
  }
  return fields;
}

// returns vector of tuples <followerId, AeFields>
// If heartBeat == true, entries[] will be empty for all messages
// else entries will each contain at most maxEntries amount of entries
std::vector<AeFields> Raft::GatherAeFieldsForBroadcastLocked(bool heartBeat) const {
  std::vector<AeFields> fieldsVec;
  fieldsVec.reserve(total_num_ - 1);
  for (int i = 1; i <= total_num_; ++i) {
    if (i == id_) {
      continue;
    }
    AeFields fields = GatherAeFieldsLocked(i, heartBeat);
    fieldsVec.push_back(fields);
  }
  return fieldsVec;
}

void Raft::CreateAndSendAppendEntryMsg(const AeFields& fields) {
  int followerId = fields.followerId;
  AppendEntries ae;
  ae.set_term(fields.term);
  ae.set_leaderid(fields.leaderId);
  ae.set_prevlogindex(fields.prevLogIndex);
  ae.set_prevlogterm(fields.prevLogTerm);
  ae.set_leadercommitindex(fields.leaderCommit);
  uint64_t entryCount = 0; 
  for (const auto& entry : fields.entries) {
    auto* newEntry = ae.add_entries();
    newEntry->set_term(entry.term);
    newEntry->set_command(entry.command);
    if (entryCount > 0 && ae.ByteSizeLong() > maxBytes) {
      ae.mutable_entries()->RemoveLast();
      break;
    }
    entryCount++;
  }
  SendMessage(MessageType::AppendEntriesMsg, ae, followerId);
  if (replicationLoggingFlag_) {
    LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Sent AE with " << entryCount << (entryCount == 1 ? " entry" : " entries");
  }
}

}  // namespace raft
}  // namespace resdb

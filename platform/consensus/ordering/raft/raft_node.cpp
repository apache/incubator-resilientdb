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

#include "platform/consensus/ordering/raft/raft_node.h"

#include <algorithm>
#include <chrono>
#include <random>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "executor/common/transaction_manager.h"
#include "glog/logging.h"
#include "platform/consensus/ordering/raft/consensus_manager_raft.h"

namespace resdb {

namespace {
constexpr std::chrono::milliseconds kMinElectionTimeout(300);
constexpr std::chrono::milliseconds kMaxElectionTimeout(600);
constexpr size_t kMaxEntriesPerAppend = 8;
}  // namespace

RaftNode::RaftNode(const ResDBConfig& config, ConsensusManagerRaft* manager,
                   TransactionManager* transaction_manager, RaftLog* log,
                   RaftPersistentState* persistent_state,
                   RaftSnapshotManager* snapshot_manager, RaftRpc* rpc)
    : config_(config),
      manager_(manager),
      transaction_manager_(transaction_manager),
      raft_log_(log),
      persistent_state_(persistent_state),
      snapshot_manager_(snapshot_manager),
      raft_rpc_(rpc) {
  replicas_ = config_.GetReplicaInfos();
  for (const auto& replica : replicas_) {
    replica_map_[replica.id()] = replica;
  }
  self_id_ = config_.GetSelfInfo().id();
  current_term_ = persistent_state_ ? persistent_state_->CurrentTerm() : 0;
  voted_for_ = persistent_state_ ? persistent_state_->VotedFor() : 0;
  commit_index_ = persistent_state_ ? persistent_state_->CommitIndex() : 0;
  last_applied_ = persistent_state_ ? persistent_state_->LastApplied() : 0;
  ResetElectionDeadline();
}

RaftNode::~RaftNode() { Stop(); }

void RaftNode::Start() {
  stop_ = false;
  election_thread_ = std::thread(&RaftNode::RunElectionLoop, this);
}

void RaftNode::Stop() {
  stop_ = true;
  election_cv_.notify_all();
  if (election_thread_.joinable()) {
    election_thread_.join();
  }
}

int RaftNode::HandleClientRequest(std::unique_ptr<Context> context,
                                  std::unique_ptr<Request> request) {
  std::unique_lock<std::mutex> lk(state_mutex_);
  if (role_ != Role::kLeader) {
    if (context && context->client) {
      LOG(WARNING) << "Rejecting client request: not leader";
    }
    return -1;
  }

  raft::LogEntry entry;
  entry.set_term(current_term_);
  entry.set_index(raft_log_->LastLogIndex() + 1);
  *entry.mutable_request() = *request;

  absl::Status status = raft_log_->Append({entry});
  if (!status.ok()) {
    LOG(ERROR) << "Failed to append RAFT log entry: " << status;
    return -2;
  }

  pending_requests_[entry.index()] =
      PendingRequest{std::move(context), std::move(request)};

  match_index_[self_id_] = entry.index();
  next_index_[self_id_] = entry.index() + 1;
  lk.unlock();
  BroadcastAppendEntries(/*send_all_entries=*/true);
  return 0;
}

int RaftNode::HandleConsensusMessage(std::unique_ptr<Context> context,
                                     std::unique_ptr<Request> request) {
  (void)context;
  switch (request->user_type()) {
    case raft::RAFT_APPEND_ENTRIES_REQUEST: {
      raft::AppendEntriesRequest rpc;
      if (!rpc.ParseFromString(request->data())) {
        return -1;
      }
      return HandleAppendEntriesRequest(*request, rpc);
    }
    case raft::RAFT_APPEND_ENTRIES_RESPONSE: {
      raft::AppendEntriesResponse rpc;
      if (!rpc.ParseFromString(request->data())) {
        return -1;
      }
      return HandleAppendEntriesResponse(rpc);
    }
    case raft::RAFT_REQUEST_VOTE_REQUEST: {
      raft::RequestVoteRequest rpc;
      if (!rpc.ParseFromString(request->data())) {
        return -1;
      }
      return HandleRequestVoteRequest(*request, rpc);
    }
    case raft::RAFT_REQUEST_VOTE_RESPONSE: {
      raft::RequestVoteResponse rpc;
      if (!rpc.ParseFromString(request->data())) {
        return -1;
      }
      return HandleRequestVoteResponse(rpc);
    }
    case raft::RAFT_INSTALL_SNAPSHOT_REQUEST: {
      raft::InstallSnapshotRequest rpc;
      if (!rpc.ParseFromString(request->data())) {
        return -1;
      }
      return HandleInstallSnapshotRequest(*request, rpc);
    }
    default:
      return -1;
  }
}

void RaftNode::RunElectionLoop() {
  while (!stop_) {
    std::unique_lock<std::mutex> lk(election_mutex_);
    if (election_cv_.wait_until(lk, next_election_deadline_,
                                [&]() { return stop_.load(); })) {
      continue;
    }
    lk.unlock();
    StartElection();
  }
}

void RaftNode::ResetElectionDeadline() {
  std::lock_guard<std::mutex> lk(election_mutex_);
  static thread_local std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<int> dist(
      0, static_cast<int>((kMaxElectionTimeout - kMinElectionTimeout).count()));
  next_election_deadline_ =
      std::chrono::steady_clock::now() + kMinElectionTimeout +
      std::chrono::milliseconds(dist(rng));
  election_cv_.notify_all();
}

void RaftNode::StartElection() {
  std::unique_lock<std::mutex> lk(state_mutex_);
  if (role_ == Role::kLeader) {
    ResetElectionDeadline();
    return;
  }

  manager_->UpdateLeadership(0, current_term_);
  role_ = Role::kCandidate;
  current_term_++;
  voted_for_ = self_id_;
  leader_id_ = 0;
  votes_granted_ = 1;
  vote_record_.clear();
  vote_record_[self_id_] = true;
  PersistTermAndVote();

  raft::RequestVoteRequest rpc;
  rpc.set_term(current_term_);
  rpc.set_candidate_id(self_id_);
  rpc.set_last_log_index(raft_log_->LastLogIndex());
  rpc.set_last_log_term(LastLogTerm());
  lk.unlock();

  for (const auto& replica : replicas_) {
    if (replica.id() == self_id_) {
      continue;
    }
    if (raft_rpc_) {
      raft_rpc_->SendRequestVote(rpc, replica);
    }
  }
  ResetElectionDeadline();
}

void RaftNode::BecomeFollower(uint64_t term, uint32_t leader_id) {
  role_ = Role::kFollower;
  leader_id_ = leader_id;
  if (term > current_term_) {
    current_term_ = term;
    voted_for_ = 0;
    PersistTermAndVote();
  }
  manager_->UpdateLeadership(leader_id_, current_term_);
  ResetElectionDeadline();
}

void RaftNode::BecomeCandidate() {
  role_ = Role::kCandidate;
  leader_id_ = 0;
  manager_->UpdateLeadership(0, current_term_);
  ResetElectionDeadline();
}

void RaftNode::BecomeLeader() {
  role_ = Role::kLeader;
  leader_id_ = self_id_;
  manager_->UpdateLeadership(self_id_, current_term_);
  uint64_t next = raft_log_->LastLogIndex() + 1;
  for (const auto& replica : replicas_) {
    next_index_[replica.id()] = next;
    match_index_[replica.id()] = 0;
  }
  match_index_[self_id_] = raft_log_->LastLogIndex();
  next_index_[self_id_] = next;
  ResetElectionDeadline();
  SendHeartbeats();
}

void RaftNode::SendHeartbeats() { BroadcastAppendEntries(/*send_all_entries=*/false); }

void RaftNode::HeartbeatTick() { SendHeartbeats(); }

void RaftNode::BroadcastAppendEntries(bool send_all_entries) {
  std::unique_lock<std::mutex> lk(state_mutex_);
  if (role_ != Role::kLeader) {
    return;
  }
  uint64_t leader_commit = commit_index_;
  uint64_t current_term = current_term_;
  uint32_t leader_id = self_id_;
  auto next_index = next_index_;
  auto last_log_index = raft_log_->LastLogIndex();
  lk.unlock();

  for (const auto& replica : replicas_) {
    if (replica.id() == self_id_) {
      continue;
    }
    raft::AppendEntriesRequest rpc;
    rpc.set_term(current_term);
    rpc.set_leader_id(leader_id);
    uint64_t prev_index = next_index[replica.id()] == 0
                              ? 0
                              : next_index[replica.id()] - 1;
    rpc.set_prev_log_index(prev_index);
    rpc.set_prev_log_term(GetLogTerm(prev_index));
    rpc.set_leader_commit(leader_commit);

    if (send_all_entries && next_index[replica.id()] <= last_log_index) {
      uint64_t start = next_index[replica.id()];
      uint64_t end = std::min(last_log_index,
                              start + static_cast<uint64_t>(kMaxEntriesPerAppend) - 1);
      auto entries = raft_log_->GetEntries(start, end);
      if (entries.ok()) {
        for (const auto& entry : *entries) {
          *rpc.add_entries() = entry;
        }
      }
    }

    if (raft_rpc_) {
      raft_rpc_->SendAppendEntries(rpc, replica);
    }
  }
}

void RaftNode::MaybeAdvanceCommitIndex() {
  std::lock_guard<std::mutex> lk(state_mutex_);
  if (role_ != Role::kLeader) {
    return;
  }
  uint64_t last_index = raft_log_->LastLogIndex();
  for (uint64_t index = commit_index_ + 1; index <= last_index; ++index) {
    int votes = 1;  // leader
    for (const auto& replica : replicas_) {
      if (replica.id() == self_id_) continue;
      if (match_index_[replica.id()] >= index) {
        votes++;
      }
    }
    if (votes >= static_cast<int>(RequiredQuorum()) &&
        GetLogTerm(index) == current_term_) {
      commit_index_ = index;
    }
  }
  raft_log_->CommitTo(commit_index_);
  persistent_state_->SetCommitIndex(commit_index_);
  persistent_state_->Store();
  ApplyEntries();
}

void RaftNode::ApplyEntries() {
  while (true) {
    std::unique_lock<std::mutex> lk(state_mutex_);
    if (last_applied_ >= commit_index_) {
      break;
    }
    uint64_t index = last_applied_ + 1;
    auto entry = raft_log_->GetEntry(index);
    if (!entry.ok()) {
      break;
    }
    PendingRequest pending;
    auto it = pending_requests_.find(index);
    if (it != pending_requests_.end()) {
      pending = std::move(it->second);
      pending_requests_.erase(it);
    }
    lk.unlock();
    ApplyEntry(index, *entry);
    if (pending.context && pending.context->client) {
      pending.context->client->SendRawMessage(entry->request());
    }
    lk.lock();
    last_applied_ = index;
    persistent_state_->SetLastApplied(last_applied_);
    persistent_state_->Store();
  }
}

void RaftNode::ApplyEntry(uint64_t index, const raft::LogEntry& entry) {
  (void)index;
  if (!transaction_manager_) {
    return;
  }
  transaction_manager_->ExecuteData(entry.request().data());
}

int RaftNode::HandleAppendEntriesRequest(
    const Request& envelope, const raft::AppendEntriesRequest& request) {
  std::lock_guard<std::mutex> lk(state_mutex_);
  if (request.term() < current_term_) {
    raft::AppendEntriesResponse response;
    response.set_term(current_term_);
    response.set_success(false);
    response.set_responder_id(self_id_);
    response.set_conflict_index(raft_log_->LastLogIndex() + 1);
    if (auto replica = ReplicaById(envelope.sender_id()); replica && raft_rpc_) {
      raft_rpc_->SendAppendEntriesResponse(response, *replica);
    }
    return 0;
  }

  if (request.term() > current_term_ || role_ != Role::kFollower) {
    BecomeFollower(request.term(), request.leader_id());
  }
  ResetElectionDeadline();
  leader_id_ = request.leader_id();

  uint64_t prev_log_index = request.prev_log_index();
  uint64_t prev_log_term = request.prev_log_term();
  uint64_t local_term = GetLogTerm(prev_log_index);
  raft::AppendEntriesResponse response;
  response.set_term(current_term_);
  response.set_responder_id(self_id_);

  if (local_term != prev_log_term) {
    response.set_success(false);
    response.set_conflict_index(prev_log_index == 0 ? 1 : prev_log_index);
  } else {
    response.set_success(true);
    for (const auto& entry : request.entries()) {
      absl::Status status = raft_log_->Append({entry});
      if (!status.ok()) {
        LOG(ERROR) << "Failed to append follower log: " << status;
        response.set_success(false);
        break;
      }
    }
    if (request.leader_commit() > commit_index_) {
      commit_index_ = std::min(request.leader_commit(),
                               raft_log_->LastLogIndex());
      raft_log_->CommitTo(commit_index_);
      ApplyEntries();
    }
    response.set_match_index(raft_log_->LastLogIndex());
  }

  if (auto replica = ReplicaById(envelope.sender_id()); replica && raft_rpc_) {
    raft_rpc_->SendAppendEntriesResponse(response, *replica);
  }
  return 0;
}

int RaftNode::HandleAppendEntriesResponse(
    const raft::AppendEntriesResponse& response) {
  std::lock_guard<std::mutex> lk(state_mutex_);
  if (response.term() > current_term_) {
    BecomeFollower(response.term(), 0);
    return 0;
  }
  if (role_ != Role::kLeader) {
    return 0;
  }
  if (!response.success()) {
    next_index_[response.responder_id()] =
        std::max<uint64_t>(1, response.conflict_index());
    return 0;
  }
  match_index_[response.responder_id()] = response.match_index();
  next_index_[response.responder_id()] = response.match_index() + 1;
  lk.unlock();
  MaybeAdvanceCommitIndex();
  return 0;
}

int RaftNode::HandleRequestVoteRequest(
    const Request& envelope, const raft::RequestVoteRequest& request) {
  std::lock_guard<std::mutex> lk(state_mutex_);
  raft::RequestVoteResponse response;
  response.set_term(current_term_);
  response.set_responder_id(self_id_);

  if (request.term() < current_term_) {
    response.set_vote_granted(false);
  } else {
    if (request.term() > current_term_) {
      BecomeFollower(request.term(), 0);
    }
    bool voted = (voted_for_ == 0 || voted_for_ == request.candidate_id());
    bool log_ok = (request.last_log_term() > LastLogTerm()) ||
                  (request.last_log_term() == LastLogTerm() &&
                   request.last_log_index() >= raft_log_->LastLogIndex());
    if (voted && log_ok) {
      voted_for_ = request.candidate_id();
      PersistTermAndVote();
      response.set_vote_granted(true);
      ResetElectionDeadline();
    } else {
      response.set_vote_granted(false);
    }
  }

  if (auto replica = ReplicaById(envelope.sender_id()); replica && raft_rpc_) {
    raft_rpc_->SendRequestVoteResponse(response, *replica);
  }
  return 0;
}

int RaftNode::HandleRequestVoteResponse(
    const raft::RequestVoteResponse& response) {
  std::lock_guard<std::mutex> lk(state_mutex_);
  if (response.term() > current_term_) {
    BecomeFollower(response.term(), 0);
    return 0;
  }
  if (role_ != Role::kCandidate || response.term() != current_term_) {
    return 0;
  }
  if (response.vote_granted() && !vote_record_[response.responder_id()]) {
    vote_record_[response.responder_id()] = true;
    votes_granted_++;
    if (votes_granted_ >= RequiredQuorum()) {
      BecomeLeader();
    }
  }
  return 0;
}

int RaftNode::HandleInstallSnapshotRequest(
    const Request& envelope, const raft::InstallSnapshotRequest& request) {
  std::lock_guard<std::mutex> lk(state_mutex_);
  if (request.term() > current_term_) {
    BecomeFollower(request.term(), request.leader_id());
  }
  ResetElectionDeadline();
  raft::InstallSnapshotResponse response;
  response.set_term(current_term_);
  response.set_responder_id(self_id_);
  response.set_success(true);
  response.set_applied_index(request.metadata().last_included_index());
  if (auto replica = ReplicaById(envelope.sender_id()); replica && raft_rpc_) {
    raft_rpc_->SendInstallSnapshotResponse(response, *replica);
  }
  return 0;
}

std::optional<ReplicaInfo> RaftNode::ReplicaById(uint32_t node_id) const {
  auto it = replica_map_.find(node_id);
  if (it == replica_map_.end()) {
    return std::nullopt;
  }
  return it->second;
}

uint64_t RaftNode::LastLogTerm() const {
  return GetLogTerm(raft_log_->LastLogIndex());
}

uint64_t RaftNode::GetLogTerm(uint64_t index) const {
  if (index == 0) {
    return 0;
  }
  auto entry = raft_log_->GetEntry(index);
  if (!entry.ok()) {
    return 0;
  }
  return entry->term();
}

bool RaftNode::IsMajority(uint64_t match_index) const {
  int votes = 1;
  for (const auto& replica : replicas_) {
    if (replica.id() == self_id_) continue;
    if (match_index_[replica.id()] >= match_index) {
      votes++;
    }
  }
  return votes >= static_cast<int>(RequiredQuorum());
}

uint64_t RaftNode::RequiredQuorum() const {
  return replicas_.empty() ? 1 : (replicas_.size() / 2 + 1);
}

void RaftNode::PersistTermAndVote() {
  if (!persistent_state_) {
    return;
  }
  persistent_state_->SetCurrentTerm(current_term_);
  persistent_state_->SetVotedFor(voted_for_);
  persistent_state_->Store();
}

}  // namespace resdb

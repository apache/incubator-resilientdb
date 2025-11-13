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

#include "platform/consensus/ordering/raft/raft_persistent_state.h"

#include "glog/logging.h"

namespace resdb {

RaftPersistentState::RaftPersistentState(Storage* storage)
    : storage_(storage) {
  auto& allocator = storage::PrefixAllocator::Instance();
  auto status_or_key =
      allocator.MakeKey(storage::kPrefixRaftState, "persistent_state");
  if (!status_or_key.ok()) {
    LOG(FATAL) << "Failed to compute RAFT state key: "
               << status_or_key.status();
  }
  state_key_ = *status_or_key;
}

absl::Status RaftPersistentState::Load() {
  if (storage_ == nullptr) {
    return absl::FailedPreconditionError("storage is null");
  }
  std::string payload = storage_->GetValue(state_key_);
  if (payload.empty()) {
    state_.Clear();
    state_.set_current_term(0);
    state_.set_voted_for(0);
    state_.set_last_applied(0);
    state_.set_commit_index(0);
    return absl::OkStatus();
  }
  if (!state_.ParseFromString(payload)) {
    return absl::InternalError("failed to parse persistent state");
  }
  return absl::OkStatus();
}

absl::Status RaftPersistentState::Store() const {
  if (storage_ == nullptr) {
    return absl::FailedPreconditionError("storage is null");
  }
  if (storage_->SetValue(state_key_, state_.SerializeAsString()) != 0) {
    return absl::InternalError("failed to store persistent state");
  }
  return absl::OkStatus();
}

void RaftPersistentState::SetCurrentTerm(uint64_t term) {
  state_.set_current_term(term);
}

void RaftPersistentState::SetVotedFor(uint32_t node_id) {
  state_.set_voted_for(node_id);
}

void RaftPersistentState::SetLastApplied(uint64_t index) {
  state_.set_last_applied(index);
}

void RaftPersistentState::SetCommitIndex(uint64_t index) {
  state_.set_commit_index(index);
}

void RaftPersistentState::SetSnapshotMetadata(
    const raft::SnapshotMetadata& metadata) {
  *state_.mutable_snapshot() = metadata;
}

}  // namespace resdb

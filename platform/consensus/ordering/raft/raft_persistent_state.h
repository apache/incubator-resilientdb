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

#pragma once

#include <cstdint>
#include <string>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "chain/storage/prefix_allocator.h"
#include "chain/storage/storage.h"
#include "platform/consensus/ordering/raft/proto/raft.pb.h"

namespace resdb {

class RaftPersistentState {
 public:
  explicit RaftPersistentState(Storage* storage);

  absl::Status Load();
  absl::Status Store() const;

  void SetCurrentTerm(uint64_t term);
  uint64_t CurrentTerm() const { return state_.current_term(); }

  void SetVotedFor(uint32_t node_id);
  uint32_t VotedFor() const { return state_.voted_for(); }

  void SetLastApplied(uint64_t index);
  uint64_t LastApplied() const { return state_.last_applied(); }

  void SetCommitIndex(uint64_t index);
  uint64_t CommitIndex() const { return state_.commit_index(); }

  void SetSnapshotMetadata(const raft::SnapshotMetadata& metadata);
  const raft::SnapshotMetadata& SnapshotMetadata() const {
    return state_.snapshot();
  }

 private:
  Storage* storage_;
  std::string state_key_;
  raft::PersistentState state_;
};

}  // namespace resdb


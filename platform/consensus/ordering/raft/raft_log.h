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
#include <map>
#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "chain/storage/prefix_allocator.h"
#include "chain/storage/storage.h"
#include "platform/consensus/ordering/raft/proto/raft.pb.h"

namespace resdb {

class RaftLog {
 public:
  explicit RaftLog(Storage* storage);

  absl::Status LoadFromStorage();
  absl::Status Append(const std::vector<raft::LogEntry>& entries);
  absl::Status Truncate(uint64_t index);
  absl::Status CommitTo(uint64_t index);

  absl::StatusOr<raft::LogEntry> GetEntry(uint64_t index) const;
  absl::StatusOr<std::vector<raft::LogEntry>> GetEntries(
      uint64_t start_index, uint64_t end_index) const;

  uint64_t LastLogIndex() const { return last_log_index_; }
  uint64_t LastLogTerm() const;
  uint64_t FirstLogIndex() const { return first_log_index_; }
  uint64_t CommitIndex() const { return commit_index_; }

 private:
  absl::StatusOr<std::string> EncodeEntryKey(uint64_t index) const;
  absl::Status PersistIndex(const std::string& key, uint64_t value) const;
  uint64_t LoadIndex(const std::string& key, uint64_t default_value) const;
  absl::Status PersistEntry(const raft::LogEntry& entry) const;

 private:
  Storage* storage_;
  std::string log_prefix_;
  std::string state_prefix_;

  uint64_t first_log_index_ = 1;
  uint64_t last_log_index_ = 0;
  uint64_t commit_index_ = 0;

  std::map<uint64_t, raft::LogEntry> entries_;
};

}  // namespace resdb


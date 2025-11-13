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

#include "platform/consensus/ordering/raft/raft_log.h"

#include <algorithm>
#include <utility>

#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "chain/storage/storage.h"
#include "glog/logging.h"

namespace resdb {

namespace {
constexpr char kLastIndexKey[] = "last_log_index";
constexpr char kFirstIndexKey[] = "first_log_index";
constexpr char kCommitIndexKey[] = "commit_index";
}  // namespace

RaftLog::RaftLog(Storage* storage) : storage_(storage) {
  auto& allocator = storage::PrefixAllocator::Instance();
  auto log_prefix = allocator.GetPrefix(storage::kPrefixRaftLog);
  auto state_prefix = allocator.GetPrefix(storage::kPrefixRaftState);
  if (!log_prefix.ok() || !state_prefix.ok()) {
    LOG(FATAL) << "RAFT prefixes not registered.";
  }
  log_prefix_ = *log_prefix;
  state_prefix_ = *state_prefix;
}

absl::Status RaftLog::LoadFromStorage() {
  if (storage_ == nullptr) {
    return absl::FailedPreconditionError("storage is null");
  }
  entries_.clear();
  first_log_index_ =
      LoadIndex(state_prefix_ + kFirstIndexKey, /*default_value=*/1);
  last_log_index_ =
      LoadIndex(state_prefix_ + kLastIndexKey, first_log_index_ - 1);
  commit_index_ =
    LoadIndex(state_prefix_ + kCommitIndexKey, first_log_index_ - 1);

  for (uint64_t idx = first_log_index_; idx <= last_log_index_; ++idx) {
    auto key = EncodeEntryKey(idx);
    if (!key.ok()) {
      return key.status();
    }
    std::string serialized = storage_->GetValue(*key);
    if (serialized.empty()) {
      continue;
    }
    raft::LogEntry entry;
    if (!entry.ParseFromString(serialized)) {
      return absl::InternalError("failed to parse log entry from storage");
    }
    entries_[idx] = entry;
  }
  return absl::OkStatus();
}

absl::Status RaftLog::Append(const std::vector<raft::LogEntry>& entries) {
  if (storage_ == nullptr) {
    return absl::FailedPreconditionError("storage is null");
  }
  if (entries.empty()) {
    return absl::OkStatus();
  }
  for (const auto& entry : entries) {
    if (entry.index() == 0) {
      return absl::InvalidArgumentError("log entry must have an index");
    }
    entries_[entry.index()] = entry;
    last_log_index_ = std::max(last_log_index_, entry.index());
    if (first_log_index_ == 0 || entry.index() < first_log_index_) {
      first_log_index_ = entry.index();
    }
    absl::Status persist_status = PersistEntry(entry);
    if (!persist_status.ok()) {
      return persist_status;
    }
  }
  absl::Status status =
      PersistIndex(state_prefix_ + kLastIndexKey, last_log_index_);
  if (!status.ok()) {
    return status;
  }
  status = PersistIndex(state_prefix_ + kFirstIndexKey, first_log_index_);
  if (!status.ok()) {
    return status;
  }
  return absl::OkStatus();
}

absl::Status RaftLog::Truncate(uint64_t index) {
  if (index <= first_log_index_) {
    return absl::InvalidArgumentError("cannot truncate before first index");
  }
  if (index > last_log_index_ + 1) {
    return absl::InvalidArgumentError("truncate index out of range");
  }
  auto it = entries_.lower_bound(index);
  while (it != entries_.end()) {
    it = entries_.erase(it);
  }
  last_log_index_ = index - 1;
  if (commit_index_ > last_log_index_) {
    commit_index_ = last_log_index_;
  }
  absl::Status status =
      PersistIndex(state_prefix_ + kLastIndexKey, last_log_index_);
  if (!status.ok()) {
    return status;
  }
  status = PersistIndex(state_prefix_ + kCommitIndexKey, commit_index_);
  if (!status.ok()) {
    return status;
  }
  return absl::OkStatus();
}

absl::Status RaftLog::CommitTo(uint64_t index) {
  if (index > last_log_index_) {
    return absl::InvalidArgumentError("commit index beyond last log index");
  }
  if (index < commit_index_) {
    return absl::OkStatus();
  }
  commit_index_ = index;
  return PersistIndex(state_prefix_ + kCommitIndexKey, commit_index_);
}

absl::StatusOr<raft::LogEntry> RaftLog::GetEntry(uint64_t index) const {
  auto it = entries_.find(index);
  if (it == entries_.end()) {
    return absl::NotFoundError("entry not in memory");
  }
  return it->second;
}

absl::StatusOr<std::vector<raft::LogEntry>> RaftLog::GetEntries(
    uint64_t start_index, uint64_t end_index) const {
  if (start_index > end_index) {
    return absl::InvalidArgumentError("start index beyond end index");
  }
  std::vector<raft::LogEntry> result;
  for (uint64_t idx = start_index; idx <= end_index; ++idx) {
    auto entry = GetEntry(idx);
    if (!entry.ok()) {
      return entry.status();
    }
    result.push_back(*entry);
  }
  return result;
}

uint64_t RaftLog::LastLogTerm() const {
  auto it = entries_.find(last_log_index_);
  if (it == entries_.end()) {
    return 0;
  }
  return it->second.term();
}

absl::StatusOr<std::string> RaftLog::EncodeEntryKey(uint64_t index) const {
  if (log_prefix_.empty()) {
    return absl::FailedPreconditionError("log prefix not initialised");
  }
  return absl::StrCat(log_prefix_, absl::StrFormat("%020lu", index));
}

absl::Status RaftLog::PersistIndex(const std::string& key,
                                   uint64_t value) const {
  if (storage_->SetValue(key, absl::StrCat(value)) != 0) {
    return absl::InternalError("failed to persist index");
  }
  return absl::OkStatus();
}

uint64_t RaftLog::LoadIndex(const std::string& key,
                            uint64_t default_value) const {
  std::string value = storage_->GetValue(key);
  uint64_t parsed = default_value;
  if (!value.empty() && !absl::SimpleAtoi(value, &parsed)) {
    LOG(ERROR) << "Failed to parse stored index for key " << key;
    parsed = default_value;
  }
  return parsed;
}

absl::Status RaftLog::PersistEntry(const raft::LogEntry& entry) const {
  auto key = EncodeEntryKey(entry.index());
  if (!key.ok()) {
    return key.status();
  }
  if (storage_->SetValue(*key, entry.SerializeAsString()) != 0) {
    return absl::InternalError("failed to persist log entry");
  }
  return absl::OkStatus();
}

}  // namespace resdb

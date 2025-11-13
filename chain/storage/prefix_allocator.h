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

#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "absl/base/thread_annotations.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"

namespace resdb {
namespace storage {

inline constexpr char kPrefixPbftCheckpoint[] = "pbft_checkpoint";
inline constexpr char kPrefixPbftMessage[] = "pbft_message";
inline constexpr char kPrefixPbftRecovery[] = "pbft_recovery";
inline constexpr char kPrefixRaftState[] = "raft_state";
inline constexpr char kPrefixRaftLog[] = "raft_log";
inline constexpr char kPrefixRaftSnapshot[] = "raft_snapshot";

class PrefixAllocator {
 public:
  static PrefixAllocator& Instance();

  absl::Status Reserve(const std::string& owner,
                       const std::string& prefix) ABSL_LOCKS_EXCLUDED(mutex_);

  absl::StatusOr<std::string> GetPrefix(const std::string& owner) const
      ABSL_LOCKS_EXCLUDED(mutex_);

  absl::StatusOr<std::string> MakeKey(const std::string& owner,
                                      const std::string& suffix) const
      ABSL_LOCKS_EXCLUDED(mutex_);

  bool HasPrefix(const std::string& owner) const
      ABSL_LOCKS_EXCLUDED(mutex_);

 private:
  PrefixAllocator() = default;

  absl::StatusOr<std::string> Lookup(const std::string& owner) const
      ABSL_SHARED_LOCKS_REQUIRED(mutex_);

 private:
  mutable std::mutex mutex_;
  std::unordered_map<std::string, std::string> owner_to_prefix_
      ABSL_GUARDED_BY(mutex_);
  std::unordered_set<std::string> allocated_prefixes_
      ABSL_GUARDED_BY(mutex_);
};

void RegisterDefaultPrefixes(PrefixAllocator* allocator = nullptr);

}  // namespace storage
}  // namespace resdb

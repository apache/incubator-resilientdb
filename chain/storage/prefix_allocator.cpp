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

#include "chain/storage/prefix_allocator.h"

#include <mutex>
#include <utility>

#include "glog/logging.h"

namespace resdb {
namespace storage {

namespace {

struct PrefixReservation {
  const char* owner;
  const char* prefix;
};

constexpr PrefixReservation kDefaultReservations[] = {
    {kPrefixPbftCheckpoint, "pbft_checkpoint/"},
    {kPrefixPbftMessage, "pbft_message/"},
    {kPrefixPbftRecovery, "pbft_recovery/"},
    {kPrefixRaftState, "raft_state/"},
    {kPrefixRaftLog, "raft_log/"},
    {kPrefixRaftSnapshot, "raft_snapshot/"},
};

}  // namespace

PrefixAllocator& PrefixAllocator::Instance() {
  static PrefixAllocator* allocator = new PrefixAllocator();
  static std::once_flag init_flag;
  std::call_once(init_flag, []() { RegisterDefaultPrefixes(allocator); });
  return *allocator;
}

absl::Status PrefixAllocator::Reserve(const std::string& owner,
                                      const std::string& prefix) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = owner_to_prefix_.find(owner);
  if (it != owner_to_prefix_.end()) {
    if (it->second == prefix) {
      return absl::OkStatus();
    }
    return absl::AlreadyExistsError("Owner already registered with prefix");
  }
  if (allocated_prefixes_.count(prefix)) {
    return absl::AlreadyExistsError("Prefix already in use");
  }
  owner_to_prefix_[owner] = prefix;
  allocated_prefixes_.insert(prefix);
  return absl::OkStatus();
}

absl::StatusOr<std::string> PrefixAllocator::Lookup(
    const std::string& owner) const {
  auto it = owner_to_prefix_.find(owner);
  if (it == owner_to_prefix_.end()) {
    return absl::NotFoundError("Prefix owner not registered");
  }
  return it->second;
}

absl::StatusOr<std::string> PrefixAllocator::GetPrefix(
    const std::string& owner) const {
  std::lock_guard<std::mutex> lock(mutex_);
  return Lookup(owner);
}

bool PrefixAllocator::HasPrefix(const std::string& owner) const {
  std::lock_guard<std::mutex> lock(mutex_);
  return owner_to_prefix_.find(owner) != owner_to_prefix_.end();
}

absl::StatusOr<std::string> PrefixAllocator::MakeKey(
    const std::string& owner, const std::string& suffix) const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto prefix = Lookup(owner);
  if (!prefix.ok()) {
    return prefix.status();
  }
  return *prefix + suffix;
}

void RegisterDefaultPrefixes(PrefixAllocator* allocator) {
  PrefixAllocator& target =
      allocator == nullptr ? PrefixAllocator::Instance() : *allocator;
  for (const auto& reservation : kDefaultReservations) {
    absl::Status status =
        target.Reserve(reservation.owner, reservation.prefix);
    if (!status.ok()) {
      LOG(ERROR) << "Failed to register prefix for " << reservation.owner
                 << ": " << status;
    }
  }
}

}  // namespace storage
}  // namespace resdb

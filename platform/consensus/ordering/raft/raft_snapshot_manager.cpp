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

#include "platform/consensus/ordering/raft/raft_snapshot_manager.h"

#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "glog/logging.h"

namespace resdb {

RaftSnapshotManager::RaftSnapshotManager(Storage* storage)
    : storage_(storage) {
  auto& allocator = storage::PrefixAllocator::Instance();
  auto meta_key =
      allocator.MakeKey(storage::kPrefixRaftSnapshot, "metadata");
  auto chunk_count =
      allocator.MakeKey(storage::kPrefixRaftSnapshot, "chunk_count");
  auto chunk_prefix =
      allocator.GetPrefix(storage::kPrefixRaftSnapshot);

  if (!meta_key.ok() || !chunk_count.ok() || !chunk_prefix.ok()) {
    LOG(FATAL) << "Failed to compute snapshot storage prefixes";
  }
  metadata_key_ = *meta_key;
  chunk_count_key_ = *chunk_count;
  chunk_prefix_ = *chunk_prefix + "chunk_";
}

absl::Status RaftSnapshotManager::PersistSnapshot(
    const raft::SnapshotMetadata& metadata,
    const std::vector<raft::SnapshotChunk>& chunks) {
  if (storage_ == nullptr) {
    return absl::FailedPreconditionError("storage is null");
  }
  if (storage_->SetValue(metadata_key_, metadata.SerializeAsString()) != 0) {
    return absl::InternalError("failed to persist snapshot metadata");
  }
  for (const auto& chunk : chunks) {
    if (storage_->SetValue(ChunkKey(chunk.chunk_index()),
                           chunk.SerializeAsString()) != 0) {
      return absl::InternalError("failed to persist snapshot chunk");
    }
  }
  if (storage_->SetValue(chunk_count_key_, absl::StrCat(chunks.size())) != 0) {
    return absl::InternalError("failed to persist chunk count");
  }
  return absl::OkStatus();
}

absl::StatusOr<raft::SnapshotMetadata> RaftSnapshotManager::LoadMetadata()
    const {
  if (storage_ == nullptr) {
    return absl::FailedPreconditionError("storage is null");
  }
  std::string payload = storage_->GetValue(metadata_key_);
  raft::SnapshotMetadata metadata;
  if (payload.empty()) {
    return metadata;
  }
  if (!metadata.ParseFromString(payload)) {
    return absl::InternalError("failed to parse snapshot metadata");
  }
  return metadata;
}

absl::StatusOr<std::vector<raft::SnapshotChunk>>
RaftSnapshotManager::LoadChunks() const {
  if (storage_ == nullptr) {
    return absl::FailedPreconditionError("storage is null");
  }
  std::string count_str = storage_->GetValue(chunk_count_key_);
  uint64_t count = 0;
  if (!count_str.empty() && !absl::SimpleAtoi(count_str, &count)) {
    return absl::InternalError("invalid chunk count stored");
  }
  std::vector<raft::SnapshotChunk> chunks;
  chunks.reserve(count);
  for (uint64_t idx = 0; idx < count; ++idx) {
    std::string payload = storage_->GetValue(ChunkKey(idx));
    if (payload.empty()) {
      continue;
    }
    raft::SnapshotChunk chunk;
    if (!chunk.ParseFromString(payload)) {
      return absl::InternalError("failed to parse snapshot chunk");
    }
    chunks.push_back(chunk);
  }
  return chunks;
}

absl::Status RaftSnapshotManager::Reset() {
  if (storage_ == nullptr) {
    return absl::FailedPreconditionError("storage is null");
  }
  if (storage_->SetValue(metadata_key_, "") != 0) {
    return absl::InternalError("failed to reset metadata");
  }
  if (storage_->SetValue(chunk_count_key_, "0") != 0) {
    return absl::InternalError("failed to reset chunk count");
  }
  return absl::OkStatus();
}

std::string RaftSnapshotManager::ChunkKey(uint64_t chunk_index) const {
  return absl::StrCat(chunk_prefix_, absl::StrFormat("%020lu", chunk_index));
}

}  // namespace resdb

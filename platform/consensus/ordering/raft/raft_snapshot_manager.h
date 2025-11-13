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

#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "chain/storage/prefix_allocator.h"
#include "chain/storage/storage.h"
#include "platform/consensus/ordering/raft/proto/raft.pb.h"

namespace resdb {

class RaftSnapshotManager {
 public:
  explicit RaftSnapshotManager(Storage* storage);

  absl::Status PersistSnapshot(const raft::SnapshotMetadata& metadata,
                               const std::vector<raft::SnapshotChunk>& chunks);

  absl::StatusOr<raft::SnapshotMetadata> LoadMetadata() const;
  absl::StatusOr<std::vector<raft::SnapshotChunk>> LoadChunks() const;

  absl::Status Reset();

 private:
  std::string ChunkKey(uint64_t chunk_index) const;

 private:
  Storage* storage_;
  std::string metadata_key_;
  std::string chunk_count_key_;
  std::string chunk_prefix_;
};

}  // namespace resdb


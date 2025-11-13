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

#include "chain/storage/memory_db.h"
#include "gtest/gtest.h"

namespace resdb {
namespace {

TEST(RaftSnapshotManagerTest, PersistAndLoadChunks) {
  auto storage = storage::NewMemoryDB();
  RaftSnapshotManager manager(storage.get());

  raft::SnapshotMetadata metadata;
  metadata.set_last_included_index(42);
  metadata.set_last_included_term(3);

  std::vector<raft::SnapshotChunk> chunks;
  for (uint64_t i = 0; i < 3; ++i) {
    raft::SnapshotChunk chunk;
    chunk.set_chunk_index(i);
    chunk.set_data("chunk_" + std::to_string(i));
    chunk.set_last_chunk(i == 2);
    chunks.push_back(chunk);
  }

  ASSERT_TRUE(manager.PersistSnapshot(metadata, chunks).ok());

  auto loaded_meta = manager.LoadMetadata();
  ASSERT_TRUE(loaded_meta.ok());
  EXPECT_EQ(loaded_meta->last_included_index(), 42);

  auto loaded_chunks = manager.LoadChunks();
  ASSERT_TRUE(loaded_chunks.ok());
  EXPECT_EQ(loaded_chunks->size(), chunks.size());
  EXPECT_TRUE((*loaded_chunks)[2].last_chunk());
}

}  // namespace
}  // namespace resdb


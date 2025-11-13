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

#include "chain/storage/memory_db.h"
#include "gtest/gtest.h"

namespace resdb {
namespace {

TEST(RaftPersistentStateTest, PersistAndLoad) {
  auto storage = storage::NewMemoryDB();
  RaftPersistentState state(storage.get());
  state.SetCurrentTerm(5);
  state.SetVotedFor(3);
  state.SetCommitIndex(10);
  state.SetLastApplied(8);
  raft::SnapshotMetadata metadata;
  metadata.set_last_included_index(7);
  metadata.set_last_included_term(2);
  state.SetSnapshotMetadata(metadata);
  ASSERT_TRUE(state.Store().ok());

  RaftPersistentState reloaded(storage.get());
  ASSERT_TRUE(reloaded.Load().ok());
  EXPECT_EQ(reloaded.CurrentTerm(), 5);
  EXPECT_EQ(reloaded.VotedFor(), 3);
  EXPECT_EQ(reloaded.CommitIndex(), 10);
  EXPECT_EQ(reloaded.LastApplied(), 8);
  EXPECT_EQ(reloaded.SnapshotMetadata().last_included_index(), 7);
}

}  // namespace
}  // namespace resdb


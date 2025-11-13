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

#include "chain/storage/memory_db.h"
#include "gtest/gtest.h"

namespace resdb {
namespace {

raft::LogEntry MakeEntry(uint64_t index, uint64_t term) {
  raft::LogEntry entry;
  entry.set_index(index);
  entry.set_term(term);
  entry.mutable_request()->set_seq(index);
  return entry;
}

TEST(RaftLogTest, AppendCommitAndReload) {
  auto storage = storage::NewMemoryDB();
  RaftLog log(storage.get());

  std::vector<raft::LogEntry> entries = {MakeEntry(1, 1), MakeEntry(2, 2),
                                         MakeEntry(3, 3)};
  ASSERT_TRUE(log.Append(entries).ok());
  ASSERT_TRUE(log.CommitTo(2).ok());

  RaftLog reloaded(storage.get());
  ASSERT_TRUE(reloaded.LoadFromStorage().ok());
  EXPECT_EQ(reloaded.LastLogIndex(), 3);
  EXPECT_EQ(reloaded.CommitIndex(), 2);
  auto entry = reloaded.GetEntry(2);
  ASSERT_TRUE(entry.ok());
  EXPECT_EQ(entry->term(), 2);
}

TEST(RaftLogTest, TruncateDropsTail) {
  auto storage = storage::NewMemoryDB();
  RaftLog log(storage.get());
  ASSERT_TRUE(log.Append({MakeEntry(1, 1), MakeEntry(2, 1), MakeEntry(3, 1)})
                  .ok());
  ASSERT_TRUE(log.Truncate(3).ok());
  EXPECT_EQ(log.LastLogIndex(), 2);
  auto missing = log.GetEntry(3);
  EXPECT_FALSE(missing.ok());
}

}  // namespace
}  // namespace resdb


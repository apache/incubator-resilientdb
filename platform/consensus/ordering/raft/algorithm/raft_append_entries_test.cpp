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

#include "platform/consensus/ordering/raft/algorithm/raft_tests.h"

namespace resdb {
namespace raft {

// Test 1: A follower receiving 1 AppendEntries with multiple entries that it
// can accept.
TEST_F(RaftTest, FollowerAddsAppendEntriesWithMultipleEntries) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_TRUE(aer.success());
            EXPECT_EQ(aer.last_log_index(), 3);
            return 0;
          }));
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(1);

  auto ae_fields = CreateAeFields(
      /*term=*/0,
      /*leader_id=*/2,
      /*prev_log_index=*/0,
      /*prev_log_term=*/0,
      /*entries=*/
      CreateLogEntries({
          {0, "Transaction 1"},
          {0, "Transaction 2"},
          {0, "Transaction 3"},
      }),
      /*leader_commit=*/0,
      /*follower_id=*/1);

  auto ae_message = CreateAeMessage(ae_fields);
  raft_->SetStateForTest({
      .current_term = 0,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries({}, true),
  });

  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_message)));
  EXPECT_TRUE(success);
}

// Test 2: A follower receiving multiple AppendEntries that it can accept.
TEST_F(RaftTest, FollowerAddsMultipleAppendEntries) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_TRUE(aer.success());
            EXPECT_EQ(aer.last_log_index(), 1);
            return 0;
          }))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_TRUE(aer.success());
            EXPECT_EQ(aer.last_log_index(), 2);
            return 0;
          }))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_TRUE(aer.success());
            EXPECT_EQ(aer.last_log_index(), 3);
            return 0;
          }));
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(3);

  auto ae_fields1 = CreateAeFields(
      /*term=*/0,
      /*leader_id=*/2,
      /*prev_log_index=*/0,
      /*prev_log_term=*/0,
      /*entries=*/
      CreateLogEntries({
          {0, "Transaction 1"},
      }),
      /*leader_commit=*/0,
      /*follower_id=*/1);

  auto ae_fields2 = CreateAeFields(
      /*term=*/0,
      /*leader_id=*/2,
      /*prev_log_index=*/1,
      /*prev_log_term=*/0,
      /*entries=*/
      CreateLogEntries({
          {0, "Transaction 2"},
      }),
      /*leader_commit=*/0,
      /*follower_id=*/1);

  auto ae_fields3 = CreateAeFields(
      /*term=*/0,
      /*leader_id=*/2,
      /*prev_log_index=*/2,
      /*prev_log_term=*/0,
      /*entries=*/
      CreateLogEntries({
          {0, "Transaction 3"},
      }),
      /*leader_commit=*/0,
      /*follower_id=*/1);

  auto ae_message1 = CreateAeMessage(ae_fields1);
  auto ae_message2 = CreateAeMessage(ae_fields2);
  auto ae_message3 = CreateAeMessage(ae_fields3);

  raft_->SetStateForTest({
      .current_term = 0,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries({}, true),
  });

  bool success1 = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_message1)));
  EXPECT_TRUE(success1);

  bool success2 = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_message2)));
  EXPECT_TRUE(success2);

  bool success3 = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_message3)));
  EXPECT_TRUE(success3);
}

// Test 3: A follower rejects Append Entries because its own entry at
// prev_log_index does not have the same term.
TEST_F(RaftTest, FollowerRejectsMismatchedTermAtPrevLogIndex) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_FALSE(aer.success());
            EXPECT_EQ(aer.last_log_index(), 1);
            return 0;
          }));
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(1);

  auto ae_fields = CreateAeFields(
      /*term=*/2,
      /*leader_id=*/2,
      /*prev_log_index=*/1,
      /*prev_log_term=*/2,
      /*entries=*/
      CreateLogEntries({
          {2, "Term 2 Transaction 1"},
      }),
      /*leader_commit=*/0,
      /*follower_id=*/1);

  raft_->SetStateForTest({
      .current_term = 0,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries(
          {
              {1, "Term 1 Transaction 1"},
          },
          true),
  });

  auto ae_message = CreateAeMessage(ae_fields);

  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_message)));
  EXPECT_TRUE(success);
}

// Test 4: A follower rejects Append Entries because it does not have an entry
// at prev_log_index.
TEST_F(RaftTest, FollowerRejectsMissingIndex) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_FALSE(aer.success());
            EXPECT_EQ(aer.last_log_index(), 1);
            EXPECT_EQ(aer.conflicting_index(), 0);
            EXPECT_EQ(aer.conflicting_term(), 0);
            return 0;
          }));
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(1);

  auto ae_fields = CreateAeFields(
      /*term=*/0,
      /*leader_id=*/2,
      /*prev_log_index=*/2,
      /*prev_log_term=*/0,
      /*entries=*/
      CreateLogEntries({
          {0, "Transaction 3"},
      }),
      /*leader_commit=*/0,
      /*follower_id=*/1);

  auto ae_message = CreateAeMessage(ae_fields);

  raft_->SetStateForTest({
      .current_term = 0,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries({{0, "Term 0 Transaction 1"}}, true),
  });

  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_message)));
  EXPECT_TRUE(success);
}

// Test 5: A follower receiving 1 AppendEntries with multiple entries and
// needing to truncate part of its log.
TEST_F(RaftTest, FollowerAddsAppendEntriesAndTruncatesLog) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_TRUE(aer.success());
            EXPECT_EQ(aer.last_log_index(), 3);
            return 0;
          }));
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(1);

  auto ae_fields = CreateAeFields(
      /*term=*/1,
      /*leader_id=*/2,
      /*prev_log_index=*/1,
      /*prev_log_term=*/0,
      /*entries=*/
      CreateLogEntries({
          {1, "Term 1 Transaction 1"},
          {1, "Term 1 Transaction 2"},
      }),
      /*leader_commit=*/0,
      /*follower_id=*/1);
  auto ae_message = CreateAeMessage(ae_fields);

  raft_->SetStateForTest({
      .current_term = 0,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries(
          {
              {0, "Term 0 Transaction 1"},  // index 1
              {0, "Term 0 Transaction 2"},  // mismatched entry will be removed
          },
          true),
  });

  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_message)));

  const auto& raft_log = raft_->GetLog();
  EXPECT_EQ(raft_log[0].entry.term(), 0);
  EXPECT_EQ(raft_log[0].entry.command(), "COMMON_PREFIX");
  EXPECT_EQ(raft_log[1].entry.term(), 0);
  // TODO: Use serialized string instead of manually doing it.
  EXPECT_EQ(raft_log[1].entry.command(), "\n\x14Term 0 Transaction 1");
  EXPECT_EQ(raft_log[2].entry.term(), 1);
  EXPECT_EQ(raft_log[2].entry.command(), "\n\x14Term 1 Transaction 1");
  EXPECT_EQ(raft_log[3].entry.term(), 1);
  EXPECT_EQ(raft_log[3].entry.command(), "\n\x14Term 1 Transaction 2");
  EXPECT_TRUE(success);
}

// Test 6: A follower increases its commit_index.
TEST_F(RaftTest, FollowerIncreasesCommitIndex) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_TRUE(aer.success());
            EXPECT_EQ(aer.last_log_index(), 5);
            return 0;
          }));
  EXPECT_CALL(mock_commit, Commit(_)).Times(2);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(1);

  auto ae_fields = CreateAeFields(
      /*term=*/1,
      /*leader_id=*/2,
      /*prev_log_index=*/5,
      /*prev_log_term=*/1,
      /*entries=*/CreateLogEntries({}),
      /*leader_commit=*/3,
      /*follower_id=*/1);
  auto ae_message = CreateAeMessage(ae_fields);

  raft_->SetStateForTest({
      .current_term = 1,
      .commit_index = 1,
      .last_committed = 1,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries(
          {
              {1, "Term 1 Transaction 1"},
              {1, "Term 1 Transaction 2"},
              {1, "Term 1 Transaction 3"},
              {1, "Term 1 Transaction 4"},
              {1, "Term 1 Transaction 5"},
          },
          true),
  });

  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_message)));

  EXPECT_TRUE(success);
  EXPECT_EQ(raft_->GetCommitIndex(), 3);
}

// Test 7: A follower increases its commit_index, but not past its own log
// size.
TEST_F(RaftTest, FollowerIncreasesCommitIndexCappedAtLogSize) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_TRUE(aer.success());
            EXPECT_EQ(aer.last_log_index(), 5);
            return 0;
          }));
  EXPECT_CALL(mock_commit, Commit(_)).Times(4);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(1);

  auto ae_fields = CreateAeFields(
      /*term=*/1,
      /*leader_id=*/2,
      /*prev_log_index=*/5,
      /*prev_log_term=*/1,
      /*entries=*/CreateLogEntries({}),
      /*leader_commit=*/7,
      /*follower_id=*/1);
  auto ae_message = CreateAeMessage(ae_fields);

  raft_->SetStateForTest({
      .current_term = 1,
      .commit_index = 1,
      .last_committed = 1,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries(
          {
              {1, "Term 1 Transaction 1"},
              {1, "Term 1 Transaction 2"},
              {1, "Term 1 Transaction 3"},
              {1, "Term 1 Transaction 4"},
              {1, "Term 1 Transaction 5"},
          },
          true),
  });

  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_message)));

  EXPECT_TRUE(success);
  EXPECT_EQ(raft_->GetCommitIndex(), 5);
}

// Test 8: A candidate rejecting an AppendEntries from an outdated term and
// staying candidate.
TEST_F(RaftTest, CandidateRejectsAppendEntriesFromOutdatedTerm) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_FALSE(aer.success());
            EXPECT_EQ(aer.last_log_index(), 0);
            return 0;
          }));
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(0);

  auto ae_fields = CreateAeFields(
      /*term=*/1,
      /*leader_id=*/2,
      /*prev_log_index=*/0,
      /*prev_log_term=*/0,
      /*entries=*/
      CreateLogEntries({
          {1, "Transaction 1"},
          {1, "Transaction 2"},
          {1, "Transaction 3"},
      }),
      /*leader_commit=*/0,
      /*follower_id=*/1);
  auto ae_message = CreateAeMessage(ae_fields);

  raft_->SetStateForTest({
      .current_term = 2,
      .role = Role::CANDIDATE,
      .log = CreateLogEntries({}, true),
  });

  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_message)));
  EXPECT_TRUE(success);
}

// Test 9: A candidate rejecting an AppendEntries because their log is further
// behind, but it is in the same term so they still demote.
TEST_F(RaftTest, CandidateRejectsAppendEntriesFromSameTerm) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_FALSE(aer.success());
            EXPECT_EQ(aer.last_log_index(), 1);
            return 0;
          }));
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(1);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(1);

  auto ae_fields = CreateAeFields(
      /*term=*/2,
      /*leader_id=*/2,
      /*prev_log_index=*/2,
      /*prev_log_term=*/0,
      /*entries=*/
      CreateLogEntries({
          {2, "Transaction 1"},
          {2, "Transaction 2"},
          {2, "Transaction 3"},
      }),
      /*leader_commit=*/0,
      /*follower_id=*/1);
  auto ae_message = CreateAeMessage(ae_fields);

  raft_->SetStateForTest({
      .current_term = 2,
      .role = Role::CANDIDATE,
      .log = CreateLogEntries({{1, "Old Transaction 1"}}, true),
  });

  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_message)));
  EXPECT_TRUE(success);
}

// Test 10: A candidate receiving an AppendEntries it can accept from a newer
// term.
TEST_F(RaftTest, CandidateReceivesNewerTermWithAppendEntriesItCanAccept) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_TRUE(aer.success());
            EXPECT_EQ(aer.last_log_index(), 3);
            return 0;
          }));
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(1);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(1);

  auto ae_fields = CreateAeFields(
      /*term=*/2,
      /*leader_id=*/2,
      /*prev_log_index=*/2,
      /*prev_log_term=*/0,
      /*entries=*/
      CreateLogEntries({
          {2, "Transaction 1"},
      }),
      /*leader_commit=*/2,
      /*follower_id=*/1);
  auto ae_message = CreateAeMessage(ae_fields);

  raft_->SetStateForTest({
      .current_term = 1,
      .last_committed = 2,
      .role = Role::CANDIDATE,
      .log = CreateLogEntries(
          {
              {0, "old-1"},
              {0, "old-2"},
          },
          true),
  });

  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_message)));
  EXPECT_TRUE(success);
  EXPECT_EQ(raft_->GetRoleSnapshot(), Role::FOLLOWER);
}

// Test 11: A candidate receiving an AppendEntries that it can accept from the
// same term but further along.
TEST_F(RaftTest, CandidateReceivesSameTermWithAppendEntriesItCanAccept) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_TRUE(aer.success());
            EXPECT_EQ(aer.last_log_index(), 3);
            return 0;
          }));
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(1);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(1);

  auto ae_fields = CreateAeFields(
      /*term=*/2,
      /*leader_id=*/2,
      /*prev_log_index=*/2,
      /*prev_log_term=*/0,
      /*entries=*/
      CreateLogEntries({
          {2, "Transaction 1"},
      }),
      /*leader_commit=*/2,
      /*follower_id=*/1);
  auto ae_message = CreateAeMessage(ae_fields);

  raft_->SetStateForTest({
      .current_term = 2,
      .last_committed = 2,
      .role = Role::CANDIDATE,
      .log = CreateLogEntries(
          {
              {0, "old-1"},
              {0, "old-2"},
          },
          true),
  });

  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_message)));
  EXPECT_TRUE(success);
  EXPECT_EQ(raft_->GetRoleSnapshot(), Role::FOLLOWER);
}

// Test 12: A follower receiving a leader_commit whose index is less than its
// own commit_index does not lower its commit_index.
TEST_F(RaftTest, FollowerWillNotLowerCommitIndex) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(
          ::testing::Invoke([](int type, const google::protobuf::Message& msg,
                               int node_id) { return 0; }));
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(1);

  auto ae_fields = CreateAeFields(
      /*term=*/1,
      /*leader_id=*/2,
      /*prev_log_index=*/0,
      /*prev_log_term=*/2,
      /*entries=*/
      CreateLogEntries({}),
      /*leader_commit=*/2,
      /*follower_id=*/1);
  auto ae_message = CreateAeMessage(ae_fields);

  raft_->SetStateForTest({
      .current_term = 0,
      .commit_index = 4,
      .last_committed = 0,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries(
          {
              {0, "Transaction 1"},
              {0, "Transaction 2"},
          },
          true),
  });

  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_message)));
  EXPECT_TRUE(success);
}

// Test 13: A leader ignores an AppendEntries from itself
TEST_F(RaftTest, LeaderIgnoresAppendEntriesFromSelf) {
  EXPECT_CALL(mock_call, Call(_, _, _)).Times(0);
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(0);

  auto ae_fields = CreateAeFields(
      /*term=*/0,
      /*leader_id=*/1,
      /*prev_log_index=*/0,
      /*prev_log_term=*/0,
      /*entries=*/
      CreateLogEntries({
          {0, "Transaction 1"},
      }),
      /*leader_commit=*/0,
      /*follower_id=*/1);
  auto ae_message = CreateAeMessage(ae_fields);

  raft_->SetStateForTest({
      .current_term = 0,
      .last_committed = 0,
      .role = Role::LEADER,
      .log = CreateLogEntries({}, true),
  });

  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_message)));
  EXPECT_FALSE(success);
}

// Test 14: A follower receiving a heartbeat will advance its commit index.
TEST_F(RaftTest, FollowerAdvancesCommitIndexOnHeartbeat) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(
          ::testing::Invoke([](int type, const google::protobuf::Message& msg,
                               int node_id) { return 0; }));
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(1);

  auto ae_fields = CreateAeFields(
      /*term=*/0,
      /*leader_id=*/2,
      /*prev_log_index=*/2,
      /*prev_log_term=*/0,
      /*entries=*/
      CreateLogEntries({}),
      /*leader_commit=*/2,
      /*follower_id=*/1);
  auto ae_message = CreateAeMessage(ae_fields);

  raft_->SetStateForTest({
      .current_term = 0,
      .commit_index = 0,
      .last_committed = 0,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries(
          {
              {0, "Transaction 1"},
              {0, "Transaction 2"},
          },
          true),
  });

  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_message)));
  EXPECT_TRUE(success);
  EXPECT_EQ(raft_->GetCommitIndex(), 2);
}

// Test 15: A leader correctly sends a heartbeat.
TEST_F(RaftTest, LeaderCorrectlySendsHeartbeat) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 2);
            EXPECT_EQ(ae.prev_log_index(), 2);
            EXPECT_EQ(ae.entries().size(), 0);
            return 0;
          }))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 3);
            EXPECT_EQ(ae.prev_log_index(), 1);
            EXPECT_EQ(ae.entries().size(), 1);
            return 0;
          }))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 4);
            EXPECT_EQ(ae.prev_log_index(), 0);
            EXPECT_EQ(ae.entries().size(), 2);
            return 0;
          }));
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(0);

  raft_->SetStateForTest(
      {.current_term = 1,
       .voted_for = 1,
       .commit_index = 0,
       .last_committed = 0,
       .role = Role::LEADER,
       .log = CreateLogEntries(
           {
               {0, "Transaction 1"},
               {1, "Transaction 2"},
           },
           true),
       CreateProgressPatch(
           {.next_index = std::vector<uint64_t>{1, 4, 3, 2, 1},
            .match_index = std::vector<uint64_t>{0, 2, 0, 1, 0}}),
       .votes = std::vector<int>{1, 3, 2}});

  raft_->SendHeartbeat();

  EXPECT_EQ(raft_->GetCurrentTerm(), 1);
  EXPECT_EQ(raft_->GetVotedFor(), 1);
  EXPECT_EQ(raft_->GetCommitIndex(), 0);
  EXPECT_EQ(raft_->GetLastCommitted(), 0);
  EXPECT_EQ(raft_->GetRole(), Role::LEADER);
  auto log = raft_->GetLog();
  // Maybe check that the log itself is equal
  EXPECT_EQ(log.size(), 3);
  EXPECT_EQ(raft_->GetLastLogIndex(), log.size() - 1);
  EXPECT_THAT(raft_->GetNextIndex(), ::testing::ElementsAre(_, 4, 3, 3, 3));
  EXPECT_THAT(raft_->GetMatchIndex(), ::testing::ElementsAre(_, 2, 0, 1, 0));
  EXPECT_THAT(raft_->GetVotes(), ::testing::ElementsAre(1, 3, 2));
}

// Test 16: A follower receives duplicate AppendEntries
TEST_F(RaftTest, FollowerReceivesDuplicateAppendEntries) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_TRUE(aer.success());
            EXPECT_EQ(aer.last_log_index(), 2);
            return 0;
          }));
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(1);

  auto ae_fields = CreateAeFields(
      /*term=*/2,
      /*leader_id=*/2,
      /*prev_log_index=*/1,
      /*prev_log_term=*/2,
      /*entries=*/
      CreateLogEntries({
          {2, "Term 2 Transaction 1"},
      }),
      /*leader_commit=*/0,
      /*follower_id=*/1);

  raft_->SetStateForTest({
      .current_term = 2,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries(
          {
              {2, "Term 2 Transaction 1"},
          },
          true),
  });

  auto ae_message = CreateAeMessage(ae_fields);

  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_message)));
  EXPECT_TRUE(success);
}

// Test 17: A leader ignores its own AppendEntries
TEST_F(RaftTest, LeaderIgnoresItsOwnAppendEntries) {
  EXPECT_CALL(mock_call, Call(_, _, _)).Times(0);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(0);

  auto ae_fields = CreateAeFields(
      /*term=*/2,
      /*leader_id=*/1,
      /*prev_log_index=*/1,
      /*prev_log_term=*/2,
      /*entries=*/
      CreateLogEntries({
          {2, "Term 2 Transaction 1"},
      }),
      /*leader_commit=*/0,
      /*follower_id=*/1);

  raft_->SetStateForTest({
      .current_term = 2,
      .role = Role::LEADER,
      .log = CreateLogEntries(
          {
              {2, "Term 2 Transaction 1"},
          },
          true),
  });

  auto ae_message = CreateAeMessage(ae_fields);

  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_message)));
  EXPECT_FALSE(success);
}

// Test 18: A follower receiving an AppendEntries that covers an entry already
// committed does not remove committed entries, but still adds new entries
TEST_F(RaftTest, FollowerReceivingAppendEntriesCoveringCommittedTransactions) {
  EXPECT_CALL(mock_call, Call(_, _, _)).Times(1);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(1);
  EXPECT_CALL(mock_commit, Commit(_)).Times(0);

  auto ae_fields = CreateAeFields(
      /*term=*/3,
      /*leader_id=*/2,
      /*prev_log_index=*/1,
      /*prev_log_term=*/2,
      /*entries=*/
      CreateLogEntries({
          {2, "DO NOT ADD Term 2 Transaction 2"},
          {2, "DO NOT ADD Term 2 Transaction 3"},
          {3, "Term 3 Transaction 1"},
          {3, "Term 3 Transaction 2"},
      }),
      /*leader_commit=*/3,
      /*follower_id=*/1);

  raft_->SetStateForTest({
      .current_term = 2,
      .commit_index = 3,
      .last_committed = 3,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries(
          {
              {2, "Term 2 Transaction 1"},
              {2, "Term 2 Transaction 2"},
              {2, "Term 2 Transaction 3"},
              {2, "Term 2 Transaction 4"},
          },
          true),
  });

  auto ae_message = CreateAeMessage(ae_fields);

  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_message)));
  EXPECT_TRUE(success);

  const auto& le = raft_->GetLog()[0];
  EXPECT_EQ(le.entry.term(), 0);
  EXPECT_EQ(raft_->GetLog().size(), 6);
  for (size_t i = 1; i < raft_->GetLog().size(); ++i) {
    const auto& le = raft_->GetLog()[i];
    ClientTestRequest req;
    req.ParseFromString(le.entry.command());
    if (i <= 3) {
      EXPECT_EQ(req.value(), "Term 2 Transaction " + std::to_string(i));
      EXPECT_EQ(le.entry.term(), 2);
    } else {
      EXPECT_EQ(req.value(), "Term 3 Transaction " + std::to_string(i - 3));
      EXPECT_EQ(le.entry.term(), 3);
    }
  }

  EXPECT_EQ(raft_->GetCommitIndex(), 3);
}

// Test 19: A follower performs 2 checkpoints/truncations and still does
// accurate log arithmetric after adding and truncating entries.
TEST_F(RaftTest, FollowerHasProperLogAfterCheckpoints) {
  EXPECT_CALL(mock_call, Call(_, _, _)).Times(3);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(3);
  EXPECT_CALL(mock_commit, Commit(_)).Times(1);

  // An entry that has been committed must be persisted, so while normally we
  // truncate and just use whatever is in the AppendEntries, this is unnecessary
  // for entries that have been committed. This
  auto ae_fields = CreateAeFields(
      /*term=*/3,
      /*leader_id=*/2,
      /*prev_log_index=*/1,
      /*prev_log_term=*/2,
      /*entries=*/
      CreateLogEntries({
          {2, "DO NOT ADD Term 2 Transaction 2"},
          {2, "DO NOT ADD Term 2 Transaction 3"},
          {3, "Term 3 Transaction 1"},
          {3, "Term 3 Transaction 2"},
      }),
      /*leader_commit=*/3,
      /*follower_id=*/1);

  raft_->SetStateForTest({.current_term = 2,
                          .commit_index = 3,
                          .last_committed = 3,
                          .role = Role::FOLLOWER,
                          .log = CreateLogEntries(
                              {
                                  {2, "Term 2 Transaction 1"},
                                  {2, "Term 2 Transaction 2"},
                                  {2, "Term 2 Transaction 3"},
                                  {2, "Term 2 Transaction 4"},
                              },
                              true),
                          .snapshot_buffer_amount = 0});

  auto ae_message = CreateAeMessage(ae_fields);
  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_message)));
  EXPECT_TRUE(success);

  /**
   * Log is now:
   * Sentinel
   * "Term 2 Transaction 1"
   * "Term 2 Transaction 2"
   * "Term 2 Transaction 3"
   * "Term 3 Transaction 1"
   * "Term 3 Transaction 2"
   **/

  raft_->TruncatePrefix(3);

  /**
   * Log is now:
   * Sentinel
   * "Term 3 Transaction 1"
   * "Term 3 Transaction 2"
   **/

  auto ae_fields2 = CreateAeFields(
      /*term=*/3,
      /*leader_id=*/2,
      /*prev_log_index=*/4,
      /*prev_log_term=*/3,
      /*entries=*/
      CreateLogEntries({
          {3, "Term 3 Transaction 2"},
          {3, "Term 3 Transaction 3"},
          {3, "Term 3 Transaction 4"},
      }),
      /*leader_commit=*/4,
      /*follower_id=*/1);
  auto ae_message2 = CreateAeMessage(ae_fields2);
  /**
   * Log is now:
   * Sentinel
   * "Term 3 Transaction 1"
   * "Term 3 Transaction 2"
   * "Term 3 Transaction 3"
   * "Term 3 Transaction 4"
   **/

  bool success2 = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_message2)));
  EXPECT_TRUE(success2);

  const auto& le_sentinel = raft_->GetLog()[0];
  // term of the sentinel should be the last snapshotted entry
  EXPECT_EQ(le_sentinel.entry.term(), 2);
  EXPECT_EQ(raft_->GetLog().size(), 5);
  EXPECT_EQ(raft_->GetLogicalLogSize(), 8);

  for (size_t i = 1; i < raft_->GetLog().size(); ++i) {
    const auto& le = raft_->GetLog()[i];
    ClientTestRequest req;
    req.ParseFromString(le.entry.command());
    EXPECT_EQ(req.value(), "Term 3 Transaction " + std::to_string(i));
    EXPECT_EQ(le.entry.term(), 3);
  }

  EXPECT_EQ(raft_->GetCommitIndex(), 4);
  raft_->TruncatePrefix(4);
  /**
   * Log is now:
   * Sentinel
   * "Term 3 Transaction 2"
   * "Term 3 Transaction 3"
   * "Term 3 Transaction 4"
   **/

  auto ae_fields3 = CreateAeFields(
      /*term=*/4,
      /*leader_id=*/2,
      /*prev_log_index=*/7,
      /*prev_log_term=*/3,
      /*entries=*/
      CreateLogEntries({
          {4, "Term 4 Transaction 1"},
      }),
      /*leader_commit=*/4,
      /*follower_id=*/1);
  auto ae_message3 = CreateAeMessage(ae_fields3);
  /**
   * Log is now:
   * Sentinel
   * "Term 3 Transaction 2"
   * "Term 3 Transaction 3"
   * "Term 3 Transaction 4"
   * "Term 4 Transaction 1"
   **/

  bool success3 = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_message3)));
  EXPECT_TRUE(success3);

  EXPECT_EQ(le_sentinel.entry.term(), 3);
  EXPECT_EQ(raft_->GetLog().size(), 5);
  EXPECT_EQ(raft_->GetLogicalLogSize(), 9);

  for (size_t i = 1; i < raft_->GetLog().size() - 1; ++i) {
    const auto& le = raft_->GetLog()[i];
    ClientTestRequest req;
    req.ParseFromString(raft_->GetLogEntryAtIndex(i + 4).entry.command());
    ClientTestRequest req2;
    req2.ParseFromString(le.entry.command());
    EXPECT_EQ(req.value(), "Term 3 Transaction " + std::to_string(i + 1));
    EXPECT_EQ(req.value(), req2.value());
    EXPECT_EQ(le.entry.term(), 3);
    EXPECT_EQ(raft_->GetLogTermAtIndex(i + 4), 3);
  }
  const auto& le = raft_->GetLog()[4];
  ClientTestRequest req;
  req.ParseFromString(raft_->GetLogEntryAtIndex(8).entry.command());
  ClientTestRequest req2;
  req2.ParseFromString(le.entry.command());
  EXPECT_EQ(req.value(), "Term 4 Transaction 1");
  EXPECT_EQ(req.value(), req2.value());
  EXPECT_EQ(le.entry.term(), 4);
  EXPECT_EQ(raft_->GetLogTermAtIndex(8), 4);

  EXPECT_EQ(raft_->GetTruncatedLastIndex(), 4);
  EXPECT_DEATH(raft_->GetLogEntryAtIndex(4),
               "Tried to access entry that has been prefix truncated");
  EXPECT_DEATH(raft_->GetLogEntryAtIndex(9),
               "Tried to access element that has not been added yet");
  EXPECT_DEATH(
      raft_->TruncatePrefix(5),
      "Tried to prefix truncate an element that has not been committed");
}

// Test 20: A follower receives an AppendEntries containing entries that had
// already been truncated from its log.
TEST_F(RaftTest, FollowerReceivesAppendEntriesWithOnlySnapshottedEntries) {
  EXPECT_CALL(*recovery_, WriteMetadata(_, _, _, _)).Times(AnyNumber());
  EXPECT_CALL(*recovery_, AddLogEntry(::testing::An<const Entry*>()))
      .Times(AnyNumber());
  EXPECT_CALL(*recovery_, AddLogEntry(::testing::An<std::vector<Entry>&>()))
      .Times(AnyNumber());
  EXPECT_CALL(*recovery_, TruncateLog(_)).Times(AnyNumber());

  raft_->SetStateForTest({
      .current_term = 2,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries({}, true),
  });
  // Follower has snapshot up to index 3
  raft_->SetSnapshotLastIndexAndTerm(3, 2, false);

  AppendEntriesResponse aer;
  EXPECT_CALL(mock_call, Call(MessageType::AppendEntriesResponseMsg, _, _))
      .WillOnce(Invoke([&](int, const google::protobuf::Message& msg, int) {
        aer = dynamic_cast<const AppendEntriesResponse&>(msg);
        return 0;
      }));
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(AnyNumber());

  auto ae_fields = CreateAeFields(
      /*term=*/2,
      /*leader_id=*/2,
      /*prev_log_index=*/1,
      /*prev_log_term=*/2, CreateLogEntries({{2, "Entry2"}, {2, "Entry3"}}),
      /*leader_commit=*/1,
      /*follower_id=*/1);
  auto ae_msg = CreateAeMessage(ae_fields);

  bool accepted_ae = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_msg)));

  EXPECT_TRUE(accepted_ae);
  EXPECT_TRUE(aer.success());
  EXPECT_EQ(aer.term(), 2);
  EXPECT_EQ(aer.last_log_index(), 3u);
}

// Test 21: A follower receives an AppendEntries continuing right after its last
// snapshot with an otherwise empty log.
TEST_F(RaftTest,
       FollowerReceivesAppendEntriesDirectlyAfterCheckpointTruncation) {
  EXPECT_CALL(*recovery_, WriteMetadata(_, _, _, _)).Times(AnyNumber());
  EXPECT_CALL(*recovery_, AddLogEntry(::testing::An<const Entry*>()))
      .Times(AnyNumber());
  EXPECT_CALL(*recovery_, AddLogEntry(::testing::An<std::vector<Entry>&>()))
      .Times(AnyNumber());
  EXPECT_CALL(*recovery_, TruncateLog(_)).Times(AnyNumber());

  raft_->SetStateForTest({
      .current_term = 2,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries({}, true),
  });
  // Follower has snapshot up to index 3
  raft_->SetSnapshotLastIndexAndTerm(3, 2, false);

  AppendEntriesResponse aer;
  EXPECT_CALL(mock_call, Call(MessageType::AppendEntriesResponseMsg, _, _))
      .WillOnce(Invoke([&](int, const google::protobuf::Message& msg, int) {
        aer = dynamic_cast<const AppendEntriesResponse&>(msg);
        return 0;
      }));
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(AnyNumber());

  auto ae_fields = CreateAeFields(
      /*term=*/2,
      /*leader_id=*/2,
      /*prev_log_index=*/3,
      /*prev_log_term=*/2, CreateLogEntries({{2, "Entry4"}, {2, "Entry5"}}),
      /*leader_commit=*/1,
      /*follower_id=*/1);
  auto ae_msg = CreateAeMessage(ae_fields);

  bool accepted_ae = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_msg)));

  EXPECT_TRUE(accepted_ae);
  EXPECT_TRUE(aer.success());
  EXPECT_EQ(aer.term(), 2);
  EXPECT_EQ(aer.last_log_index(), 5u);
}

// Test 22: A leader sends a heartbeat to a follower even if that follower is
// over the in flight message size limit.
TEST_F(RaftTest, LeaderSendsHeartbeatToFollowerOverInFlightLimit) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .Times(3)
      .WillRepeatedly(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(ae.entries().size(), 0);
            return 0;
          }));

  auto now = std::chrono::steady_clock::now();
  // Fill all in-flight slots for every follower
  std::vector<std::vector<InFlightMsg>> in_flight_vecs(5);
  for (int follower_id = 2; follower_id <= 4; ++follower_id) {
    for (size_t i = 0; i < raft_->GetMaxInFlightVecs(); ++i) {
      InFlightMsg msg;
      msg.time_sent = now;
      msg.prev_log_index_sent = i;
      msg.last_index_of_segment_sent = i + 1;
      in_flight_vecs[follower_id].push_back(msg);
    }
  }

  raft_->SetStateForTest(
      {.current_term = 1,
       .role = Role::LEADER,
       .log = CreateLogEntries(
           {
               {1, "Transaction 1"},
               {1, "Transaction 2"},
               {1, "Transaction 3"},
           },
           true),
       CreateProgressPatch({
           .next_index = std::vector<uint64_t>{1, 4, 4, 4, 4},
           .match_index = std::vector<uint64_t>{0, 3, 3, 3, 3},
           .in_flight = in_flight_vecs,
       })});

  for (int follower_id = 2; follower_id <= 4; ++follower_id) {
    EXPECT_FALSE(raft_->CanSendLocked(follower_id));
  }

  raft_->SendHeartbeat();

  // In-flight vecs should be unchanged.
  auto result = raft_->GetFollowerProgress();
  for (int follower_id = 2; follower_id <= 4; ++follower_id) {
    EXPECT_EQ(result[follower_id].in_flight.size(),
              raft_->GetMaxInFlightVecs());
  }
}

// Test 23: A follower receiving 1 AppendEntries containing entries with a
// prev_log_index that it does have in its log, but not a matching term. This
// will go back to the beginning of the log.
TEST_F(RaftTest, FollowerProperlySetsConflictingTermAndIndexInAERRejection) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_FALSE(aer.success());
            EXPECT_EQ(aer.last_log_index(), 4);
            EXPECT_EQ(aer.conflicting_index(), 0);
            EXPECT_EQ(aer.conflicting_term(), 0);
            return 0;
          }));
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(1);

  auto ae_fields = CreateAeFields(
      /*term=*/1,
      /*leader_id=*/2,
      /*prev_log_index=*/4,
      /*prev_log_term=*/1,
      /*entries=*/
      CreateLogEntries({
          {1, "Term 1 Transaction 3"},
          {1, "Term 1 Transaction 4"},
      }),
      /*leader_commit=*/0,
      /*follower_id=*/1);
  auto ae_message = CreateAeMessage(ae_fields);

  raft_->SetStateForTest({
      .current_term = 0,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries(
          {
              {0, "Term 0 Transaction 1"},
              {0, "Term 0 Transaction 2"},
              {0, "Term 0 Transaction 3"},
              {0, "Term 0 Transaction 4"},
          },
          true),
  });

  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_message)));

  // Log is unchanged
  EXPECT_EQ(raft_->GetLogicalLogSize(), 5);
  EXPECT_TRUE(success);
}

// Test 24: A follower receiving 1 AppendEntries containing entries with a
// prev_log_index that it does have in its log, but not a matching term. This
// will return the first entry with term 1.
TEST_F(RaftTest, FollowerProperlySetsConflictingTermAndIndexInAERRejection2) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_FALSE(aer.success());
            EXPECT_EQ(aer.last_log_index(), 4);
            EXPECT_EQ(aer.conflicting_index(), 3);
            EXPECT_EQ(aer.conflicting_term(), 1);
            return 0;
          }));
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(1);

  auto ae_fields = CreateAeFields(
      /*term=*/2,
      /*leader_id=*/2,
      /*prev_log_index=*/4,
      /*prev_log_term=*/2,
      /*entries=*/
      CreateLogEntries({
          {2, "Term 2 Transaction 2"},
          {2, "Term 2 Transaction 3"},
      }),
      /*leader_commit=*/0,
      /*follower_id=*/1);
  auto ae_message = CreateAeMessage(ae_fields);

  raft_->SetStateForTest({
      .current_term = 1,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries(
          {
              {0, "Term 0 Transaction 1"},
              {0, "Term 0 Transaction 2"},
              {1, "Term 1 Transaction 1"},
              {1, "Term 1 Transaction 2"},
          },
          true),
  });

  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_message)));

  // Log is unchanged
  EXPECT_EQ(raft_->GetLogicalLogSize(), 5);
  EXPECT_TRUE(success);
}

// Test 25: A follower receiving 1 AppendEntries containing entries with a
// prev_log_index that it does not have. It leaves conflicting_index and term at
// 0.
TEST_F(RaftTest, FollowerLeavesConflictingIndexAndTermAt0WhenLogIsShort) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_FALSE(aer.success());
            EXPECT_EQ(aer.last_log_index(), 4);
            EXPECT_EQ(aer.conflicting_index(), 0);
            EXPECT_EQ(aer.conflicting_term(), 0);
            return 0;
          }));
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(1);

  auto ae_fields = CreateAeFields(
      /*term=*/1,
      /*leader_id=*/2,
      /*prev_log_index=*/5,
      /*prev_log_term=*/1,
      /*entries=*/
      CreateLogEntries({
          {1, "Term 1 Transaction 4"},
          {1, "Term 1 Transaction 5"},
      }),
      /*leader_commit=*/0,
      /*follower_id=*/1);
  auto ae_message = CreateAeMessage(ae_fields);

  raft_->SetStateForTest({
      .current_term = 0,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries(
          {
              {0, "Term 0 Transaction 1"},
              {0, "Term 0 Transaction 2"},
              {1, "Term 1 Transaction 1"},
              {1, "Term 1 Transaction 2"},
          },
          true),
  });

  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_message)));

  // Log is unchanged
  EXPECT_EQ(raft_->GetLogicalLogSize(), 5);
  EXPECT_TRUE(success);
}

// Test 26: A follower receiving an AppendEntries it can accept from a newer
// term.
TEST_F(RaftTest, FollowerReceivesNewerTermWithAppendEntriesItCanAccept) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_TRUE(aer.success());
            EXPECT_EQ(aer.last_log_index(), 3);
            return 0;
          }));
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(1);

  auto ae_fields = CreateAeFields(
      /*term=*/2,
      /*leader_id=*/2,
      /*prev_log_index=*/2,
      /*prev_log_term=*/0,
      /*entries=*/
      CreateLogEntries({
          {2, "Transaction 1"},
      }),
      /*leader_commit=*/2,
      /*follower_id=*/1);
  auto ae_message = CreateAeMessage(ae_fields);

  raft_->SetStateForTest({
      .current_term = 1,
      .last_committed = 2,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries(
          {
              {0, "old-1"},
              {0, "old-2"},
          },
          true),
  });

  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_message)));
  EXPECT_TRUE(success);
  EXPECT_EQ(raft_->GetRoleSnapshot(), Role::FOLLOWER);
}

// Test 27: A follower does not truncate its log upon heartbeat.
TEST_F(RaftTest, FollowerDoesNotTruncateLogUponHearbeat) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_TRUE(aer.success());
            EXPECT_EQ(aer.last_log_index(), 4);
            return 0;
          }));
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(1);

  auto ae_fields = CreateAeFields(
      /*term=*/1,
      /*leader_id=*/2,
      /*prev_log_index=*/2,
      /*prev_log_term=*/0,
      /*entries=*/
      CreateLogEntries({}),
      /*leader_commit=*/0,
      /*follower_id=*/1);
  auto ae_message = CreateAeMessage(ae_fields);

  raft_->SetStateForTest({
      .current_term = 0,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries(
          {
              {0, "Term 0 Transaction 1"},
              {0, "Term 0 Transaction 2"},
              {0, "Term 0 Transaction 3"},
              {0, "Term 0 Transaction 4"},
          },
          true),
  });

  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_message)));

  EXPECT_EQ(raft_->GetLogicalLogSize(), 5);
  EXPECT_TRUE(success);
}

}  // namespace raft
}  // namespace resdb

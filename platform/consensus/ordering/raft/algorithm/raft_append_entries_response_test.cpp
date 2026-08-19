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

// Test 1: A leader receiving an AppendEntriesResponse success and updating the
// follower's match_index.
TEST_F(RaftTest, LeaderReceivesAppendEntriesResponseSuccess) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);

  AppendEntriesResponse ae_response;
  ae_response.set_success(true);
  ae_response.set_term(1);
  ae_response.set_id(2);
  ae_response.set_last_log_index(2);

  raft_->SetStateForTest(
      {.current_term = 1,
       .commit_index = 0,
       .role = Role::LEADER,
       .log = CreateLogEntries(
           {
               {0, "Transaction 1"},
               {0, "Transaction 2"},
           },
           true),
       CreateProgressPatch(
           {.match_index = std::vector<uint64_t>{0, 2, 0, 0, 0}})});

  bool success = raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(ae_response));
  EXPECT_TRUE(success);
  EXPECT_THAT(raft_->GetMatchIndex(), ::testing::ElementsAre(_, 2, 2, 0, 0));
}

// Test 2: A leader receiving an AppendEntriesResponse from a follower that in a
// newer term.
TEST_F(RaftTest, LeaderReceivesAppendEntriesResponseFromNewerTerm) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(1);

  raft_->SetStateForTest({
      .current_term = 1,
      .role = Role::LEADER,
  });

  AppendEntriesResponse ae_response;
  ae_response.set_success(false);
  ae_response.set_term(2);

  bool success = raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(ae_response));
  EXPECT_FALSE(success);
  EXPECT_EQ(raft_->GetRoleSnapshot(), Role::FOLLOWER);
}

// Test 3: A leader receiving an AppendEntriesResponse success, updating the
// follower's match_index, and committing a new entry.
TEST_F(RaftTest, LeaderReceivesAppendEntriesResponseSuccessAndCommits) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(mock_commit, Commit(_)).Times(1);

  AppendEntriesResponse ae_response;
  ae_response.set_success(true);
  ae_response.set_term(1);
  ae_response.set_id(2);
  ae_response.set_last_log_index(2);

  raft_->SetStateForTest(
      {.current_term = 1,
       .commit_index = 0,
       .last_committed = 0,
       .role = Role::LEADER,
       .log = CreateLogEntries(
           {
               {1, "Transaction 1"},
               {1, "Transaction 2"},
           },
           true),
       CreateProgressPatch(
           {.next_index = std::vector<uint64_t>{1, 2, 2, 2, 2},
            .match_index = std::vector<uint64_t>{0, 2, 0, 1, 0}})});

  bool success = raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(ae_response));
  EXPECT_TRUE(success);
  EXPECT_THAT(raft_->GetMatchIndex(), ::testing::ElementsAre(_, 2, 2, 1, 0));
  EXPECT_EQ(raft_->GetCommitIndex(), 1);
}

// Test 4: A leader receiving an AppendEntriesResponse success and catching up a
// follower that is behind.
TEST_F(RaftTest, LeaderCatchesUpFollowerThatIsBehind) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(ae.entries_size(), 1);
            // TODO: Use serialized string instead of manually doing it.
            EXPECT_EQ(ae.entries(0).command(), "\n\rTransaction 2");
            EXPECT_EQ(node_id, 2);
            return 0;
          }));

  AppendEntriesResponse ae_response;
  ae_response.set_success(true);
  ae_response.set_term(1);
  ae_response.set_id(2);
  ae_response.set_last_log_index(1);
  ae_response.set_conflicting_index(0);
  ae_response.set_conflicting_term(0);

  raft_->SetStateForTest({
      .current_term = 1,
      .commit_index = 0,
      .last_committed = 0,
      .role = Role::LEADER,
      .snapshot_last_index = 0,
      .snapshot_last_term = 0,
      .truncated_last_index = 0,
      .truncated_last_term = 0,
      .log = CreateLogEntries(
          {
              {1, "Transaction 1"},
              {1, "Transaction 2"},
          },
          true),
  });

  bool success = raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(ae_response));
  EXPECT_TRUE(success);
}

// Test 6: A follower ignores an AppendEntriesResponse.
TEST_F(RaftTest, FollowerIgnoresAppendEntriesResponse) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(mock_call, Call(_, _, _)).Times(0);

  AppendEntriesResponse ae_response;
  ae_response.set_term(1);

  raft_->SetStateForTest({
      .current_term = 1,
      .role = Role::FOLLOWER,
  });

  bool success = raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(ae_response));
  EXPECT_TRUE(success);
}

// Test 7: A leader ignores an AppendEntriesResponse from an outdated term. The
// leader's state should not change.
TEST_F(RaftTest, LeaderIgnoresAppendEntriesResponseFromOutdatedTerm) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(mock_call, Call(_, _, _)).Times(0);

  raft_->SetStateForTest(
      {.current_term = 3,
       .commit_index = 3,
       .last_committed = 3,
       .role = Role::LEADER,
       .log = CreateLogEntries({{2, "Transaction 1"},
                                {2, "Transaction 2"},
                                {2, "Transaction 3"},
                                {2, "Transaction 4"},
                                {3, "RAFT_NO_OP"}},
                               true),
       CreateProgressPatch({.next_index = std::vector<uint64_t>{1, 6, 6, 6, 6},
                            .match_index = std::vector<uint64_t>{0, 5, 5, 0, 0},
                            .states = std::vector<ProgressState>{
                                ProgressState::PROBE, ProgressState::PROBE,
                                ProgressState::PROBE, ProgressState::PROBE,
                                ProgressState::PROBE}})});

  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);

  AppendEntriesResponse aer;
  aer.set_success(false);
  aer.set_term(2);
  aer.set_id(3);
  aer.set_last_log_index(10);

  raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(aer));

  const auto& follower_progress = raft_->GetFollowerProgress();
  EXPECT_EQ(follower_progress[3].state, ProgressState::PROBE);

  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 2);
            EXPECT_EQ(ae.prev_log_index(), 5);
            EXPECT_EQ(ae.entries().size(), 0);
            return 0;
          }))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 3);
            EXPECT_EQ(ae.prev_log_index(), 5);
            EXPECT_EQ(ae.entries().size(), 0);
            return 0;
          }))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 4);
            EXPECT_EQ(ae.prev_log_index(), 5);
            EXPECT_EQ(ae.entries().size(), 0);
            return 0;
          }));

  raft_->SendHeartbeat();
}

// Test 8: A leader does not advance its commit index from a previous term if it
// has not replicated an entry from its current term.
TEST_F(RaftTest,
       LeaderReceivesAppendEntriesResponseSuccessAndDoesNotCommitOldTerm) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(mock_commit, Commit(_)).Times(0);

  AppendEntriesResponse ae_response;
  ae_response.set_success(true);
  ae_response.set_term(1);
  ae_response.set_id(2);
  ae_response.set_last_log_index(2);

  raft_->SetStateForTest(
      {.current_term = 1,
       .commit_index = 0,
       .last_committed = 0,
       .role = Role::LEADER,
       .log = CreateLogEntries(
           {
               {0, "Transaction 1"},
               {0, "Transaction 2"},
           },
           true),
       CreateProgressPatch(
           {.next_index = std::vector<uint64_t>{0, 2, 2, 2, 2},
            .match_index = std::vector<uint64_t>{0, 2, 0, 1, 0}})});

  bool success = raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(ae_response));
  EXPECT_TRUE(success);
  EXPECT_THAT(raft_->GetMatchIndex(), ::testing::ElementsAre(_, 2, 2, 1, 0));
  EXPECT_EQ(raft_->GetCommitIndex(), 0);
}

// Test 9: A leader receiving an AppendEntriesResponse success, updating the
// follower's match_index, and not committing the entry.
TEST_F(RaftTest, LeaderReceivesAppendEntriesResponseSuccessAndDoesNotCommit) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(mock_commit, Commit(_)).Times(0);

  AppendEntriesResponse ae_response;
  ae_response.set_success(true);
  ae_response.set_term(1);
  ae_response.set_id(2);
  ae_response.set_last_log_index(2);

  raft_->SetStateForTest(
      {.current_term = 1,
       .commit_index = 0,
       .last_committed = 0,
       .role = Role::LEADER,
       .log = CreateLogEntries(
           {
               {1, "Transaction 1"},
               {1, "Transaction 2"},
           },
           true),
       CreateProgressPatch(
           {.next_index = std::vector<uint64_t>{1, 2, 2, 2, 2},
            .match_index = std::vector<uint64_t>{0, 0, 0, 1, 0}})});

  bool success = raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(ae_response));
  EXPECT_TRUE(success);
  EXPECT_THAT(raft_->GetMatchIndex(), ::testing::ElementsAre(_, 0, 2, 1, 0));
  EXPECT_EQ(raft_->GetCommitIndex(), 0);
}

// Test 10: A leader receiving an AppendEntriesResponse success with a lower
// last_log_index than the match_index corresponding to that follower does not
// lower that match_index.
TEST_F(RaftTest, LeaderReceivingOutOfDateAERDoesNotLowerMatchIndex) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(mock_commit, Commit(_)).Times(0);

  AppendEntriesResponse ae_response;
  ae_response.set_success(true);
  ae_response.set_term(1);
  ae_response.set_id(2);
  ae_response.set_last_log_index(1);

  raft_->SetStateForTest(
      {.current_term = 1,
       .commit_index = 0,
       .last_committed = 0,
       .role = Role::LEADER,
       .log = CreateLogEntries(
           {
               {1, "Transaction 1"},
               {1, "Transaction 2"},
           },
           true),
       CreateProgressPatch(
           {.next_index = std::vector<uint64_t>{1, 2, 2, 2, 2},
            .match_index = std::vector<uint64_t>{0, 0, 2, 1, 0}})});

  bool success = raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(ae_response));
  EXPECT_TRUE(success);
  EXPECT_THAT(raft_->GetMatchIndex(), ::testing::ElementsAre(_, 0, 2, 1, 0));
  EXPECT_EQ(raft_->GetCommitIndex(), 0);
}

// Test 11: A leader receiving an AppendEntriesResponse success does not commit
// an entry from a previous term (without committing it transitively via a
// commit from its own term)
TEST_F(RaftTest, LeaderReceivingAERDoesNotCommitFromPreviousTerm) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(mock_commit, Commit(_)).Times(0);

  AppendEntriesResponse ae_response;
  ae_response.set_success(true);
  ae_response.set_term(2);
  ae_response.set_id(2);
  ae_response.set_last_log_index(1);

  raft_->SetStateForTest(
      {.current_term = 2,
       .commit_index = 0,
       .last_committed = 0,
       .role = Role::LEADER,
       .log = CreateLogEntries(
           {
               {1, "Transaction 1"},
               {1, "Transaction 2"},
           },
           true),
       CreateProgressPatch(
           {.next_index = std::vector<uint64_t>{1, 2, 2, 2, 2},
            .match_index = std::vector<uint64_t>{0, 2, 0, 1, 0}})});

  bool success = raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(ae_response));
  EXPECT_TRUE(success);
  EXPECT_THAT(raft_->GetMatchIndex(), ::testing::ElementsAre(_, 2, 1, 1, 0));
  EXPECT_EQ(raft_->GetCommitIndex(), 0);
}

// Test 12: A leader receiving an AppendEntriesResponse from a follower whose
// log is longer and does not crash.
TEST_F(RaftTest, LeaderReceivesAppendEntriesResponseFromLongerLog) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);

  AppendEntriesResponse ae_response;
  ae_response.set_success(false);
  ae_response.set_term(1);
  ae_response.set_id(2);
  ae_response.set_last_log_index(8);

  raft_->SetStateForTest(
      {.current_term = 1,
       .commit_index = 0,
       .role = Role::LEADER,
       .log = CreateLogEntries(
           {
               {0, "Transaction 1"},
               {0, "Transaction 2"},
           },
           true),
       CreateProgressPatch(
           {.next_index = std::vector<uint64_t>{0, 2, 0, 0, 0},
            .match_index = std::vector<uint64_t>{0, 2, 0, 0, 0}})});

  bool success = raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(ae_response));
  EXPECT_TRUE(success);
  EXPECT_THAT(raft_->GetMatchIndex(), ::testing::ElementsAre(_, 2, 0, 0, 0));
  EXPECT_THAT(raft_->GetNextIndex(), ::testing::ElementsAre(_, 2, 3, 0, 0));
}

// Test 13: A leader receiving an out of order AppendEntriesResponse does not
// decrease a follower's next_index below 1 + match_index.
TEST_F(RaftTest, AppendEntriesResponseDoesNotDecreaseNextIndexBelowMatchIndex) {
  raft_->SetStateForTest(
      {.current_term = 2,
       .commit_index = 3,
       .last_committed = 3,
       .role = Role::LEADER,
       .log = CreateLogEntries({{2, "Transaction 1"},
                                {2, "Transaction 2"},
                                {2, "Transaction 3"},
                                {2, "Transaction 4"}},
                               true),
       CreateProgressPatch(
           {.next_index = std::vector<uint64_t>{1, 5, 4, 5, 5},
            .match_index = std::vector<uint64_t>{0, 3, 3, 3, 3}})});

  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  // Whenever a leader receives a stale/out of order AppendEntriesResponse, it
  // is ignored and nothing is sent out.
  EXPECT_CALL(mock_call, Call(_, _, _)).Times(0);

  // Stale success AER from follower 2 reporting last_log_index=2
  // (older than the already-acknowledged match_index of 3).
  AppendEntriesResponse aer;
  aer.set_success(true);
  aer.set_term(2);
  aer.set_id(2);
  aer.set_last_log_index(2);

  raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(aer));

  EXPECT_EQ(raft_->GetMatchIndex()[2], 3u) << "match_index must never decrease";
  EXPECT_GE(raft_->GetNextIndex()[2], 4u)
      << "next_index for a follower must never decrease below 1 + its "
         "match_index";
}

// Test 14: A leader transitively commits entries from a previous term using its
// NO-OP.
TEST_F(RaftTest, LeaderUsesNoOpToTransitivelyCommitOldEntries) {
  raft_->SetStateForTest(
      {.current_term = 3,
       .commit_index = 3,
       .last_committed = 3,
       .role = Role::LEADER,
       .log = CreateLogEntries({{2, "Transaction 1"},
                                {2, "Transaction 2"},
                                {2, "Transaction 3"},
                                {2, "Transaction 4"},
                                {3, "RAFT_NO_OP"}},
                               true),
       CreateProgressPatch(
           {.next_index = std::vector<uint64_t>{1, 6, 6, 6, 6},
            .match_index = std::vector<uint64_t>{0, 5, 5, 0, 0}})});

  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(mock_call, Call(_, _, _)).Times(0);

  AppendEntriesResponse aer;
  aer.set_success(true);
  aer.set_term(3);
  aer.set_id(3);
  aer.set_last_log_index(5);

  raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(aer));

  EXPECT_EQ(raft_->GetCommitIndex(), 5u);
}

// ProgressState tests

// Test 15: Upon AppendEntriesResponse failure, the follower gets set to PROBE,
// and gets its in flight messages cleared.
TEST_F(RaftTest, FollowerGoesFromReplicateToProbeAfterFailure) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(ae.entries_size(), 0);
            EXPECT_EQ(node_id, 3);
            return 0;
          }));

  // Fill all in-flight slots for every follower
  std::vector<std::vector<InFlightMsg>> in_flight_vecs(5);
  auto now = std::chrono::steady_clock::now();
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
      {.current_term = 3,
       .commit_index = 3,
       .last_committed = 3,
       .role = Role::LEADER,
       .log = CreateLogEntries({{2, "Transaction 1"},
                                {2, "Transaction 2"},
                                {2, "Transaction 3"},
                                {2, "Transaction 4"}},
                               true),
       CreateProgressPatch(
           {.next_index = std::vector<uint64_t>{1, 6, 6, 6, 6},
            .match_index = std::vector<uint64_t>{0, 5, 5, 0, 0},
            .in_flight = in_flight_vecs,
            .states = std::vector<ProgressState>{
                ProgressState::REPLICATE, ProgressState::REPLICATE,
                ProgressState::REPLICATE, ProgressState::REPLICATE,
                ProgressState::REPLICATE}})});

  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);

  AppendEntriesResponse aer;
  aer.set_success(false);
  aer.set_term(3);
  aer.set_id(3);
  aer.set_last_log_index(2);

  raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(aer));

  const auto& follower_progress = raft_->GetFollowerProgress();
  EXPECT_EQ(follower_progress[3].state, ProgressState::PROBE);
  EXPECT_EQ(follower_progress[3].in_flight.size(), 0);
  EXPECT_TRUE(follower_progress[3].probe_in_flight);
}

// Test 17: A follower in PROBE state does not get sent any messages except
// heartbeats until it responds.
TEST_F(RaftTest, FollowerInProbeStateDoesNotGetTransactions) {
  raft_->SetStateForTest(
      {.current_term = 3,
       .commit_index = 3,
       .last_committed = 3,
       .role = Role::LEADER,
       .log = CreateLogEntries({{2, "Transaction 1"},
                                {2, "Transaction 2"},
                                {2, "Transaction 3"},
                                {2, "Transaction 4"},
                                {3, "RAFT_NO_OP"}},
                               true),
       CreateProgressPatch(
           {.next_index = std::vector<uint64_t>{1, 6, 6, 6, 6},
            .match_index = std::vector<uint64_t>{0, 5, 5, 0, 0}})});

  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(mock_call, Call(_, _, _)).Times(0);

  AppendEntriesResponse aer;
  aer.set_success(true);
  aer.set_term(3);
  aer.set_id(3);
  aer.set_last_log_index(5);

  raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(aer));

  EXPECT_EQ(raft_->GetCommitIndex(), 5u);
}

// Test 18: A follower in PROBE sends the leader a success, and transitions back
// to REPLICATE.
TEST_F(RaftTest, FollowerRespondingToProbeWithSuccessTransitionsToReplicate) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(ae.entries_size(), 3);
            EXPECT_EQ(node_id, 3);
            return 0;
          }));

  raft_->SetStateForTest(
      {.current_term = 3,
       .commit_index = 3,
       .last_committed = 3,
       .role = Role::LEADER,
       .log = CreateLogEntries({{2, "Transaction 1"},
                                {2, "Transaction 2"},
                                {2, "Transaction 3"},
                                {2, "Transaction 4"},
                                {3, "RAFT_NO_OP"}},
                               true),
       CreateProgressPatch({.next_index = std::vector<uint64_t>{1, 6, 6, 6, 6},
                            .match_index = std::vector<uint64_t>{0, 5, 5, 0, 0},
                            .states = std::vector<ProgressState>{
                                ProgressState::PROBE, ProgressState::PROBE,
                                ProgressState::PROBE, ProgressState::PROBE,
                                ProgressState::PROBE}})});

  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);

  AppendEntriesResponse aer;
  aer.set_success(true);
  aer.set_term(3);
  aer.set_id(3);
  aer.set_last_log_index(2);

  raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(aer));

  const auto& follower_progress = raft_->GetFollowerProgress();
  EXPECT_EQ(follower_progress[3].state, ProgressState::REPLICATE);
}

// Test 19: A follower whose log is ahead does not crash the leader upon
// failure. Note: There is no need to test the success case, because leader's
// add a no-op entry at the start of their term. If a leader receives a
// successful AER from a follower, it must have added at least the no-op at the
// start of the term, meaning its log cannot be further ahead.
TEST_F(RaftTest, FollowerFailureWhoseLogIsAheadDoesNotCrashSameTerm) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 3);
            EXPECT_EQ(ae.prev_log_index(), 4);
            EXPECT_EQ(ae.entries().size(), 0);
            return 0;
          }));

  raft_->SetStateForTest(
      {.current_term = 3,
       .commit_index = 3,
       .last_committed = 3,
       .role = Role::LEADER,
       .log = CreateLogEntries({{2, "Transaction 1"},
                                {2, "Transaction 2"},
                                {2, "Transaction 3"},
                                {2, "Transaction 4"},
                                {3, "RAFT_NO_OP"}},
                               true),
       CreateProgressPatch(
           {.next_index = std::vector<uint64_t>{1, 6, 6, 6, 6},
            .match_index = std::vector<uint64_t>{0, 5, 5, 0, 0},
            .states = std::vector<ProgressState>{
                ProgressState::PROBE, ProgressState::REPLICATE,
                ProgressState::REPLICATE, ProgressState::REPLICATE,
                ProgressState::PROBE}})});

  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);

  AppendEntriesResponse aer;
  aer.set_success(false);
  aer.set_term(3);
  aer.set_id(3);
  aer.set_last_log_index(10);
  aer.set_conflicting_index(1);
  aer.set_conflicting_term(2);

  raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(aer));

  const auto& follower_progress = raft_->GetFollowerProgress();
  EXPECT_EQ(follower_progress[3].state, ProgressState::PROBE);
}

// Note for the following tests, conflictingIndex <= last_log_index

// Test 20: Follower whose:
// conflicting_index < truncated_last_index_
// last_log_index < truncated_last_index_
// gets a snapshot.
TEST_F(RaftTest, FollowerConflictingIndexLastLogIndexCase1) {
  EXPECT_CALL(mock_call, Call(_, _, _)).Times(0);

  raft_->SetStateForTest({
      .current_term = 3,
      .commit_index = 3,
      .last_committed = 3,
      .role = Role::LEADER,
      .snapshot_last_index = 3,
      .snapshot_last_term = 2,
      .truncated_last_index = 2,
      .truncated_last_term = 2,
      .log = CreateLogEntries(
          {//{2, "Transaction 1"},  Truncated
           //{2, "Transaction 2"},  Truncated
           {2, "Transaction 3"},
           {2, "Transaction 4"},
           {3, "RAFT_NO_OP"}},
          true),
      CreateProgressPatch(
          {.next_index = std::vector<uint64_t>{1, 6, 6, 6, 6},
           .match_index = std::vector<uint64_t>{0, 5, 5, 0, 0},
           .states = std::vector<ProgressState>{ProgressState::PROBE,
                                                ProgressState::REPLICATE,
                                                ProgressState::REPLICATE,
                                                ProgressState::REPLICATE,
                                                ProgressState::PROBE}}),
  });

  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);

  AppendEntriesResponse aer;
  aer.set_success(false);
  aer.set_term(3);
  aer.set_id(3);
  aer.set_last_log_index(1);
  aer.set_conflicting_index(1);
  aer.set_conflicting_term(2);

  raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(aer));

  const auto& follower_progress = raft_->GetFollowerProgress();
  EXPECT_EQ(follower_progress[3].state, ProgressState::SNAPSHOT);

  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 2);
            EXPECT_EQ(ae.prev_log_index(), 5);
            EXPECT_EQ(ae.entries().size(), 0);
            return 0;
          }))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 3);
            EXPECT_EQ(
                ae.prev_log_index(),
                2);  // This value cannot be less than truncated_last_index
            EXPECT_EQ(ae.entries().size(), 0);
            return 0;
          }))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 4);
            EXPECT_EQ(ae.prev_log_index(), 5);
            EXPECT_EQ(ae.entries().size(), 0);
            return 0;
          }));

  raft_->SendHeartbeat();
  auto follower_progress2 = raft_->GetFollowerProgress();
  EXPECT_EQ(follower_progress2[3].state, ProgressState::SNAPSHOT);
}

// Test 21: Follower whose:
// conflicting_index < truncated_last_index_
// next_term_index > follower_last_log_index >= truncated_last_index_
// gets a probe starting after their last_log_index.
TEST_F(RaftTest, FollowerConflictingIndexLastLogIndexCase2) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 3);
            EXPECT_EQ(ae.prev_log_index(), 2);
            EXPECT_EQ(ae.entries().size(), 0);
            return 0;
          }));

  raft_->SetStateForTest({
      .current_term = 3,
      .commit_index = 3,
      .last_committed = 3,
      .role = Role::LEADER,
      .snapshot_last_index = 3,
      .snapshot_last_term = 2,
      .truncated_last_index = 2,
      .truncated_last_term = 2,
      .log = CreateLogEntries(
          {//{2, "Transaction 1"},  Truncated
           //{2, "Transaction 2"},  Truncated
           {2, "Transaction 3"},
           {2, "Transaction 4"},
           {3, "RAFT_NO_OP"}},
          true),
      CreateProgressPatch(
          {.next_index = std::vector<uint64_t>{1, 6, 6, 6, 6},
           .match_index = std::vector<uint64_t>{0, 5, 5, 0, 0},
           .states = std::vector<ProgressState>{ProgressState::PROBE,
                                                ProgressState::REPLICATE,
                                                ProgressState::REPLICATE,
                                                ProgressState::REPLICATE,
                                                ProgressState::PROBE}}),
  });

  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);

  AppendEntriesResponse aer;
  aer.set_success(false);
  aer.set_term(3);
  aer.set_id(3);
  aer.set_last_log_index(2);
  aer.set_conflicting_index(1);
  aer.set_conflicting_term(2);

  raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(aer));

  const auto& follower_progress = raft_->GetFollowerProgress();
  EXPECT_EQ(follower_progress[3].state, ProgressState::PROBE);
}

// Test 22: Follower whose
// conflicting_index < truncated_last_index_
// follower_last_log_index >= next_term_index
// follower_last_log_index >= truncated_last_index_
// gets a probe starting at the first index of the next term.
TEST_F(RaftTest, FollowerConflictingIndexLastLogIndexCase3) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 3);
            EXPECT_EQ(ae.prev_log_index(), 4);
            EXPECT_EQ(ae.entries().size(), 0);
            return 0;
          }));

  raft_->SetStateForTest({
      .current_term = 3,
      .commit_index = 3,
      .last_committed = 3,
      .role = Role::LEADER,
      .snapshot_last_index = 3,
      .snapshot_last_term = 2,
      .truncated_last_index = 2,
      .truncated_last_term = 2,
      .log = CreateLogEntries(
          {//{2, "Transaction 1"},  Truncated
           //{2, "Transaction 2"},  Truncated
           {2, "Transaction 3"},
           {2, "Transaction 4"},
           {3, "RAFT_NO_OP"}},
          true),
      CreateProgressPatch(
          {.next_index = std::vector<uint64_t>{1, 6, 6, 6, 6},
           .match_index = std::vector<uint64_t>{0, 5, 5, 0, 0},
           .states = std::vector<ProgressState>{ProgressState::PROBE,
                                                ProgressState::REPLICATE,
                                                ProgressState::REPLICATE,
                                                ProgressState::REPLICATE,
                                                ProgressState::PROBE}}),
  });

  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);

  AppendEntriesResponse aer;
  aer.set_success(false);
  aer.set_term(3);
  aer.set_id(3);
  aer.set_last_log_index(5);
  aer.set_conflicting_index(1);
  aer.set_conflicting_term(2);

  raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(aer));

  const auto& follower_progress = raft_->GetFollowerProgress();
  EXPECT_EQ(follower_progress[3].state, ProgressState::PROBE);
}

// Test 23: Follower whose:
// conflicting_term == 0
// last_log_index < truncated_last_index_
// gets a snapshot.
TEST_F(RaftTest, FollowerConflictingTermZeroNeedsSnapshot) {
  EXPECT_CALL(mock_call, Call(_, _, _)).Times(0);

  raft_->SetStateForTest({
      .current_term = 3,
      .commit_index = 3,
      .last_committed = 3,
      .role = Role::LEADER,
      .snapshot_last_index = 3,
      .snapshot_last_term = 2,
      .truncated_last_index = 2,
      .truncated_last_term = 2,
      .log = CreateLogEntries(
          {//{2, "Transaction 1"},  Truncated
           //{2, "Transaction 2"},  Truncated
           {2, "Transaction 3"},
           {2, "Transaction 4"},
           {3, "RAFT_NO_OP"}},
          true),
      CreateProgressPatch(
          {.next_index = std::vector<uint64_t>{1, 6, 6, 6, 6},
           .match_index = std::vector<uint64_t>{0, 5, 5, 0, 0},
           .states = std::vector<ProgressState>{ProgressState::PROBE,
                                                ProgressState::REPLICATE,
                                                ProgressState::REPLICATE,
                                                ProgressState::REPLICATE,
                                                ProgressState::PROBE}}),
  });

  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);

  AppendEntriesResponse aer;
  aer.set_success(false);
  aer.set_term(3);
  aer.set_id(3);
  aer.set_last_log_index(1);
  aer.set_conflicting_index(0);
  aer.set_conflicting_term(0);

  raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(aer));

  const auto& follower_progress = raft_->GetFollowerProgress();
  EXPECT_EQ(follower_progress[3].state, ProgressState::SNAPSHOT);
}

// Test 24: Follower whose:
// conflicting_term == 0
// last_log_index >= truncated_last_index_
// gets a probe starting right after their last_log_index, no snapshot.
TEST_F(RaftTest, FollowerConflictingTermZeroNoSnapshot) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 3);
            EXPECT_EQ(ae.prev_log_index(), 4);
            EXPECT_EQ(ae.entries().size(), 0);
            return 0;
          }));

  raft_->SetStateForTest({
      .current_term = 3,
      .commit_index = 3,
      .last_committed = 3,
      .role = Role::LEADER,
      .snapshot_last_index = 3,
      .snapshot_last_term = 2,
      .truncated_last_index = 2,
      .truncated_last_term = 2,
      .log = CreateLogEntries(
          {//{2, "Transaction 1"},  Truncated
           //{2, "Transaction 2"},  Truncated
           {2, "Transaction 3"},
           {2, "Transaction 4"},
           {3, "RAFT_NO_OP"}},
          true),
      CreateProgressPatch(
          {.next_index = std::vector<uint64_t>{1, 6, 6, 6, 6},
           .match_index = std::vector<uint64_t>{0, 5, 5, 0, 0},
           .states = std::vector<ProgressState>{ProgressState::PROBE,
                                                ProgressState::REPLICATE,
                                                ProgressState::REPLICATE,
                                                ProgressState::REPLICATE,
                                                ProgressState::PROBE}}),
  });

  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);

  AppendEntriesResponse aer;
  aer.set_success(false);
  aer.set_term(3);
  aer.set_id(3);
  aer.set_last_log_index(4);
  aer.set_conflicting_index(0);
  aer.set_conflicting_term(0);

  raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(aer));

  const auto& follower_progress = raft_->GetFollowerProgress();
  EXPECT_EQ(follower_progress[3].state, ProgressState::PROBE);
}

// Test 25: Follower whose:
// conflicting_index > truncated_last_index_
// gets a probe starting at the first index of the next term.
TEST_F(RaftTest, FollowerConflictingIndexAboveTruncatedIndex) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 3);
            EXPECT_EQ(ae.prev_log_index(), 4);
            EXPECT_EQ(ae.entries().size(), 0);
            return 0;
          }));

  raft_->SetStateForTest({
      .current_term = 3,
      .commit_index = 3,
      .last_committed = 3,
      .role = Role::LEADER,
      .snapshot_last_index = 3,
      .snapshot_last_term = 2,
      .truncated_last_index = 2,
      .truncated_last_term = 2,
      .log = CreateLogEntries(
          {//{2, "Transaction 1"},  Truncated
           //{2, "Transaction 2"},  Truncated
           {2, "Transaction 3"},
           {2, "Transaction 4"},
           {3, "RAFT_NO_OP"}},
          true),
      CreateProgressPatch(
          {.next_index = std::vector<uint64_t>{1, 6, 6, 6, 6},
           .match_index = std::vector<uint64_t>{0, 5, 5, 0, 0},
           .states = std::vector<ProgressState>{ProgressState::PROBE,
                                                ProgressState::REPLICATE,
                                                ProgressState::REPLICATE,
                                                ProgressState::REPLICATE,
                                                ProgressState::PROBE}}),
  });

  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);

  AppendEntriesResponse aer;
  aer.set_success(false);
  aer.set_term(3);
  aer.set_id(3);
  aer.set_last_log_index(4);
  aer.set_conflicting_index(4);
  aer.set_conflicting_term(2);

  raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(aer));

  const auto& follower_progress = raft_->GetFollowerProgress();
  EXPECT_EQ(follower_progress[3].state, ProgressState::PROBE);
}

// Test 26: Follower whose:
// conflicting_index <= truncated_last_index_
// conflicting_term < truncated_last_term_
// gets a snapshot.
TEST_F(RaftTest, FollowerConflictingIndexTruncatedTermMismatch) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 3);
            EXPECT_EQ(ae.prev_log_index(), 2);
            EXPECT_EQ(ae.entries().size(), 0);
            return 0;
          }));

  raft_->SetStateForTest({
      .current_term = 3,
      .commit_index = 3,
      .last_committed = 3,
      .role = Role::LEADER,
      .snapshot_last_index = 3,
      .snapshot_last_term = 2,
      .truncated_last_index = 2,
      .truncated_last_term = 2,
      .log = CreateLogEntries(
          {//{2, "Transaction 1"},  Truncated
           //{2, "Transaction 2"},  Truncated
           {2, "Transaction 3"},
           {2, "Transaction 4"},
           {3, "RAFT_NO_OP"}},
          true),
      CreateProgressPatch(
          {.next_index = std::vector<uint64_t>{1, 6, 6, 6, 6},
           .match_index = std::vector<uint64_t>{0, 5, 5, 0, 0},
           .states = std::vector<ProgressState>{ProgressState::PROBE,
                                                ProgressState::REPLICATE,
                                                ProgressState::REPLICATE,
                                                ProgressState::REPLICATE,
                                                ProgressState::PROBE}}),
  });

  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);

  AppendEntriesResponse aer;
  aer.set_success(false);
  aer.set_term(3);
  aer.set_id(3);
  aer.set_last_log_index(2);
  aer.set_conflicting_index(2);
  aer.set_conflicting_term(1);

  raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(aer));

  const auto& follower_progress = raft_->GetFollowerProgress();
  EXPECT_EQ(follower_progress[3].state, ProgressState::SNAPSHOT);
}

// Test 27: Leader receives delayed success response from follower whose next
// index to send has already been truncated. Upon the next heartbeat, it cannot
// use a prev_log_index lower than truncated_last_index, so the follower will
// respond with success == false, which will trigger a snapshot.
TEST_F(RaftTest, LeaderReceivesDelayedAERSuccessAndSendsSnapshot) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 2);
            EXPECT_EQ(ae.prev_log_index(), 4);
            EXPECT_EQ(ae.entries().size(), 0);
            return 0;
          }))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 3);
            EXPECT_EQ(ae.prev_log_index(), 2);
            EXPECT_EQ(ae.entries().size(), 0);
            return 0;
          }))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 4);
            EXPECT_EQ(ae.prev_log_index(), 4);
            EXPECT_EQ(ae.entries().size(), 0);
            return 0;
          }));

  raft_->SetStateForTest({
      .current_term = 2,
      .commit_index = 3,
      .last_committed = 3,
      .role = Role::LEADER,
      .snapshot_last_index = 3,
      .snapshot_last_term = 2,
      .truncated_last_index = 2,
      .truncated_last_term = 2,
      .log = CreateLogEntries(
          {//{2, "Transaction 1"},  Truncated
           //{2, "Transaction 2"},  Truncated
           {2, "Transaction 3"},
           {2, "Transaction 4"}},
          true),
      CreateProgressPatch(
          {.next_index = std::vector<uint64_t>{1, 5, 5, 3, 5},
           .match_index = std::vector<uint64_t>{0, 4, 4, 0, 0},
           .states = std::vector<ProgressState>{ProgressState::PROBE,
                                                ProgressState::REPLICATE,
                                                ProgressState::REPLICATE,
                                                ProgressState::REPLICATE,
                                                ProgressState::PROBE}}),
  });

  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);

  AppendEntriesResponse aer;
  aer.set_success(true);
  aer.set_term(2);
  aer.set_id(3);
  aer.set_last_log_index(1);
  aer.set_conflicting_index(0);
  aer.set_conflicting_term(0);

  raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(aer));

  // Follower 3's next_index will be behind truncated_last_index, so nothing
  // will be sent out until the next heartbeat.

  raft_->SendHeartbeat();
  const auto& follower_progress = raft_->GetFollowerProgress();
  EXPECT_EQ(follower_progress[3].state, ProgressState::SNAPSHOT);
}

}  // namespace raft
}  // namespace resdb

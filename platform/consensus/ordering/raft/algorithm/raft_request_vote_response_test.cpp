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

// Test 1: A candidate gets elected, adds a NO-OP entry, and sends a probe to
// all followers.
TEST_F(RaftTest, CandidateGetsElected) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(1);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(0);
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& AppendEntriesMessage =
                dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 2);
            EXPECT_EQ(AppendEntriesMessage.entries_size(), 0);
            EXPECT_EQ(AppendEntriesMessage.prev_log_term(), 1);
            EXPECT_EQ(AppendEntriesMessage.prev_log_index(), 2);
            EXPECT_EQ(AppendEntriesMessage.leader_id(), 1);
            EXPECT_EQ(AppendEntriesMessage.leader_commit_index(), 1);
            return 0;
          }))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& AppendEntriesMessage =
                dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 3);
            EXPECT_EQ(AppendEntriesMessage.entries_size(), 0);
            EXPECT_EQ(AppendEntriesMessage.prev_log_term(), 1);
            EXPECT_EQ(AppendEntriesMessage.prev_log_index(), 2);
            EXPECT_EQ(AppendEntriesMessage.leader_id(), 1);
            EXPECT_EQ(AppendEntriesMessage.leader_commit_index(), 1);
            return 0;
          }))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& AppendEntriesMessage =
                dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 4);
            EXPECT_EQ(AppendEntriesMessage.entries_size(), 0);
            EXPECT_EQ(AppendEntriesMessage.prev_log_term(), 1);
            EXPECT_EQ(AppendEntriesMessage.prev_log_index(), 2);
            EXPECT_EQ(AppendEntriesMessage.leader_id(), 1);
            EXPECT_EQ(AppendEntriesMessage.leader_commit_index(), 1);
            return 0;
          }));

  raft_->SetStateForTest({.current_term = 2,
                          .commit_index = 1,
                          .last_committed = 1,
                          .role = Role::CANDIDATE,
                          .log = CreateLogEntries(
                              {
                                  {0, "Term 0 Transaction 1"},
                                  {1, "Term 1 Transaction 1"},
                              },
                              true),
                          .votes = std::vector<int>{1, 3}});

  RequestVoteResponse rvr;
  rvr.set_term(2);
  rvr.set_voterid(2);
  rvr.set_votegranted(true);
  raft_->ReceiveRequestVoteResponse(std::make_unique<RequestVoteResponse>(rvr));

  EXPECT_EQ(raft_->GetCurrentTerm(), 2);
  EXPECT_EQ(raft_->GetRoleSnapshot(), Role::LEADER);
  // NO-OP entry is added.
  EXPECT_EQ(raft_->GetLastLogIndexFromLog(), 3);
  EXPECT_THAT(raft_->GetNextIndex(), ::testing::ElementsAre(_, 4, 3, 3, 3));
  EXPECT_THAT(raft_->GetMatchIndex(), ::testing::ElementsAre(_, 3, 0, 0, 0));
  const auto& follower_progress = raft_->GetFollowerProgress();
  for (const auto& follower : follower_progress) {
    EXPECT_EQ(follower.state, ProgressState::PROBE);
  }
}

// Test 2: A candidate receives a RequestVoteResponse from an older term and
// ignores it.
TEST_F(RaftTest, CandidateIgnoresResponseFromOldTerm) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(0);
  EXPECT_CALL(mock_call, Call(_, _, _)).Times(0);

  raft_->SetStateForTest({
      .current_term = 2,
      .role = Role::CANDIDATE,
      .log = CreateLogEntries(
          {
              {0, "Term 0 Transaction 1"},
          },
          true),
  });

  RequestVoteResponse rvr;
  rvr.set_term(1);
  rvr.set_voterid(2);
  rvr.set_votegranted(true);
  raft_->ReceiveRequestVoteResponse(std::make_unique<RequestVoteResponse>(rvr));

  EXPECT_EQ(raft_->GetRoleSnapshot(), Role::CANDIDATE);
}

// Test 3: A candidate receives a RequestVoteResponse from an newer term and
// demotes.
TEST_F(RaftTest, CandidateDemotesAfterRequestVoteResponseFromNewerTerm) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(1);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(0);
  EXPECT_CALL(mock_call, Call(_, _, _)).Times(0);

  raft_->SetStateForTest({
      .current_term = 2,
      .role = Role::CANDIDATE,
      .log = CreateLogEntries(
          {
              {0, "Term 0 Transaction 1"},
          },
          true),
  });

  RequestVoteResponse rvr;
  rvr.set_term(3);
  rvr.set_voterid(2);
  rvr.set_votegranted(false);
  raft_->ReceiveRequestVoteResponse(std::make_unique<RequestVoteResponse>(rvr));

  EXPECT_EQ(raft_->GetVotedFor(), -1);
  EXPECT_EQ(raft_->GetCurrentTerm(), 3);
  EXPECT_EQ(raft_->GetRoleSnapshot(), Role::FOLLOWER);
}

// Test 4: A follower ignores a RequestVoteResponse.
TEST_F(RaftTest, FollowerIgnoresRequestVoteResponse) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(0);
  EXPECT_CALL(mock_call, Call(_, _, _)).Times(0);

  raft_->SetStateForTest({
      .current_term = 2,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries(
          {
              {0, "Term 0 Transaction 1"},
          },
          true),
  });

  RequestVoteResponse rvr;
  rvr.set_term(2);
  rvr.set_voterid(2);
  rvr.set_votegranted(true);
  raft_->ReceiveRequestVoteResponse(std::make_unique<RequestVoteResponse>(rvr));

  EXPECT_EQ(raft_->GetRoleSnapshot(), Role::FOLLOWER);
}

// Test 5: A candidate ignores a no vote in a RequestVoteResponse.
TEST_F(RaftTest, CandidateIgnoresNoVote) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(mock_call, Call(_, _, _)).Times(0);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(0);

  raft_->SetStateForTest({
      .current_term = 2,
      .role = Role::CANDIDATE,
      .log = CreateLogEntries(
          {
              {0, "Term 0 Transaction 1"},
          },
          true),
  });

  RequestVoteResponse rvr;
  rvr.set_term(2);
  rvr.set_voterid(2);
  rvr.set_votegranted(false);
  raft_->ReceiveRequestVoteResponse(std::make_unique<RequestVoteResponse>(rvr));

  EXPECT_EQ(raft_->GetRoleSnapshot(), Role::CANDIDATE);
}

// Test 6: A candidate ignores a duplicate vote.
TEST_F(RaftTest, CandidateIgnoresDuplicateVote) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(0);
  EXPECT_CALL(mock_call, Call(_, _, _)).Times(0);

  raft_->SetStateForTest({.current_term = 2,
                          .commit_index = 1,
                          .last_committed = 1,
                          .role = Role::CANDIDATE,
                          .log = CreateLogEntries(
                              {
                                  {0, "Term 0 Transaction 1"},
                                  {1, "Term 1 Transaction 1"},
                              },
                              true),
                          .votes = std::vector<int>{1, 2}});

  RequestVoteResponse rvr;
  rvr.set_term(2);
  rvr.set_voterid(2);
  rvr.set_votegranted(true);
  const auto& votes = raft_->GetVotes();
  raft_->ReceiveRequestVoteResponse(std::make_unique<RequestVoteResponse>(rvr));

  EXPECT_EQ(raft_->GetRoleSnapshot(), Role::CANDIDATE);
  EXPECT_EQ(raft_->GetVotes(), votes);
}

// Test 7: A leader ignores a RequestVoteResponse.
TEST_F(RaftTest, LeaderIgnoresRequestVoteResponse) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(0);
  EXPECT_CALL(mock_call, Call(_, _, _)).Times(0);

  raft_->SetStateForTest({.current_term = 2,
                          .voted_for = 1,
                          .commit_index = 1,
                          .last_committed = 1,
                          .role = Role::LEADER,
                          .log = CreateLogEntries(
                              {
                                  {0, "Term 0 Transaction 1"},
                                  {1, "Term 1 Transaction 1"},
                              },
                              true),
                          .votes = std::vector<int>{1, 2}});

  RequestVoteResponse rvr;
  rvr.set_term(2);
  rvr.set_voterid(2);
  rvr.set_votegranted(true);
  raft_->ReceiveRequestVoteResponse(std::make_unique<RequestVoteResponse>(rvr));

  EXPECT_EQ(raft_->GetRoleSnapshot(), Role::LEADER);
}

}  // namespace raft
}  // namespace resdb

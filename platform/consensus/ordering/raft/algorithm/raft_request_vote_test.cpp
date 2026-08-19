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

// Test 1: A follower times out, transitions to candidate, and starts an
// election.
TEST_F(RaftTest, FollowerTransitionsToCandidateAndStartsElection) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(1);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(0);
  EXPECT_CALL(mock_broadcast, Broadcast(_, _))
      .WillOnce(
          ::testing::Invoke([](int type, const google::protobuf::Message& msg) {
            const auto& request_vote = dynamic_cast<const RequestVote&>(msg);
            EXPECT_EQ(request_vote.term(), 1);
            EXPECT_EQ(request_vote.candidateid(), 1);
            EXPECT_EQ(request_vote.last_log_index(), 1);
            EXPECT_EQ(request_vote.lastlogterm(), 0);
            return 0;
          }));

  raft_->SetStateForTest({
      .current_term = 0,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries(
          {
              {0, "Term 0 Transaction 1"},
          },
          true),
  });

  raft_->StartElection();
  EXPECT_EQ(raft_->GetVotedFor(), 1);
  EXPECT_EQ(raft_->GetCurrentTerm(), 1);
  EXPECT_EQ(raft_->GetRoleSnapshot(), Role::CANDIDATE);
}

// Test 2: A leader receives a RequestVote from a candidate in a newer term and
// demotes.
TEST_F(RaftTest, LeaderReceivesRequestVoteFromNewTermAndDemotes) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(1);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(1);
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& request_vote_response =
                dynamic_cast<const RequestVoteResponse&>(msg);
            EXPECT_EQ(node_id, 2);
            EXPECT_EQ(request_vote_response.term(), 1);
            EXPECT_EQ(request_vote_response.voterid(), 1);
            EXPECT_TRUE(request_vote_response.votegranted());
            return 0;
          }));

  RequestVote rv;
  rv.set_term(1);
  rv.set_candidateid(2);
  rv.set_last_log_index(1);
  rv.set_lastlogterm(0);

  raft_->SetStateForTest({
      .current_term = 0,
      .role = Role::LEADER,
      .log = CreateLogEntries(
          {
              {0, "Term 0 Transaction 1"},
          },
          true),
  });

  raft_->ReceiveRequestVote(std::make_unique<RequestVote>(rv));

  EXPECT_EQ(raft_->GetVotedFor(), 2);
  EXPECT_EQ(raft_->GetCurrentTerm(), 1);
  EXPECT_EQ(raft_->GetRoleSnapshot(), Role::FOLLOWER);
}

// Test 3: A leader receives a RequestVote from a candidate whose last_log_term
// is fewer and does not vote.
TEST_F(RaftTest, LeaderReceivesRequestVoteFromOldTerm) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(0);
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& request_vote_response =
                dynamic_cast<const RequestVoteResponse&>(msg);
            EXPECT_EQ(node_id, 2);
            EXPECT_EQ(request_vote_response.term(), 1);
            EXPECT_EQ(request_vote_response.voterid(), 1);
            EXPECT_FALSE(request_vote_response.votegranted());
            return 0;
          }));

  RequestVote rv;
  rv.set_term(1);
  rv.set_candidateid(2);
  rv.set_last_log_index(0);
  rv.set_lastlogterm(0);

  raft_->SetStateForTest({
      .current_term = 1,
      .role = Role::LEADER,
      .log = CreateLogEntries(
          {
              {0, "Term 0 Transaction 1"},
          },
          true),
  });

  raft_->ReceiveRequestVote(std::make_unique<RequestVote>(rv));

  EXPECT_EQ(raft_->GetVotedFor(), -1);
  EXPECT_EQ(raft_->GetCurrentTerm(), 1);
  EXPECT_EQ(raft_->GetRoleSnapshot(), Role::LEADER);
}

// Test 4: A leader receives a RequestVote from a candidate whose last_log_term
// is the same, but whose last_log_index is further behind.
TEST_F(RaftTest, LeaderReceivesRequestVoteFromFurtherBehindLog) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(1);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(0);
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& request_vote_response =
                dynamic_cast<const RequestVoteResponse&>(msg);
            EXPECT_EQ(node_id, 2);
            EXPECT_EQ(request_vote_response.term(), 2);
            EXPECT_EQ(request_vote_response.voterid(), 1);
            EXPECT_FALSE(request_vote_response.votegranted());
            return 0;
          }));

  RequestVote rv;
  rv.set_term(2);
  rv.set_candidateid(2);
  rv.set_last_log_index(0);
  rv.set_lastlogterm(0);

  raft_->SetStateForTest({
      .current_term = 1,
      .role = Role::LEADER,
      .log = CreateLogEntries(
          {
              {0, "Term 0 Transaction 1"},
          },
          true),
  });

  raft_->ReceiveRequestVote(std::make_unique<RequestVote>(rv));

  EXPECT_EQ(raft_->GetVotedFor(), -1);
  EXPECT_EQ(raft_->GetCurrentTerm(), 2);
  EXPECT_EQ(raft_->GetRoleSnapshot(), Role::FOLLOWER);
}

// Test 5: A follower receives a RequestVote from a candidate who it would vote
// for, if it had not already voted for someone else.
TEST_F(RaftTest, FollowerRejectsRequestVoteBecauseAlreadyVoted) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(0);
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& request_vote_response =
                dynamic_cast<const RequestVoteResponse&>(msg);
            EXPECT_EQ(node_id, 2);
            EXPECT_EQ(request_vote_response.term(), 2);
            EXPECT_EQ(request_vote_response.voterid(), 1);
            EXPECT_FALSE(request_vote_response.votegranted());
            return 0;
          }));

  RequestVote rv;
  rv.set_term(2);
  rv.set_candidateid(2);
  rv.set_last_log_index(2);
  rv.set_lastlogterm(1);

  raft_->SetStateForTest({
      .current_term = 2,
      .voted_for = 3,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries(
          {
              {0, "Term 0 Transaction 1"},
          },
          true),
  });

  raft_->ReceiveRequestVote(std::make_unique<RequestVote>(rv));

  EXPECT_EQ(raft_->GetVotedFor(), 3);
  EXPECT_EQ(raft_->GetCurrentTerm(), 2);
  EXPECT_EQ(raft_->GetRoleSnapshot(), Role::FOLLOWER);
}

// Test 6: A follower times out and starts an election. Then, as a candidate
// times out and starts another election.
TEST_F(RaftTest, CandidateTimesOutAndStartsAnotherElection) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(1);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(0);
  EXPECT_CALL(mock_broadcast, Broadcast(_, _))
      .WillOnce(
          ::testing::Invoke([](int type, const google::protobuf::Message& msg) {
            const auto& request_vote = dynamic_cast<const RequestVote&>(msg);
            EXPECT_EQ(request_vote.term(), 1);
            EXPECT_EQ(request_vote.candidateid(), 1);
            EXPECT_EQ(request_vote.last_log_index(), 1);
            EXPECT_EQ(request_vote.lastlogterm(), 0);
            return 0;
          }))
      .WillOnce(
          ::testing::Invoke([](int type, const google::protobuf::Message& msg) {
            const auto& request_vote = dynamic_cast<const RequestVote&>(msg);
            EXPECT_EQ(request_vote.term(), 2);
            EXPECT_EQ(request_vote.candidateid(), 1);
            EXPECT_EQ(request_vote.last_log_index(), 1);
            EXPECT_EQ(request_vote.lastlogterm(), 0);
            return 0;
          }));

  raft_->SetStateForTest({
      .current_term = 0,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries(
          {
              {0, "Term 0 Transaction 1"},
          },
          true),
  });

  raft_->StartElection();
  EXPECT_EQ(raft_->GetVotedFor(), 1);
  EXPECT_EQ(raft_->GetCurrentTerm(), 1);
  EXPECT_EQ(raft_->GetRoleSnapshot(), Role::CANDIDATE);

  // Start another election after a timeout
  raft_->StartElection();
  EXPECT_EQ(raft_->GetVotedFor(), 1);
  EXPECT_EQ(raft_->GetCurrentTerm(), 2);
  EXPECT_EQ(raft_->GetRoleSnapshot(), Role::CANDIDATE);
}

// Test 7: A candidate receives a RequestVote from another candidate in the same
// term and does not demote.
TEST_F(RaftTest, CandidateReceivesRequestVoteFromSameTermAndDoesNotDemote) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(0);
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& request_vote_response =
                dynamic_cast<const RequestVoteResponse&>(msg);
            EXPECT_EQ(node_id, 2);
            EXPECT_EQ(request_vote_response.term(), 1);
            EXPECT_EQ(request_vote_response.voterid(), 1);
            EXPECT_FALSE(request_vote_response.votegranted());
            return 0;
          }));

  RequestVote rv;
  rv.set_term(1);
  rv.set_candidateid(2);
  rv.set_last_log_index(1);
  rv.set_lastlogterm(0);

  raft_->SetStateForTest({
      .current_term = 1,
      .voted_for = 1,
      .role = Role::CANDIDATE,
      .log = CreateLogEntries(
          {
              {0, "Term 0 Transaction 1"},
          },
          true),
  });

  raft_->ReceiveRequestVote(std::make_unique<RequestVote>(rv));

  EXPECT_EQ(raft_->GetVotedFor(), 1);
  EXPECT_EQ(raft_->GetCurrentTerm(), 1);
  EXPECT_EQ(raft_->GetRoleSnapshot(), Role::CANDIDATE);
}

// Test 8: A candidate receives a RequestVote from another candidate in the same
// term, with a further along log and does not demote.
TEST_F(RaftTest,
       CandidateReceivesRequestVoteFromSameTermWithLongerLogAndDoesNotDemote) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(0);
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& request_vote_response =
                dynamic_cast<const RequestVoteResponse&>(msg);
            EXPECT_EQ(node_id, 2);
            EXPECT_EQ(request_vote_response.term(), 1);
            EXPECT_EQ(request_vote_response.voterid(), 1);
            EXPECT_FALSE(request_vote_response.votegranted());
            return 0;
          }));

  RequestVote rv;
  rv.set_term(1);
  rv.set_candidateid(2);
  rv.set_last_log_index(4);
  rv.set_lastlogterm(0);

  raft_->SetStateForTest({
      .current_term = 1,
      .voted_for = 1,
      .role = Role::CANDIDATE,
      .log = CreateLogEntries(
          {
              {0, "Term 0 Transaction 1"},
          },
          true),
  });

  raft_->ReceiveRequestVote(std::make_unique<RequestVote>(rv));

  EXPECT_EQ(raft_->GetVotedFor(), 1);
  EXPECT_EQ(raft_->GetCurrentTerm(), 1);
  EXPECT_EQ(raft_->GetRoleSnapshot(), Role::CANDIDATE);
}

// Test 9: A follower receives a RequestVote from a candidate in the same
// term, with a further along log and votes for them.
TEST_F(RaftTest, FollowerReceivesRequestVoteFromSameTermWithLongerLog) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(1);
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& request_vote_response =
                dynamic_cast<const RequestVoteResponse&>(msg);
            EXPECT_EQ(node_id, 2);
            EXPECT_EQ(request_vote_response.term(), 2);
            EXPECT_EQ(request_vote_response.voterid(), 1);
            EXPECT_TRUE(request_vote_response.votegranted());
            return 0;
          }));

  RequestVote rv;
  rv.set_term(2);
  rv.set_candidateid(2);
  rv.set_last_log_index(4);
  rv.set_lastlogterm(1);

  raft_->SetStateForTest({
      .current_term = 2,
      .voted_for = -1,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries(
          {
              {0, "Term 0 Transaction 1"},
          },
          true),
  });

  raft_->ReceiveRequestVote(std::make_unique<RequestVote>(rv));

  EXPECT_EQ(raft_->GetVotedFor(), 2);
  EXPECT_EQ(raft_->GetCurrentTerm(), 2);
  EXPECT_EQ(raft_->GetRoleSnapshot(), Role::FOLLOWER);
}

// Test 10: A candidate ignores a RequestVote from themselves
TEST_F(RaftTest, CandidateIgnoresRequestVoteFromSelf) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(0);
  EXPECT_CALL(mock_call, Call(_, _, _)).Times(0);

  RequestVote rv;
  rv.set_term(1);
  rv.set_candidateid(1);
  rv.set_last_log_index(1);
  rv.set_lastlogterm(0);

  raft_->SetStateForTest({.current_term = 1,
                          .voted_for = 1,
                          .role = Role::CANDIDATE,
                          .log = CreateLogEntries(
                              {
                                  {0, "Term 0 Transaction 1"},
                              },
                              true),
                          .votes = std::vector<int>{1}});
  const auto& votes = raft_->GetVotes();

  raft_->ReceiveRequestVote(std::make_unique<RequestVote>(rv));

  EXPECT_EQ(raft_->GetVotedFor(), 1);
  EXPECT_EQ(raft_->GetCurrentTerm(), 1);
  EXPECT_EQ(raft_->GetRoleSnapshot(), Role::CANDIDATE);
  EXPECT_EQ(votes, raft_->GetVotes());
}

// Test 11: A follower that has already voted for a candidate will re-send their
// vote to them if prompted.
TEST_F(RaftTest, CandidateResendsVote) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(1);
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& request_vote_response =
                dynamic_cast<const RequestVoteResponse&>(msg);
            EXPECT_EQ(node_id, 2);
            EXPECT_EQ(request_vote_response.term(), 1);
            EXPECT_EQ(request_vote_response.voterid(), 1);
            EXPECT_TRUE(request_vote_response.votegranted());
            return 0;
          }));

  RequestVote rv;
  rv.set_term(1);
  rv.set_candidateid(2);
  rv.set_last_log_index(1);
  rv.set_lastlogterm(0);

  raft_->SetStateForTest({.current_term = 1,
                          .voted_for = 2,
                          .role = Role::FOLLOWER,
                          .log = CreateLogEntries(
                              {
                                  {0, "Term 0 Transaction 1"},
                              },
                              true),
                          .votes = std::vector<int>{1}});
  const auto& votes = raft_->GetVotes();

  raft_->ReceiveRequestVote(std::make_unique<RequestVote>(rv));

  EXPECT_EQ(raft_->GetVotedFor(), 2);
  EXPECT_EQ(raft_->GetCurrentTerm(), 1);
  EXPECT_EQ(raft_->GetRoleSnapshot(), Role::FOLLOWER);
  EXPECT_EQ(votes, raft_->GetVotes());
}

}  // namespace raft
}  // namespace resdb

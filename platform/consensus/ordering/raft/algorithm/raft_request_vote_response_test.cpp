#include "platform/consensus/ordering/raft/algorithm/raft_tests.h"

namespace resdb {
namespace raft {
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Invoke;
using ::testing::Matcher;

// Test 1: A candidate gets elected.
TEST_F(RaftTest, CandidateGetsElected) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(1);
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& AppendEntriesMessage =
                dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 2);
            EXPECT_EQ(AppendEntriesMessage.entries_size(), 0);
            EXPECT_EQ(AppendEntriesMessage.prevlogterm(), 1);
            EXPECT_EQ(AppendEntriesMessage.prevlogindex(), 2);
            EXPECT_EQ(AppendEntriesMessage.leaderid(), 1);
            EXPECT_EQ(AppendEntriesMessage.leadercommitindex(), 1);
            return 0;
          }))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& AppendEntriesMessage =
                dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 3);
            EXPECT_EQ(AppendEntriesMessage.entries_size(), 0);
            EXPECT_EQ(AppendEntriesMessage.prevlogterm(), 1);
            EXPECT_EQ(AppendEntriesMessage.prevlogindex(), 2);
            EXPECT_EQ(AppendEntriesMessage.leaderid(), 1);
            EXPECT_EQ(AppendEntriesMessage.leadercommitindex(), 1);
            return 0;
          }))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& AppendEntriesMessage =
                dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 4);
            EXPECT_EQ(AppendEntriesMessage.entries_size(), 0);
            EXPECT_EQ(AppendEntriesMessage.prevlogterm(), 1);
            EXPECT_EQ(AppendEntriesMessage.prevlogindex(), 2);
            EXPECT_EQ(AppendEntriesMessage.leaderid(), 1);
            EXPECT_EQ(AppendEntriesMessage.leadercommitindex(), 1);
            return 0;
          }));

  raft_->SetStateForTest({.currentTerm = 2,
                          .commitIndex = 1,
                          .lastApplied = 1,
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
  EXPECT_EQ(raft_->GetLastLogIndexFromLog(), 2);
  EXPECT_THAT(raft_->GetNextIndex(), ::testing::ElementsAre(3, 3, 3, 3, 3));
  EXPECT_THAT(raft_->GetMatchIndex(), ::testing::ElementsAre(0, 2, 0, 0, 0));
}

// Test 2: A candidate receives a RequestVoteResponse from an older term and
// ignores it.
TEST_F(RaftTest, CandidateIgnoresResponseFromOldTerm) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(mock_call, Call(_, _, _)).Times(0);
  EXPECT_CALL(*leader_election_manager_, OnHeartBeat()).Times(0);

  raft_->SetStateForTest({
      .currentTerm = 2,
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
  EXPECT_CALL(mock_call, Call(_, _, _)).Times(0);
  EXPECT_CALL(*leader_election_manager_, OnHeartBeat()).Times(0);

  raft_->SetStateForTest({
      .currentTerm = 2,
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
  EXPECT_CALL(mock_call, Call(_, _, _)).Times(0);
  EXPECT_CALL(*leader_election_manager_, OnHeartBeat()).Times(0);

  raft_->SetStateForTest({
      .currentTerm = 2,
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
  EXPECT_CALL(*leader_election_manager_, OnHeartBeat()).Times(0);

  raft_->SetStateForTest({
      .currentTerm = 2,
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
  EXPECT_CALL(mock_call, Call(_, _, _)).Times(0);
  EXPECT_CALL(*leader_election_manager_, OnHeartBeat()).Times(0);

  raft_->SetStateForTest({.currentTerm = 2,
                          .commitIndex = 1,
                          .lastApplied = 1,
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
  raft_->ReceiveRequestVoteResponse(std::make_unique<RequestVoteResponse>(rvr));

  EXPECT_EQ(raft_->GetRoleSnapshot(), Role::CANDIDATE);
}

}  // namespace raft
}  // namespace resdb

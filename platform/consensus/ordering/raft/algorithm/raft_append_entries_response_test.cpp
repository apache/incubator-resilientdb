#include "platform/consensus/ordering/raft/algorithm/raft_tests.h"

namespace resdb {
namespace raft {
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Invoke;
using ::testing::Matcher;

// Test 1: A leader receiving an AppendEntriesResponse success and updating the
// follower's matchIndex.
TEST_F(RaftTest, LeaderReceivesAppendEntriesResponseSuccess) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);

  AppendEntriesResponse aeResponse;
  aeResponse.set_success(true);
  aeResponse.set_term(1);
  aeResponse.set_id(2);
  aeResponse.set_lastlogindex(2);

  raft_->SetStateForTest({.currentTerm = 1,
                          .commitIndex = 0,
                          .role = Role::LEADER,
                          .log = CreateLogEntries(
                              {
                                  {0, "Transaction 1"},
                                  {0, "Transaction 2"},
                              },
                              true),
                          .matchIndex = std::vector<uint64_t>{0, 2, 0, 0, 0}});

  bool success = raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(aeResponse));
  EXPECT_TRUE(success);
  EXPECT_THAT(raft_->GetMatchIndex(), ::testing::ElementsAre(0, 2, 2, 0, 0));
}

// Test 2: A leader receiving an AppendEntriesResponse from a follower that in a
// newer term.
TEST_F(RaftTest, LeaderReceivesAppendEntriesResponseFromNewerTerm) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(1);

  raft_->SetStateForTest({
      .currentTerm = 1,
      .role = Role::LEADER,
  });

  AppendEntriesResponse aeResponse;
  aeResponse.set_success(false);
  aeResponse.set_term(2);

  bool success = raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(aeResponse));
  EXPECT_FALSE(success);
  EXPECT_EQ(raft_->GetRoleSnapshot(), Role::FOLLOWER);
}

// Test 3: A leader receiving an AppendEntriesResponse success, updating the
// follower's matchIndex, and committing a new entry.
TEST_F(RaftTest, LeaderReceivesAppendEntriesResponseSuccessAndCommits) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(mock_commit, Commit(_)).Times(1);

  AppendEntriesResponse aeResponse;
  aeResponse.set_success(true);
  aeResponse.set_term(1);
  aeResponse.set_id(2);
  aeResponse.set_lastlogindex(2);

  raft_->SetStateForTest({.currentTerm = 1,
                          .commitIndex = 0,
                          .lastApplied = 0,
                          .role = Role::LEADER,
                          .log = CreateLogEntries(
                              {
                                  {1, "Transaction 1"},
                                  {1, "Transaction 2"},
                              },
                              true),
                          .nextIndex = std::vector<uint64_t>{0, 2, 2, 2, 2},
                          .matchIndex = std::vector<uint64_t>{0, 2, 0, 1, 0}});

  bool success = raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(aeResponse));
  EXPECT_TRUE(success);
  EXPECT_THAT(raft_->GetMatchIndex(), ::testing::ElementsAre(0, 2, 2, 1, 0));
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

  AppendEntriesResponse aeResponse;
  aeResponse.set_success(true);
  aeResponse.set_term(1);
  aeResponse.set_id(2);
  aeResponse.set_lastlogindex(1);

  raft_->SetStateForTest({
      .currentTerm = 1,
      .commitIndex = 0,
      .lastApplied = 0,
      .role = Role::LEADER,
      .log = CreateLogEntries(
          {
              {1, "Transaction 1"},
              {1, "Transaction 2"},
          },
          true),
  });

  bool success = raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(aeResponse));
  EXPECT_TRUE(success);
}

// Test 5: A leader receiving an AppendEntriesResponse Failure and catching up a
// follower that is behind.
TEST_F(RaftTest, LeaderCatchesUpFollowerThatIsBehindFailure) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            // TODO: Use serialized string instead of manually doing it.
            EXPECT_EQ(ae.entries(0).command(), "\n\rTransaction 1");
            EXPECT_EQ(ae.entries(1).command(), "\n\rTransaction 2");
            EXPECT_EQ(ae.entries_size(), 2);
            EXPECT_EQ(node_id, 2);
            return 0;
          }));

  AppendEntriesResponse aeResponse;
  aeResponse.set_success(false);
  aeResponse.set_term(1);
  aeResponse.set_id(2);
  aeResponse.set_lastlogindex(0);

  raft_->SetStateForTest({
      .currentTerm = 1,
      .commitIndex = 0,
      .lastApplied = 0,
      .role = Role::LEADER,
      .log = CreateLogEntries(
          {
              {1, "Transaction 1"},
              {1, "Transaction 2"},
          },
          true),
  });

  bool success = raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(aeResponse));
  EXPECT_TRUE(success);
}

// Test 6: A follower ignores an AppendEntriesResponse.
TEST_F(RaftTest, FollowerIgnoresAppendEntriesResponse) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(mock_call, Call(_, _, _)).Times(0);

  AppendEntriesResponse aeResponse;
  aeResponse.set_term(1);

  raft_->SetStateForTest({
      .currentTerm = 1,
      .role = Role::FOLLOWER,
  });

  bool success = raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(aeResponse));
  EXPECT_TRUE(success);
}

// Test 7: A leader ignores an AppendEntriesResponse from an outdated term.
TEST_F(RaftTest, LeaderIgnoresAppendEntriesResponseFromOutdatedTerm) {
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(mock_call, Call(_, _, _)).Times(0);

  AppendEntriesResponse aeResponse;
  aeResponse.set_term(1);

  raft_->SetStateForTest({
      .currentTerm = 2,
      .role = Role::LEADER,
  });

  bool success = raft_->ReceiveAppendEntriesResponse(
      std::make_unique<AppendEntriesResponse>(aeResponse));
  EXPECT_TRUE(success);
}

}  // namespace raft
}  // namespace resdb

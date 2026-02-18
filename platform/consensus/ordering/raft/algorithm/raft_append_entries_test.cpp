#include "platform/consensus/ordering/raft/algorithm/raft_tests.h"

namespace resdb {
namespace raft {
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Invoke;
using ::testing::Matcher;

// Test 1: A follower receiving a client transaction should reject it.
TEST_F(RaftTest, FollowerRejectsClientTransaction) {
  EXPECT_CALL(mock_call, Call(_, _, _)).Times(0);
  EXPECT_CALL(mock_broadcast, Broadcast(_, _)).Times(0);

  auto req = std::make_unique<Request>();
  req->set_seq(1);
  raft_->SetRole(Role::FOLLOWER);

  bool success = raft_->ReceiveTransaction(std::move(req));
  EXPECT_FALSE(success);
}

// Test 2: A leader receiving a client transaction should send an AppendEntries
// to all other replicas.
TEST_F(RaftTest, LeaderSendsAppendEntriesUponClientTransaction) {
  EXPECT_CALL(mock_call, Call(_, _, _)).Times(3);
  EXPECT_CALL(*leader_election_manager_, OnAeBroadcast()).Times(1);

  auto req = std::make_unique<Request>();
  req->set_seq(1);
  raft_->SetRole(Role::LEADER);

  bool success = raft_->ReceiveTransaction(std::move(req));
  EXPECT_TRUE(success);
}

// Test 3: Sent AppendEntries should be based on the follower's nextIndex.
TEST_F(RaftTest, LeaderSendsAppendEntriesBasedOnNextIndex) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 2);
            EXPECT_EQ(ae.prevlogindex(), 2);
            EXPECT_EQ(ae.entries().size(), 3);
            return 0;
          }))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 3);
            EXPECT_EQ(ae.prevlogindex(), 1);
            EXPECT_EQ(ae.entries().size(), 4);
            return 0;
          }))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 4);
            EXPECT_EQ(ae.prevlogindex(), 0);
            EXPECT_EQ(ae.entries().size(), 5);
            return 0;
          }));
  EXPECT_CALL(*leader_election_manager_, OnAeBroadcast()).Times(1);

  raft_->SetStateForTest({.currentTerm = 0,
                          .role = Role::LEADER,
                          .log = CreateLogEntries(
                              {
                                  {0, "Term 0 Transaction 1"},
                                  {0, "Term 0 Transaction 2"},
                                  {0, "Term 0 Transaction 3"},
                                  {0, "Term 0 Transaction 4"},
                              },
                              true),
                          .nextIndex = std::vector<uint64_t>{0, 4, 3, 2, 1}});

  auto req = std::make_unique<Request>();
  req->set_seq(5);

  bool success = raft_->ReceiveTransaction(std::move(req));
  EXPECT_TRUE(success);
}

// Test 4: A follower receiving 1 AppendEntries with multiple entries that it
// can accept.
TEST_F(RaftTest, FollowerAddsAppendEntriesWithMultipleEntries) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_TRUE(aer.success());
            EXPECT_EQ(aer.lastlogindex(), 3);
            return 0;
          }));
  EXPECT_CALL(*leader_election_manager_, OnHeartBeat()).Times(1);

  auto aefields = CreateAeFields(
      /*term=*/0,
      /*leaderId=*/2,
      /*prevLogIndex=*/0,
      /*prevLogTerm=*/0,
      /*entries=*/
      CreateLogEntries({
          {0, "Transaction 1"},
          {0, "Transaction 2"},
          {0, "Transaction 3"},
      }),
      /*leaderCommit=*/0,
      /*followerId=*/1);

  auto aemessage = CreateAeMessage(aefields);
  raft_->SetRole(Role::FOLLOWER);

  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(aemessage)));
  EXPECT_TRUE(success);
}

// Test 5: A follower receiving multiple AppendEntries that it can accept.
TEST_F(RaftTest, FollowerAddsMultipleAppendEntries) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_TRUE(aer.success());
            EXPECT_EQ(aer.lastlogindex(), 1);
            return 0;
          }))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_TRUE(aer.success());
            EXPECT_EQ(aer.lastlogindex(), 2);
            return 0;
          }))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_TRUE(aer.success());
            EXPECT_EQ(aer.lastlogindex(), 3);
            return 0;
          }));
  EXPECT_CALL(*leader_election_manager_, OnHeartBeat()).Times(3);

  auto aefields1 = CreateAeFields(
      /*term=*/0,
      /*leaderId=*/2,
      /*prevLogIndex=*/0,
      /*prevLogTerm=*/0,
      /*entries=*/
      CreateLogEntries({
          {0, "Transaction 1"},
      }),
      /*leaderCommit=*/0,
      /*followerId=*/1);

  auto aefields2 = CreateAeFields(
      /*term=*/0,
      /*leaderId=*/2,
      /*prevLogIndex=*/1,
      /*prevLogTerm=*/0,
      /*entries=*/
      CreateLogEntries({
          {0, "Transaction 2"},
      }),
      /*leaderCommit=*/0,
      /*followerId=*/1);

  auto aefields3 = CreateAeFields(
      /*term=*/0,
      /*leaderId=*/2,
      /*prevLogIndex=*/2,
      /*prevLogTerm=*/0,
      /*entries=*/
      CreateLogEntries({
          {0, "Transaction 3"},
      }),
      /*leaderCommit=*/0,
      /*followerId=*/1);

  auto aemessage1 = CreateAeMessage(aefields1);
  auto aemessage2 = CreateAeMessage(aefields2);
  auto aemessage3 = CreateAeMessage(aefields3);
  raft_->SetRole(Role::FOLLOWER);

  bool success1 = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(aemessage1)));
  EXPECT_TRUE(success1);

  bool success2 = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(aemessage2)));
  EXPECT_TRUE(success2);

  bool success3 = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(aemessage3)));
  EXPECT_TRUE(success3);
}

// Test 6: A follower rejects Append Entries because its own entry at
// prevLogIndex does not have the same term.
TEST_F(RaftTest, FollowerRejectsMismatchedTermAtPrevLogIndex) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_FALSE(aer.success());
            EXPECT_EQ(aer.lastlogindex(), 1);
            return 0;
          }));
  EXPECT_CALL(*leader_election_manager_, OnHeartBeat()).Times(1);

  auto aefields = CreateAeFields(
      /*term=*/0,
      /*leaderId=*/2,
      /*prevLogIndex=*/1,
      /*prevLogTerm=*/2,
      /*entries=*/
      CreateLogEntries({
          {2, "Term 2 Transaction 1"},
      }),
      /*leaderCommit=*/0,
      /*followerId=*/1);

  raft_->SetStateForTest({
      .currentTerm = 0,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries(
          {
              {1, "Term 1 Transaction 1"},
          },
          true),
  });

  auto aemessage = CreateAeMessage(aefields);
  raft_->SetRole(Role::FOLLOWER);

  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(aemessage)));
  EXPECT_TRUE(success);
}

// Test 7: A follower rejects Append Entries because it does not have a term at
// prevLogIndex.
TEST_F(RaftTest, FollowerRejectsMissingIndex) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_FALSE(aer.success());
            EXPECT_EQ(aer.lastlogindex(), 0);
            return 0;
          }));
  EXPECT_CALL(*leader_election_manager_, OnHeartBeat()).Times(1);

  auto aefields = CreateAeFields(
      /*term=*/0,
      /*leaderId=*/2,
      /*prevLogIndex=*/1,
      /*prevLogTerm=*/0,
      /*entries=*/
      CreateLogEntries({
          {0, "Transaction 2"},
      }),
      /*leaderCommit=*/0,
      /*followerId=*/1);

  auto aemessage = CreateAeMessage(aefields);
  raft_->SetRole(Role::FOLLOWER);

  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(aemessage)));
  EXPECT_TRUE(success);
}

// Test 8: A follower receiving 1 AppendEntries with multiple entries and
// needing to truncate part of its log.
TEST_F(RaftTest, FollowerAddsAppendEntriesAndTruncatesLog) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_TRUE(aer.success());
            EXPECT_EQ(aer.lastlogindex(), 3);
            return 0;
          }));
  EXPECT_CALL(*leader_election_manager_, OnHeartBeat()).Times(1);

  auto aefields = CreateAeFields(
      /*term=*/1,
      /*leaderId=*/2,
      /*prevLogIndex=*/1,
      /*prevLogTerm=*/0,
      /*entries=*/
      CreateLogEntries({
          {1, "Term 1 Transaction 1"},
          {1, "Term 1 Transaction 2"},
      }),
      /*leaderCommit=*/0,
      /*followerId=*/1);
  auto aemessage = CreateAeMessage(aefields);

  raft_->SetStateForTest({
      .currentTerm = 0,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries(
          {
              {0, "Term 0 Transaction 1"},  // index 1
              {0, "Term 0 Transaction 2"},  // mismatched entry will be removed
          },
          true),
  });

  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(aemessage)));

  const auto& raft_log = raft_->GetLog();
  EXPECT_EQ(raft_log[0]->term, 0);
  EXPECT_EQ(raft_log[0]->command, "COMMON_PREFIX");
  EXPECT_EQ(raft_log[1]->term, 0);
  // TODO: Use serialized string instead of manually doing it.
  EXPECT_EQ(raft_log[1]->command, "\n\x14Term 0 Transaction 1");
  EXPECT_EQ(raft_log[2]->term, 1);
  EXPECT_EQ(raft_log[2]->command, "\n\x14Term 1 Transaction 1");
  EXPECT_EQ(raft_log[3]->term, 1);
  EXPECT_EQ(raft_log[3]->command, "\n\x14Term 1 Transaction 2");
  EXPECT_TRUE(success);
}

// Test 9: A follower increases its commitIndex.
TEST_F(RaftTest, FollowerIncreasesCommitIndex) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_TRUE(aer.success());
            EXPECT_EQ(aer.lastlogindex(), 5);
            return 0;
          }));
  EXPECT_CALL(mock_commit, Commit(_)).Times(2);
  EXPECT_CALL(*leader_election_manager_, OnHeartBeat()).Times(1);

  auto aefields = CreateAeFields(
      /*term=*/1,
      /*leaderId=*/2,
      /*prevLogIndex=*/5,
      /*prevLogTerm=*/1,
      /*entries=*/CreateLogEntries({}),
      /*leaderCommit=*/3,
      /*followerId=*/1);
  auto aemessage = CreateAeMessage(aefields);

  raft_->SetStateForTest({
      .currentTerm = 1,
      .commitIndex = 1,
      .lastApplied = 1,
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
      std::make_unique<AppendEntries>(std::move(aemessage)));

  EXPECT_TRUE(success);
  EXPECT_EQ(raft_->GetCommitIndex(), 3);
}

// Test 10: A follower increases its commitIndex, but not past its own log size.
TEST_F(RaftTest, FollowerIncreasesCommitIndexCappedAtLogSize) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_TRUE(aer.success());
            EXPECT_EQ(aer.lastlogindex(), 5);
            return 0;
          }));
  EXPECT_CALL(mock_commit, Commit(_)).Times(4);
  EXPECT_CALL(*leader_election_manager_, OnHeartBeat()).Times(1);

  auto aefields = CreateAeFields(
      /*term=*/1,
      /*leaderId=*/2,
      /*prevLogIndex=*/5,
      /*prevLogTerm=*/1,
      /*entries=*/CreateLogEntries({}),
      /*leaderCommit=*/7,
      /*followerId=*/1);
  auto aemessage = CreateAeMessage(aefields);

  raft_->SetStateForTest({
      .currentTerm = 1,
      .commitIndex = 1,
      .lastApplied = 1,
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
      std::make_unique<AppendEntries>(std::move(aemessage)));

  EXPECT_TRUE(success);
  EXPECT_EQ(raft_->GetCommitIndex(), 5);
}

// Test 11: A candidate rejecting an AppendEntries from an outdated term and
// staying candidate.
TEST_F(RaftTest, CandidateRejectsAppendEntriesFromOutdatedTerm) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_FALSE(aer.success());
            EXPECT_EQ(aer.lastlogindex(), 0);
            return 0;
          }));
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(0);
  EXPECT_CALL(*leader_election_manager_, OnHeartBeat()).Times(0);

  auto aefields = CreateAeFields(
      /*term=*/1,
      /*leaderId=*/2,
      /*prevLogIndex=*/0,
      /*prevLogTerm=*/0,
      /*entries=*/
      CreateLogEntries({
          {1, "Transaction 1"},
          {1, "Transaction 2"},
          {1, "Transaction 3"},
      }),
      /*leaderCommit=*/0,
      /*followerId=*/1);
  auto aemessage = CreateAeMessage(aefields);

  raft_->SetStateForTest({
      .currentTerm = 2,
      .role = Role::CANDIDATE,
      .log = CreateLogEntries({}, true),
  });

  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(aemessage)));
  EXPECT_TRUE(success);
}

// Test 12: A candidate rejecting an AppendEntries because their log is further
// behind, but it is in the same term so they still demote.
TEST_F(RaftTest, CandidateRejectsAppendEntriesFromSameTerm) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_FALSE(aer.success());
            EXPECT_EQ(aer.lastlogindex(), 1);
            return 0;
          }));
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(1);
  EXPECT_CALL(*leader_election_manager_, OnHeartBeat()).Times(1);

  auto aefields = CreateAeFields(
      /*term=*/2,
      /*leaderId=*/2,
      /*prevLogIndex=*/2,
      /*prevLogTerm=*/0,
      /*entries=*/
      CreateLogEntries({
          {2, "Transaction 1"},
          {2, "Transaction 2"},
          {2, "Transaction 3"},
      }),
      /*leaderCommit=*/0,
      /*followerId=*/1);
  auto aemessage = CreateAeMessage(aefields);

  raft_->SetStateForTest({
      .currentTerm = 2,
      .role = Role::CANDIDATE,
      .log = CreateLogEntries({{1, "Old Transaction 1"}}, true),
  });

  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(aemessage)));
  EXPECT_TRUE(success);
}

// Test 13: A candidate receiving an AppendEntries it can accept from a newer
// term.
TEST_F(RaftTest, CandidateReceivesNewerTermWithAppendEntriesItCanAccept) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_TRUE(aer.success());
            EXPECT_EQ(aer.lastlogindex(), 3);
            return 0;
          }));
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(1);
  EXPECT_CALL(*leader_election_manager_, OnHeartBeat()).Times(1);

  auto aefields = CreateAeFields(
      /*term=*/2,
      /*leaderId=*/2,
      /*prevLogIndex=*/2,
      /*prevLogTerm=*/0,
      /*entries=*/
      CreateLogEntries({
          {2, "Transaction 1"},
      }),
      /*leaderCommit=*/2,
      /*followerId=*/1);
  auto aemessage = CreateAeMessage(aefields);

  raft_->SetStateForTest({
      .currentTerm = 1,
      .lastApplied = 2,
      .role = Role::CANDIDATE,
      .log = CreateLogEntries(
          {
              {0, "old-1"},
              {0, "old-2"},
          },
          true),
  });

  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(aemessage)));
  EXPECT_TRUE(success);
  EXPECT_EQ(raft_->GetRoleSnapshot(), Role::FOLLOWER);
}

// Test 14: A candidate receiving an AppendEntries that it can accept from a the
// same term but further along.
TEST_F(RaftTest, CandidateReceivesSameTermWithAppendEntriesItCanAccept) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_TRUE(aer.success());
            EXPECT_EQ(aer.lastlogindex(), 3);
            return 0;
          }));
  EXPECT_CALL(*leader_election_manager_, OnRoleChange()).Times(1);
  EXPECT_CALL(*leader_election_manager_, OnHeartBeat()).Times(1);

  auto aefields = CreateAeFields(
      /*term=*/1,
      /*leaderId=*/2,
      /*prevLogIndex=*/2,
      /*prevLogTerm=*/0,
      /*entries=*/
      CreateLogEntries({
          {2, "Transaction 1"},
      }),
      /*leaderCommit=*/2,
      /*followerId=*/1);
  auto aemessage = CreateAeMessage(aefields);

  raft_->SetStateForTest({
      .currentTerm = 1,
      .lastApplied = 2,
      .role = Role::CANDIDATE,
      .log = CreateLogEntries(
          {
              {0, "old-1"},
              {0, "old-2"},
          },
          true),
  });

  bool success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(aemessage)));
  EXPECT_TRUE(success);
  EXPECT_EQ(raft_->GetRoleSnapshot(), Role::FOLLOWER);
}

}  // namespace raft
}  // namespace resdb

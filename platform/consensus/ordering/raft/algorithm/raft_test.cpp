#include <gmock/gmock.h>

#include "platform/config/resdb_config_utils.h"
#include "common/crypto/mock_signature_verifier.h"
#include "platform/networkstrate/mock_replica_communicator.h"
#include "platform/consensus/ordering/raft/algorithm/mock_leader_election_manager.h"
#include "platform/consensus/ordering/raft/algorithm/raft.h"
#include "platform/proto/client_test.pb.h"

namespace resdb {
namespace raft {
using ::testing::Invoke;
using ::testing::_;
using ::testing::Matcher;
using ::testing::AnyNumber;

ResDBConfig GenerateConfig() {
  ResConfigData data;
  data.set_duplicate_check_frequency_useconds(100000);
  data.set_enable_viewchange(true);
  return ResDBConfig({GenerateReplicaInfo(1, "127.0.0.1", 1234),
                      GenerateReplicaInfo(2, "127.0.0.1", 1235),
                      GenerateReplicaInfo(3, "127.0.0.1", 1236),
                      GenerateReplicaInfo(4, "127.0.0.1", 1237)},
                     GenerateReplicaInfo(1, "127.0.0.1", 1234), data);
}

class RaftTest : public ::testing::Test {
 private:
  class MockSendMessageFunction {
  public:
      MOCK_METHOD(int, Call, (int, const google::protobuf::Message&, int));
  };
  class MockBroadcastFunction {
  public:
      MOCK_METHOD(int, Broadcast, (int, const google::protobuf::Message&));
  };
  class MockCommitFunction {
  public:
      MOCK_METHOD(int, Commit, (const google::protobuf::Message&));
  };

 protected:
  void SetUp() override {
    verifier_ = std::make_unique<MockSignatureVerifier>();
    leader_election_manager_ = std::make_unique<MockLeaderElectionManager>(GenerateConfig());
    replica_communicator_ = std::make_unique<MockReplicaCommunicator>();
    raft_ = std::make_unique<Raft>(
        /*id=*/1,
        /*f=*/1,
        /*total=*/4,
        verifier_.get(),
        leader_election_manager_.get(),
        replica_communicator_.get());

    raft_->SetSingleCallFunc(
    [&](int type, const google::protobuf::Message& msg, int node_id) {
        return mock_call.Call(type, msg, node_id);
    });

    raft_->SetBroadcastCallFunc(
    [&](int type, const google::protobuf::Message& msg) {
        return mock_broadcast.Broadcast(type, msg);
    });

    raft_->SetCommitFunc(
    [&](const google::protobuf::Message& msg) {
        return mock_commit.Commit(msg);
    });
  }

  AeFields CreateAeFields(uint64_t term, int leaderId, uint64_t prevLogIndex, uint64_t prevLogTerm, const std::vector<std::unique_ptr<LogEntry>>& entries, uint64_t leaderCommit, int followerId)  {
    AeFields fields{};
    fields.term = term;
    fields.leaderId = leaderId;
    fields.leaderCommit = leaderCommit;
    fields.prevLogIndex = prevLogIndex;
    fields.prevLogTerm = prevLogTerm;
    fields.followerId = followerId;

    for (const auto& logEntry : entries) {
      LogEntry entry;
      entry.term = logEntry->term;
      entry.command = logEntry->command;
      fields.entries.push_back(std::move(entry));
    }

    return fields;
  };

  // Helper to create a single log entry
  std::unique_ptr<LogEntry> CreateLogEntry(uint64_t term, const std::string& command_data) {
    auto entry = std::make_unique<LogEntry>();
    entry->term = term;
    entry->command = command_data;
    return entry;
  }

  // Helper to create a vector of log entries for testing
  std::vector<std::unique_ptr<LogEntry>> CreateLogEntries(const std::vector<std::pair<uint64_t, std::string>>& term_and_cmds, bool usedForLogPatch = false) {
    std::vector<std::unique_ptr<LogEntry>> entries;

    if (usedForLogPatch) {
      std::unique_ptr<LogEntry> first_entry = std::make_unique<LogEntry>();
      first_entry->term = 0;
      first_entry->command = "COMMON_PREFIX";
      entries.push_back(std::move(first_entry));
    }
    
    for (const auto& [term, cmd] : term_and_cmds) {
        std::unique_ptr<LogEntry> entry = std::make_unique<LogEntry>();
        entry->term = term;
        // entry->command = cmd;

        ClientTestRequest req;
        req.set_value(cmd);
        std::string serialized;
        req.SerializeToString(&serialized);
        entry->command = serialized;
        entries.push_back(std::move(entry));
    }
    return entries;
  }

  AppendEntries CreateAeMessage(const AeFields& fields) {
    AppendEntries ae;
    ae.set_term(fields.term);
    ae.set_leaderid(fields.leaderId);
    ae.set_prevlogindex(fields.prevLogIndex);
    ae.set_prevlogterm(fields.prevLogTerm);
    ae.set_leadercommitindex(fields.leaderCommit);
    for (const auto& entry : fields.entries) {
      auto* newEntry = ae.add_entries();
      newEntry->set_term(entry.term);
      newEntry->set_command(entry.command);
    }

    return ae;
  }

  std::unique_ptr<MockSignatureVerifier> verifier_;
  std::unique_ptr<MockLeaderElectionManager> leader_election_manager_;
  std::unique_ptr<MockReplicaCommunicator> replica_communicator_;
  std::unique_ptr<Raft> raft_;
  MockSendMessageFunction mock_call;
  MockBroadcastFunction mock_broadcast;
  MockCommitFunction mock_commit;
};

// Test 1: A follower receiving a client transaction should reject it
TEST_F(RaftTest, FollowerRejectsClientTransaction) {
  EXPECT_CALL(mock_call, Call(_, _, _)).Times(0);
  EXPECT_CALL(mock_broadcast, Broadcast(_, _)).Times(0);

  auto req = std::make_unique<Request>();
  req->set_seq(1);
  raft_->SetRole(Role::FOLLOWER);

  bool success = raft_->ReceiveTransaction(std::move(req));
  EXPECT_FALSE(success);
}

// Test 2: A leader receiving a client transaction should send an AppendEntries to all other replicas
TEST_F(RaftTest, LeaderSendsAppendEntriesUponClientTransaction) {
  EXPECT_CALL(mock_call, Call(_, _, _)).Times(3);

  auto req = std::make_unique<Request>();
  req->set_seq(1);
  raft_->SetRole(Role::LEADER);

  bool success = raft_->ReceiveTransaction(std::move(req));
  EXPECT_TRUE(success);
}

// Test 3: Sent AppendEntries should be based on the follower's nextIndex
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
  
  raft_->SetStateForTest({
      .currentTerm = 0,
      .role = Role::LEADER,
      .log = CreateLogEntries({
        {0, "Term 0 Transaction 1"},
        {0, "Term 0 Transaction 2"},
        {0, "Term 0 Transaction 3"},
        {0, "Term 0 Transaction 4"},
      }, true), 
      .nextIndex = std::vector<uint64_t>{0, 4, 3, 2, 1}
  });

  auto req = std::make_unique<Request>();
  req->set_seq(5);

  bool success = raft_->ReceiveTransaction(std::move(req));
  EXPECT_TRUE(success);
}

// Test 4: A follower receiving 1 AppendEntries with multiple entries that it can accept
TEST_F(RaftTest, FollowerAddsAppendEntriesWithMultipleEntries) {
  EXPECT_CALL(mock_call, Call(_, _, _))
    .WillOnce(::testing::Invoke(
        [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_TRUE(aer.success());
            EXPECT_EQ(aer.lastlogindex(), 3);
            return 0;
        }));

  auto aefields = CreateAeFields(
    /*term=*/ 0,
    /*leaderId=*/ 2,
    /*prevLogIndex=*/ 0,
    /*prevLogTerm=*/ 0,
    /*entries=*/ CreateLogEntries({
        {0, "Transaction 1"},
        {0, "Transaction 2"},
        {0, "Transaction 3"},
    }),
    /*leaderCommit=*/ 0,
    /*followerId=*/ 1
  );

  auto aemessage = CreateAeMessage(aefields);
  raft_->SetRole(Role::FOLLOWER);

  bool success = raft_->ReceiveAppendEntries(std::make_unique<AppendEntries>(std::move(aemessage)));
  EXPECT_TRUE(success);
}

// Test 5: A follower receiving multiple AppendEntries that it can accept
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

  auto aefields1 = CreateAeFields(
    /*term=*/ 0,
    /*leaderId=*/ 2,
    /*prevLogIndex=*/ 0,
    /*prevLogTerm=*/ 0,
    /*entries=*/ CreateLogEntries({
        {0, "Transaction 1"},
    }),
    /*leaderCommit=*/ 0,
    /*followerId=*/ 1
  );

  auto aefields2 = CreateAeFields(
    /*term=*/ 0,
    /*leaderId=*/ 2,
    /*prevLogIndex=*/ 1,
    /*prevLogTerm=*/ 0,
    /*entries=*/ CreateLogEntries({
        {0, "Transaction 2"},
    }),
    /*leaderCommit=*/ 0,
    /*followerId=*/ 1
  );

  auto aefields3 = CreateAeFields(
    /*term=*/ 0,
    /*leaderId=*/ 2,
    /*prevLogIndex=*/ 2,
    /*prevLogTerm=*/ 0,
    /*entries=*/ CreateLogEntries({
        {0, "Transaction 3"},
    }),
    /*leaderCommit=*/ 0,
    /*followerId=*/ 1
  );

  auto aemessage1 = CreateAeMessage(aefields1);
  auto aemessage2 = CreateAeMessage(aefields2);
  auto aemessage3 = CreateAeMessage(aefields3);
  raft_->SetRole(Role::FOLLOWER);

  bool success1 = raft_->ReceiveAppendEntries(std::make_unique<AppendEntries>(std::move(aemessage1)));
  EXPECT_TRUE(success1);

  bool success2 = raft_->ReceiveAppendEntries(std::make_unique<AppendEntries>(std::move(aemessage2)));
  EXPECT_TRUE(success2);

  bool success3 = raft_->ReceiveAppendEntries(std::make_unique<AppendEntries>(std::move(aemessage3)));
  EXPECT_TRUE(success3);
}

// Test 6: A follower rejects Append Entries because its own entry at prevLogIndex does not have the same term.
TEST_F(RaftTest, FollowerRejectsMismatchedTermAtPrevLogIndex) {
  EXPECT_CALL(mock_call, Call(_, _, _))
    .WillOnce(::testing::Invoke(
        [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_FALSE(aer.success());
            EXPECT_EQ(aer.lastlogindex(), 1);
            return 0;
        }));

  auto aefields = CreateAeFields(
    /*term=*/ 0,
    /*leaderId=*/ 2,
    /*prevLogIndex=*/ 1,
    /*prevLogTerm=*/ 2,
    /*entries=*/ CreateLogEntries({
        {2, "Term 2 Transaction 1"},
    }),
    /*leaderCommit=*/ 0,
    /*followerId=*/ 1
  );

  raft_->SetStateForTest({
      .currentTerm = 0,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries({
        {1, "Term 1 Transaction 1"},
      }, true), 
  });

  auto aemessage = CreateAeMessage(aefields);
  raft_->SetRole(Role::FOLLOWER);

  bool success = raft_->ReceiveAppendEntries(std::make_unique<AppendEntries>(std::move(aemessage)));
  EXPECT_TRUE(success);
}

// Test 7: A follower rejects Append Entries because it does not have a term at prevLogIndex
TEST_F(RaftTest, FollowerRejectsMissingIndex) {
  EXPECT_CALL(mock_call, Call(_, _, _))
    .WillOnce(::testing::Invoke(
        [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_FALSE(aer.success());
            EXPECT_EQ(aer.lastlogindex(), 0);
            return 0;
        }));

  auto aefields = CreateAeFields(
    /*term=*/ 0,
    /*leaderId=*/ 2,
    /*prevLogIndex=*/ 1,
    /*prevLogTerm=*/ 0,
    /*entries=*/ CreateLogEntries({
        {0, "Transaction 2"},
    }),
    /*leaderCommit=*/ 0,
    /*followerId=*/ 1
  );

  auto aemessage = CreateAeMessage(aefields);
  raft_->SetRole(Role::FOLLOWER);

  bool success = raft_->ReceiveAppendEntries(std::make_unique<AppendEntries>(std::move(aemessage)));
  EXPECT_TRUE(success);
}

// Test 8: A follower receiving 1 AppendEntries with multiple entries and needing to truncate part of its log
TEST_F(RaftTest, FollowerAddsAppendEntriesAndTruncatesLog) {
  EXPECT_CALL(mock_call, Call(_, _, _))
    .WillOnce(::testing::Invoke(
        [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_TRUE(aer.success());
            EXPECT_EQ(aer.lastlogindex(), 3);
            return 0;
        }));

  auto aefields = CreateAeFields(
    /*term=*/ 1,
    /*leaderId=*/ 2,
    /*prevLogIndex=*/ 1,
    /*prevLogTerm=*/ 0,
    /*entries=*/ CreateLogEntries({
        {1, "Term 1 Transaction 1"},
        {1, "Term 1 Transaction 2"},
    }),
    /*leaderCommit=*/ 0,
    /*followerId=*/ 1
  );
  auto aemessage = CreateAeMessage(aefields);

  raft_->SetStateForTest({
      .currentTerm = 0,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries({
        {0, "Term 0 Transaction 1"}, // index 1
        {0, "Term 0 Transaction 2"}, // mismatched entry will be removed
      }, true), 
  });

  bool success = raft_->ReceiveAppendEntries(std::make_unique<AppendEntries>(std::move(aemessage)));

  const auto& raft_log = raft_->GetLog();
  EXPECT_EQ(raft_log[0]->term, 0);
  EXPECT_EQ(raft_log[0]->command, "COMMON_PREFIX");
  EXPECT_EQ(raft_log[1]->term, 0);
  // TODO: Use serialized string instead of manually doing it
  EXPECT_EQ(raft_log[1]->command, "\n\x14Term 0 Transaction 1");
  EXPECT_EQ(raft_log[2]->term, 1);
  EXPECT_EQ(raft_log[2]->command, "\n\x14Term 1 Transaction 1");
  EXPECT_EQ(raft_log[3]->term, 1);
  EXPECT_EQ(raft_log[3]->command, "\n\x14Term 1 Transaction 2");
  EXPECT_TRUE(success);
}

// Test 9: A follower increases its commitIndex
TEST_F(RaftTest, FollowerIncreasesCommitIndex) {
  EXPECT_CALL(mock_call, Call(_, _, _))
    .WillOnce(::testing::Invoke(
        [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_TRUE(aer.success());
            EXPECT_EQ(aer.lastlogindex(), 5);
            return 0;
        }));
  EXPECT_CALL(mock_commit, Commit(_))
    .Times(2);

  auto aefields = CreateAeFields(
    /*term=*/ 1,
    /*leaderId=*/ 2,
    /*prevLogIndex=*/ 5,
    /*prevLogTerm=*/ 1,
    /*entries=*/ CreateLogEntries({}),
    /*leaderCommit=*/ 3,
    /*followerId=*/ 1
  );
  auto aemessage = CreateAeMessage(aefields);

  raft_->SetStateForTest({
      .currentTerm = 1,
      .commitIndex = 1,
      .lastApplied = 1,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries({
        {1, "Term 1 Transaction 1"},
        {1, "Term 1 Transaction 2"},
        {1, "Term 1 Transaction 3"},
        {1, "Term 1 Transaction 4"},
        {1, "Term 1 Transaction 5"},
      }, true), 
  });

  bool success = raft_->ReceiveAppendEntries(std::make_unique<AppendEntries>(std::move(aemessage)));

  EXPECT_TRUE(success);
  EXPECT_EQ(raft_->GetCommitIndex(), 3);
}

// Test 10: A follower increases its commitIndex, but not past its own log size
TEST_F(RaftTest, FollowerIncreasesCommitIndexCappedAtLogSize) {
  EXPECT_CALL(mock_call, Call(_, _, _))
    .WillOnce(::testing::Invoke(
        [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_TRUE(aer.success());
            EXPECT_EQ(aer.lastlogindex(), 5);
            return 0;
        }));
  EXPECT_CALL(mock_commit, Commit(_))
    .Times(4);

  auto aefields = CreateAeFields(
    /*term=*/ 1,
    /*leaderId=*/ 2,
    /*prevLogIndex=*/ 5,
    /*prevLogTerm=*/ 1,
    /*entries=*/ CreateLogEntries({}),
    /*leaderCommit=*/ 7,
    /*followerId=*/ 1
  );
  auto aemessage = CreateAeMessage(aefields);

  raft_->SetStateForTest({
      .currentTerm = 1,
      .commitIndex = 1,
      .lastApplied = 1,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries({
        {1, "Term 1 Transaction 1"},
        {1, "Term 1 Transaction 2"},
        {1, "Term 1 Transaction 3"},
        {1, "Term 1 Transaction 4"},
        {1, "Term 1 Transaction 5"},
      }, true), 
  });

  bool success = raft_->ReceiveAppendEntries(std::make_unique<AppendEntries>(std::move(aemessage)));

  EXPECT_TRUE(success);
  EXPECT_EQ(raft_->GetCommitIndex(), 5);
}

// Test 11: A candidate rejecting an AppendEntries from an outdated term and staying candidate
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
  
  auto aefields = CreateAeFields(
    /*term=*/ 1,
    /*leaderId=*/ 2,
    /*prevLogIndex=*/ 0,
    /*prevLogTerm=*/ 0,
    /*entries=*/ CreateLogEntries({
        {1, "Transaction 1"},
        {1, "Transaction 2"},
        {1, "Transaction 3"},
    }),
    /*leaderCommit=*/ 0,
    /*followerId=*/ 1
  );
  auto aemessage = CreateAeMessage(aefields);

  raft_->SetStateForTest({
      .currentTerm = 2,
      .role = Role::CANDIDATE,
      .log = CreateLogEntries({
      }, true), 
  });


  bool success = raft_->ReceiveAppendEntries(std::make_unique<AppendEntries>(std::move(aemessage)));
  EXPECT_TRUE(success);
}

// Test 12: A candidate rejecting an AppendEntries because their log is further behind, but it is in the same term so they still demote.
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
  
  auto aefields = CreateAeFields(
    /*term=*/ 2,
    /*leaderId=*/ 2,
    /*prevLogIndex=*/ 2,
    /*prevLogTerm=*/ 0,
    /*entries=*/ CreateLogEntries({
        {2, "Transaction 1"},
        {2, "Transaction 2"},
        {2, "Transaction 3"},
    }),
    /*leaderCommit=*/ 0,
    /*followerId=*/ 1
  );
  auto aemessage = CreateAeMessage(aefields);

  raft_->SetStateForTest({
      .currentTerm = 2,
      .role = Role::CANDIDATE,
      .log = CreateLogEntries({
        {1, "Old Transaction 1"}
      }, true), 
  });


  bool success = raft_->ReceiveAppendEntries(std::make_unique<AppendEntries>(std::move(aemessage)));
  EXPECT_TRUE(success);
}

// Test 13: A candidate receiving an AppendEntries with multiple entries that it can accept from a newer term.
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

  auto aefields = CreateAeFields(
    /*term=*/ 2,
    /*leaderId=*/ 2,
    /*prevLogIndex=*/ 2,
    /*prevLogTerm=*/ 0,
    /*entries=*/ CreateLogEntries({
        {2, "Transaction 1"},
    }),
    /*leaderCommit=*/ 2,
    /*followerId=*/ 1
  );
  auto aemessage = CreateAeMessage(aefields);

  raft_->SetStateForTest({
      .currentTerm = 1,
      .commitIndex = 2,
      .role = Role::CANDIDATE,
      .log = CreateLogEntries({
          {0, "old-1"}, 
          {0, "old-2"},
      }, true), 
  });
  
  bool success = raft_->ReceiveAppendEntries(std::make_unique<AppendEntries>(std::move(aemessage)));
  EXPECT_TRUE(success);
  EXPECT_EQ(raft_->GetRoleSnapshot(), Role::FOLLOWER);
}

// Test 14: A candidate receiving an AppendEntries with multiple entries that it can accept from a the same term but further along.
TEST_F(RaftTest, CandidateReceivesSameTermWithAppendEntriesItCanAccept) {
  EXPECT_CALL(mock_call, Call(_, _, _))
    .WillOnce(::testing::Invoke(
        [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_TRUE(aer.success());
            EXPECT_EQ(aer.lastlogindex(), 3);
            return 0;
        }));
  EXPECT_CALL(*leader_election_manager_, OnRoleChange())
    .Times(1);

  auto aefields = CreateAeFields(
    /*term=*/ 1,
    /*leaderId=*/ 2,
    /*prevLogIndex=*/ 2,
    /*prevLogTerm=*/ 0,
    /*entries=*/ CreateLogEntries({
        {2, "Transaction 1"},
    }),
    /*leaderCommit=*/ 2,
    /*followerId=*/ 1
  );
  auto aemessage = CreateAeMessage(aefields);

  raft_->SetStateForTest({
      .currentTerm = 1,
      .commitIndex = 2,
      .role = Role::CANDIDATE,
      .log = CreateLogEntries({
          {0, "old-1"}, 
          {0, "old-2"},
      }, true), 
  });
  
  bool success = raft_->ReceiveAppendEntries(std::make_unique<AppendEntries>(std::move(aemessage)));
  EXPECT_TRUE(success);
  EXPECT_EQ(raft_->GetRoleSnapshot(), Role::FOLLOWER);
}

} // namespace raft
} // namespace resdb

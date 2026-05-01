// raft_integration_test.cpp
//
// Integration test: Raft state correctly restored after RecoverFromLogs().
// Uses a real RaftRecovery (seeded with WAL data) and a real Raft.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>

#include "platform/consensus/checkpoint/mock_checkpoint.h"
#include "platform/consensus/ordering/raft/algorithm/raft_test_util.h"
#include "platform/consensus/recovery/raft_recovery.h"

namespace resdb {
namespace raft {

using resdb::raft::test_utils::CreateAeFields;
using resdb::raft::test_utils::CreateAeMessage;
using resdb::raft::test_utils::CreateLogEntries;
using resdb::raft::test_utils::GenerateConfig;
using resdb::raft::test_utils::MockBroadcastFunction;
using resdb::raft::test_utils::MockCommitFunction;
using resdb::raft::test_utils::MockSendMessageFunction;
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Invoke;

namespace {

const std::string kLogPath = "./log/raft_integration_test_log";

ResDBConfig MakeConfig() {
  ResConfigData data;
  data.set_recovery_enabled(true);
  data.set_recovery_path(kLogPath);
  data.set_recovery_buffer_size(1024);
  data.set_recovery_ckpt_time_s(3);
  return ResDBConfig({GenerateReplicaInfo(1, "127.0.0.1", 1234),
                      GenerateReplicaInfo(2, "127.0.0.1", 1235),
                      GenerateReplicaInfo(3, "127.0.0.1", 1236),
                      GenerateReplicaInfo(4, "127.0.0.1", 1237)},
                     GenerateReplicaInfo(1, "127.0.0.1", 1234), data);
}

// Mirrors what Consensus::RecoverFromLogs() does.
void RecoverFromLogs(RaftRecovery& recovery, Raft& raft) {
  recovery.ReadLogs(
      [&](const RaftMetadata& metadata) {
        LOG(INFO) << "loading metadata file: term: " << metadata.current_term
                  << " votedFor: " << metadata.voted_for
                  << " snapshot_last_index: " << metadata.snapshot_last_index
                  << " snapshot_last_term: " << metadata.snapshot_last_term;
        raft.SetCurrentTerm(metadata.current_term, /*writeMetadata=*/false);
        raft.SetVotedFor(metadata.voted_for, /*writeMetadata=*/false);
        raft.SetSnapshotLastIndexAndTerm(metadata.snapshot_last_index,
                                         metadata.snapshot_last_term,
                                         /*writeMetadata=*/false);
      },
      [&](std::unique_ptr<WALRecord> record) {
        LOG(INFO) << "Replaying record with seq: " << record->seq();
        switch (record->payload_case()) {
          case WALRecord::kEntry: {
            LogEntry logEntry;
            logEntry.entry = record->entry();
            LOG(INFO) << "Adding entry from term: " << logEntry.entry.term();
            raft.AddToLog(logEntry, /*writeMetadata=*/false);
            break;
          }
          case WALRecord::kTruncation:
            raft.TruncateLog(record->truncation().truncate_from_index(),
                             /*writeMetadata=*/false);
            break;
          case WALRecord::PAYLOAD_NOT_SET:
            FAIL() << "Unexpected PAYLOAD_NOT_SET record";
            break;
        }
      },
      /*set_start_point=*/[](int) {});
}

}  // namespace

class RaftRecoveryIntegrationTest : public ::testing::Test {
 private:
  class MockCommitFunction {
   public:
    MOCK_METHOD(int, Commit, (const google::protobuf::Message&));
  };

 protected:
  void SetUp() override {
    std::filesystem::remove_all(std::filesystem::path(kLogPath).parent_path());
  }

  ResDBConfig config_ = MakeConfig();
  MockCheckPoint checkpoint_;
  MockSendMessageFunction mock_call;
  MockBroadcastFunction mock_broadcast;
  MockCommitFunction mock_commit;
};

// Test 1: Restore basic metadata and log entries.
TEST_F(RaftRecoveryIntegrationTest, RaftStateRestoredAfterRecovery) {
  {
    RaftRecovery recovery(config_, nullptr, nullptr, nullptr);

    recovery.WriteMetadata(/*current_term=*/5, /*voted_for=*/2,
                           /*snapshot_last_index=*/0, /*snapshot_last_term=*/0);

    for (int i = 1; i <= 3; ++i) {
      Entry e;
      e.set_term(i);
      ClientTestRequest req;
      req.set_value("cmd-" + std::to_string(i));
      req.SerializeToString(e.mutable_command());
      recovery.AddLogEntry(&e, i);
    }
  }

  MockSignatureVerifier verifier;
  ResDBConfig config = MakeConfig();
  MockLeaderElectionManager lem(config);
  MockReplicaCommunicator comm;
  MockCheckPoint ckpt;

  RaftRecovery recovery(config_, nullptr, nullptr, nullptr);

  Raft raft(/*id=*/1, /*f=*/1, /*total=*/4, &verifier, &lem, &comm, &recovery);

  RecoverFromLogs(recovery, raft);

  // --- Assertions ---
  EXPECT_EQ(raft.GetCurrentTerm(), 5u);
  EXPECT_EQ(raft.GetVotedFor(), 2);
  EXPECT_EQ(raft.GetSnapshotLastIndex(), 0u);

  // Log: index 0 is the sentinel (term=0), indices 1–3 are the replayed
  // entries.
  ASSERT_EQ(raft.GetLogSize(), 4u);
  for (int i = 1; i <= 3; ++i) {
    const auto& le = raft.GetLog()[i];
    EXPECT_EQ(le.entry.term(), i);
    ClientTestRequest req;
    req.ParseFromString(le.entry.command());
    EXPECT_EQ(req.value(), "cmd-" + std::to_string(i));
  }
}

// Test 2: Restore the log using a checkpoint and the Recovery WAL.
TEST_F(RaftRecoveryIntegrationTest,
       RaftStateRestoredAfterRecoveryWithCheckpoint) {
  EXPECT_CALL(mock_commit, Commit(_)).Times(2);
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& aer = dynamic_cast<const AppendEntriesResponse&>(msg);
            EXPECT_TRUE(aer.success());
            EXPECT_EQ(aer.lastlogindex(), 13);
            return 0;
          }));

  {
    std::promise<bool> insert_done, ckpt_fired;
    auto insert_done_future = insert_done.get_future();
    auto ckpt_fired_future = ckpt_fired.get_future();

    int call_count = 0;
    EXPECT_CALL(checkpoint_, GetStableCheckpoint())
        .WillRepeatedly(Invoke([&]() -> uint64_t {
          ++call_count;
          if (call_count == 1)
            insert_done_future.get();
          else if (call_count == 2)
            ckpt_fired.set_value(true);
          return 5;
        }));

    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);

    recovery.WriteMetadata(/*current_term=*/5, /*voted_for=*/2,
                           /*snapshot_last_index=*/5, /*snapshot_last_term=*/5);

    for (int i = 1; i <= 8; ++i) {
      Entry e;
      e.set_term(i);
      ClientTestRequest req;
      req.set_value("Transaction " + std::to_string(i));
      req.SerializeToString(e.mutable_command());
      recovery.AddLogEntry(&e, i);
    }

    insert_done.set_value(true);
    ckpt_fired_future.get();

    for (int i = 9; i <= 10; ++i) {
      Entry e;
      e.set_term(i);
      ClientTestRequest req;
      req.set_value("Transaction " + std::to_string(i));
      req.SerializeToString(e.mutable_command());
      recovery.AddLogEntry(&e, i);
    }
  }

  MockSignatureVerifier verifier;
  ResDBConfig config = MakeConfig();
  MockLeaderElectionManager lem(config);
  MockReplicaCommunicator comm;
  MockCheckPoint ckpt;

  RaftRecovery recovery(config_, nullptr, nullptr, nullptr);

  Raft raft(/*id=*/1, /*f=*/1, /*total=*/4, &verifier, &lem, &comm, &recovery);

  raft.SetCommitFunc([&](const google::protobuf::Message& msg) {
    return mock_commit.Commit(msg);
  });
  raft.SetSingleCallFunc(
      [&](int type, const google::protobuf::Message& msg, int node_id) {
        return mock_call.Call(type, msg, node_id);
      });

  RecoverFromLogs(recovery, raft);

  EXPECT_EQ(raft.GetCurrentTerm(), 5u);
  EXPECT_EQ(raft.GetVotedFor(), 2);
  EXPECT_EQ(raft.GetSnapshotLastIndex(), 5u);

  auto aefields = CreateAeFields(
      /*term=*/11,
      /*leaderId=*/2,
      /*prevLogIndex=*/10,
      /*prevLogTerm=*/10,
      /*entries=*/
      CreateLogEntries({
          {11, "Transaction 11"},
          {11, "Transaction 12"},
          {11, "Transaction 13"},
      }),
      /*leaderCommit=*/7,
      /*followerId=*/1);

  auto aemessage = CreateAeMessage(aefields);

  bool success = raft.ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(aemessage)));
  EXPECT_TRUE(success);

  EXPECT_EQ(raft.GetCurrentTerm(), 11u);
  // --- Assertions ---
  EXPECT_EQ(raft.GetVotedFor(), -1);
  EXPECT_EQ(raft.GetSnapshotLastIndex(), 5u);
  EXPECT_EQ(raft.GetLastLogIndex(), 13u);

  // Log: index 0 is the sentinel (term/index=5), indices 1–8 are the replayed
  // entries.
  ASSERT_EQ(raft.GetLogSize(), 9u);
  const auto& le = raft.GetLog()[0];
  EXPECT_EQ(le.entry.term(), 5);
  EXPECT_EQ(raft.GetLogTermAtIndex(5), 5);

  for (int i = 1; i < 8; ++i) {
    const auto& le = raft.GetLog()[i];
    if (i <= 5) {
      EXPECT_EQ(le.entry.term(), i + 5);
    } else {
      EXPECT_EQ(le.entry.term(), 11);
    }
    EXPECT_EQ(raft.GetLogTermAtIndex(i + 5), le.entry.term());
    ClientTestRequest req;
    req.ParseFromString(le.entry.command());
    EXPECT_EQ(req.value(), "Transaction " + std::to_string(i + 5));
    ClientTestRequest req2;
    auto log_entry = raft.GetLogEntryAtIndex(i + 5);
    req2.ParseFromString(log_entry.entry.command());
    EXPECT_EQ(req.value(), req2.value());
  }
}

// Test 3: Demotion (higher-term AppendEntries) triggers WriteMetadata, and the
// updated metadata is visible after recovery.
TEST_F(RaftRecoveryIntegrationTest, DemotionTriggersWriteMetadata) {
  {
    MockSignatureVerifier verifier;
    ResDBConfig config = MakeConfig();
    MockLeaderElectionManager lem(config);
    MockReplicaCommunicator comm;

    RaftRecovery recovery(config_, nullptr, nullptr, nullptr);

    Raft raft(/*id=*/1, /*f=*/1, /*total=*/4, &verifier, &lem, &comm,
              &recovery);

    recovery.WriteMetadata(/*current_term=*/3, /*voted_for=*/1,
                           /*snapshot_last_index=*/0,
                           /*snapshot_last_term=*/0);

    // Add a couple of entries so the log is non-trivial.
    for (int i = 1; i <= 2; ++i) {
      Entry e;
      e.set_term(3);
      ClientTestRequest req;
      req.set_value("cmd-" + std::to_string(i));
      req.SerializeToString(e.mutable_command());
      recovery.AddLogEntry(&e, i);
    }

    raft.SetStateForTest({
        .currentTerm = 6,
        .role = Role::LEADER,
        .log = CreateLogEntries({}, true),
    });

    raft.SetSingleCallFunc(
        [&](int type, const google::protobuf::Message& msg, int node_id) {
          return mock_call.Call(type, msg, node_id);
        });

    // Receive an AppendEntries from node 2 at a higher term.
    auto aefields = CreateAeFields(
        /*term=*/7,
        /*leaderId=*/2,
        /*prevLogIndex=*/0,
        /*prevLogTerm=*/0,
        /*entries=*/{},
        /*leaderCommit=*/0,
        /*followerId=*/1);
    auto aemessage = CreateAeMessage(aefields);

    raft.PrintDebugState();
    bool success = raft.ReceiveAppendEntries(
        std::make_unique<AppendEntries>(std::move(aemessage)));
    EXPECT_TRUE(success);

    EXPECT_EQ(raft.GetCurrentTerm(), 7u);
    EXPECT_EQ(raft.GetVotedFor(), -1);
  }

  {
    MockSignatureVerifier verifier;
    ResDBConfig config = MakeConfig();
    MockLeaderElectionManager lem(config);
    MockReplicaCommunicator comm;

    RaftRecovery recovery(config_, nullptr, nullptr, nullptr);

    Raft raft(/*id=*/1, /*f=*/1, /*total=*/4, &verifier, &lem, &comm,
              &recovery);

    RecoverFromLogs(recovery, raft);

    EXPECT_EQ(raft.GetCurrentTerm(), 7u);
    EXPECT_EQ(raft.GetVotedFor(), -1);

    // The two entries written before the demotion should still be present.
    ASSERT_EQ(raft.GetLogSize(), 3u);
    for (int i = 1; i <= 2; ++i) {
      const auto& le = raft.GetLog()[i];
      EXPECT_EQ(le.entry.term(), 3);
      ClientTestRequest req;
      req.ParseFromString(le.entry.command());
      EXPECT_EQ(req.value(), "cmd-" + std::to_string(i));
    }
  }
}

// Test 4: A truncation that occurs after a checkpoint is replayed correctly.
TEST_F(RaftRecoveryIntegrationTest, TruncationPersistsAfterCheckpoint) {
  // Timeline:
  //   - Write entries 1–5 (all term 3).
  //   - Checkpoint fires at stable index 2 → WAL is compacted up to index 2.
  //   - Truncate from index 4 onward.
  //   - Write new entries at index 4–5 with term 6 and different commands.
  {
    std::promise<bool> insert_done, ckpt_fired;
    auto insert_done_future = insert_done.get_future();
    auto ckpt_fired_future = ckpt_fired.get_future();

    int call_count = 0;
    EXPECT_CALL(checkpoint_, GetStableCheckpoint())
        .WillRepeatedly(Invoke([&]() -> uint64_t {
          ++call_count;
          if (call_count == 1)
            insert_done_future.get();  // block until initial entries are in
          else if (call_count == 2)
            ckpt_fired.set_value(true);  // signal that the checkpoint fired
          return 2;                      // checkpoint covers indices 1–2
        }));

    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);

    // Seed metadata.
    recovery.WriteMetadata(/*current_term=*/3, /*voted_for=*/1,
                           /*snapshot_last_index=*/0,
                           /*snapshot_last_term=*/0);

    // Write entries 1–5 at term 3.
    for (int i = 1; i <= 5; ++i) {
      Entry e;
      e.set_term(3);
      ClientTestRequest req;
      req.set_value("original-" + std::to_string(i));
      req.SerializeToString(e.mutable_command());
      recovery.AddLogEntry(&e, i);
    }

    // Unblock the checkpoint poll and wait for it to fire.
    insert_done.set_value(true);
    ckpt_fired_future.get();

    recovery.WriteMetadata(/*current_term=*/3, /*voted_for=*/1,
                           /*snapshot_last_index=*/2,
                           /*snapshot_last_term=*/3);

    // Truncate from index 4 onward (entries 4 and 5 are discarded).
    // NOTE: Assumes RaftRecovery::TruncateLog(from_index) writes a
    // kTruncation WAL record.  Adjust the call if the API differs.
    TruncationRecord truncation;
    truncation.set_truncate_from_index(4);
    truncation.set_truncate_from_term(3);
    recovery.TruncateLog(truncation);

    // Rewrite indices 4–5 under term 6 with different commands.
    for (int i = 4; i <= 5; ++i) {
      Entry e;
      e.set_term(6);
      ClientTestRequest req;
      req.set_value("rewritten-" + std::to_string(i));
      req.SerializeToString(e.mutable_command());
      recovery.AddLogEntry(&e, i);
    }
  }

  {
    MockSignatureVerifier verifier;
    ResDBConfig config = MakeConfig();
    MockLeaderElectionManager lem(config);
    MockReplicaCommunicator comm;

    RaftRecovery recovery(config_, nullptr, nullptr, nullptr);

    Raft raft(/*id=*/1, /*f=*/1, /*total=*/4, &verifier, &lem, &comm,
              &recovery);

    // Recover and verify: indices 1–3 are untouched, 4–5 carry the new data.
    RecoverFromLogs(recovery, raft);

    EXPECT_EQ(raft.GetSnapshotLastIndex(), 2u);

    // Sentinel (index 0) + entries 1–5 after truncation/rewrite = 6 total.
    // The WAL after compaction starts from the checkpoint (index 2 sentinel),
    // then replays entries 3, 4 (rewritten), 5 (rewritten).
    EXPECT_EQ(raft.GetLogSize(), 4u);

    // Entry at absolute index 3 should be original.
    {
      auto le = raft.GetLogEntryAtIndex(3);
      EXPECT_EQ(le.entry.term(), 3);
      ClientTestRequest req;
      req.ParseFromString(le.entry.command());
      EXPECT_EQ(req.value(), "original-3");
    }

    // Entries at absolute indices 4–5 must reflect the post-truncation rewrite.
    for (int i = 4; i <= 5; ++i) {
      auto le = raft.GetLogEntryAtIndex(i);
      EXPECT_EQ(le.entry.term(), 6);
      ClientTestRequest req;
      req.ParseFromString(le.entry.command());
      EXPECT_EQ(req.value(), "rewritten-" + std::to_string(i));
    }

    EXPECT_EQ(raft.GetLogTermAtIndex(4), 6);
    EXPECT_EQ(raft.GetLogTermAtIndex(5), 6);
  }
}

}  // namespace raft
}  // namespace resdb
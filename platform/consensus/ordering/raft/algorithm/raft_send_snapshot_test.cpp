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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "chain/storage/memory_db.h"
#include "platform/consensus/checkpoint/mock_checkpoint.h"
#include "platform/consensus/ordering/raft/algorithm/raft_test_util.h"
#include "platform/consensus/recovery/raft_recovery.h"

namespace resdb {
namespace raft {

using resdb::raft::test_utils::CreateAeFields;
using resdb::raft::test_utils::CreateAeMessage;
using resdb::raft::test_utils::CreateLogEntries;
using resdb::raft::test_utils::CreateProgressPatch;
using resdb::raft::test_utils::GenerateConfig;
using resdb::raft::test_utils::MockBroadcastFunction;
using resdb::raft::test_utils::MockCommitFunction;
using resdb::raft::test_utils::MockSendMessageFunction;
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Invoke;

namespace {

InstallSnapshot MakeChunk(uint64_t term, int leader_id, uint64_t last_index,
                          uint64_t last_term, uint64_t offset,
                          const std::string& data, bool done) {
  InstallSnapshot msg;
  msg.set_term(term);
  msg.set_leader_id(leader_id);
  msg.set_last_included_index(last_index);
  msg.set_last_included_term(last_term);
  msg.set_offset(offset);
  msg.set_data(data);
  msg.set_done(done);
  return msg;
}

// Read all bytes from a file; returns empty string if the file doesn't exist.
std::string ReadFileContents(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  if (!f) return "";
  return {std::istreambuf_iterator<char>(f), {}};
}

// Check whether a path exists on disk.
bool FileExists(const std::string& path) {
  return std::filesystem::exists(path);
}

// Build a ResDBConfig with recovery enabled and a given WAL path.
ResDBConfig MakeConfig(const std::string& wal_path, int self_id = 1) {
  ResConfigData data;
  data.set_recovery_enabled(true);
  data.set_recovery_path(wal_path);
  data.set_recovery_buffer_size(1024);
  data.set_recovery_ckpt_time_s(3600);  // effectively disable background ckpt
  return ResDBConfig(
      {GenerateReplicaInfo(1, "127.0.0.1", 1234),
       GenerateReplicaInfo(2, "127.0.0.1", 1235),
       GenerateReplicaInfo(3, "127.0.0.1", 1236),
       GenerateReplicaInfo(4, "127.0.0.1", 1237)},
      GenerateReplicaInfo(self_id, "127.0.0.1", 1234 + self_id - 1), data);
}

// Drive the full leader to follower snapshot transfer loop in-process.
// Stops when the last response seen by the leader has transfer_complete=true,
// or after 1,000 iterations (which would indicate a bug).
void RunTransfer(Raft& leader, Raft& follower, int follower_id, int leader_id,
                 MockSendMessageFunction& leader_send_message,
                 MockSendMessageFunction& follower_send_message) {
  std::vector<InstallSnapshot> to_follower;
  std::vector<InstallSnapshotResponse> to_leader;

  // To simulate sending messages, just push to them to a vector.
  EXPECT_CALL(leader_send_message, Call(_, _, _))
      .WillRepeatedly(::testing::Return(0));
  EXPECT_CALL(leader_send_message,
              Call(MessageType::InstallSnapshotMsg, _, follower_id))
      .WillRepeatedly(
          Invoke([&](int, const google::protobuf::Message& msg, int) {
            LOG(INFO) << "Test sending install snapshot message";
            to_follower.push_back(dynamic_cast<const InstallSnapshot&>(msg));
            return 0;
          }));

  EXPECT_CALL(follower_send_message, Call(_, _, _))
      .WillRepeatedly(::testing::Return(0));
  EXPECT_CALL(follower_send_message,
              Call(MessageType::InstallSnapshotResponseMsg, _, leader_id))
      .WillRepeatedly(Invoke([&](int, const google::protobuf::Message& msg,
                                 int) {
        LOG(INFO) << "Test sending install snapshot response message";
        to_leader.push_back(dynamic_cast<const InstallSnapshotResponse&>(msg));
        return 0;
      }));

  // Send the first snapshot chunk
  EXPECT_FALSE(leader.IsSendSnapshotInProgress());
  const auto& follower_progress = leader.GetFollowerProgress();
  for (const auto& follower : follower_progress) {
    EXPECT_NE(follower.state, ProgressState::SNAPSHOT);
  }
  leader.SendInstallSnapshot(follower_id, /*byte_offset=*/0);
  EXPECT_EQ(to_follower.size(), 1);
  EXPECT_TRUE(leader.IsSendSnapshotInProgress());

  EXPECT_EQ(leader.GetFollowerProgress()[follower_id].state,
            ProgressState::SNAPSHOT);

  size_t fi = 0, li = 0;
  for (int iter = 0; iter < 1000; ++iter) {
    while (fi < to_follower.size()) {
      LOG(INFO) << "Follower is going to install snapshot";
      follower.ReceiveInstallSnapshot(
          std::make_unique<InstallSnapshot>(to_follower[fi++]));
      EXPECT_TRUE(leader.IsSendSnapshotInProgress());
    }
    while (li < to_leader.size()) {
      LOG(INFO) << "Leader is going to receive snapshot response";
      EXPECT_EQ(leader.GetFollowerProgress()[follower_id].state,
                ProgressState::SNAPSHOT);
      leader.ReceiveInstallSnapshotResponse(
          std::make_unique<InstallSnapshotResponse>(to_leader[li++]));
    }
    // Once the follower has responded to the leader with transfer_complete, the
    // snapshot has completed.
    if (!to_leader.empty() && to_leader.back().transfer_complete()) {
      EXPECT_NE(leader.GetFollowerProgress()[follower_id].state,
                ProgressState::SNAPSHOT);
      EXPECT_FALSE(leader.IsSendSnapshotInProgress());
      return;
    }
  }
  assert(false && "Snapshot sending went on for more than 1000 iterations");
}

// Mirrors what Consensus::RecoverFromLogs() does.
void RecoverFromLogs(RaftRecovery& recovery, Raft& raft) {
  recovery.ReadLogs(
      [](const RaftMetadata&) {},
      [&](std::unique_ptr<WALRecord> record) {
        switch (record->payload_case()) {
          case WALRecord::kEntry: {
            LogEntry le;
            le.entry = record->entry();
            raft.AddToLog(le, /*write_metadata=*/false);
            break;
          }
          case WALRecord::kTruncation:
            raft.TruncateLog(record->truncation().truncate_from_index(),
                             /*write_metadata=*/false);
            break;
          case WALRecord::PAYLOAD_NOT_SET:
            break;
        }
      },
      [&](const RaftMetadata& m) {
        raft.SetCurrentTerm(m.current_term, /*write_metadata=*/false);
        raft.SetVotedFor(m.voted_for, /*write_metadata=*/false);
        raft.SetSnapshotLastIndexAndTerm(m.snapshot_last_index,
                                         m.snapshot_last_term,
                                         /*write_metadata=*/false);
      });
}

}  // namespace

class RaftSnapshotFileTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Each test gets its own log directory so they don't interfere.
    leader_log_ =
        "./log/snap_file_test_leader_" +
        std::to_string(
            ::testing::UnitTest::GetInstance()->current_test_info()->line()) +
        "/log";
    follower_log_ =
        "./log/snap_file_test_follower_" +
        std::to_string(
            ::testing::UnitTest::GetInstance()->current_test_info()->line()) +
        "/log";
    std::filesystem::remove_all(
        std::filesystem::path(leader_log_).parent_path());
    std::filesystem::remove_all(
        std::filesystem::path(follower_log_).parent_path());

    // Pre-create the directories so the single-level mkdir in RecoveryBase
    // succeeds.
    std::filesystem::create_directories(
        std::filesystem::path(leader_log_).parent_path());
    std::filesystem::create_directories(
        std::filesystem::path(follower_log_).parent_path());
  }

  std::string leader_log_;
  std::string follower_log_;
};

// Test 1 : After a completed single-chunk transfer the committed snapshot file
// exists on the follower's disk, and the temp file does not exist.
TEST_F(RaftSnapshotFileTest, SnapshotFileWrittenToFollowerDisk) {
  auto leader_storage = resdb::storage::NewMemoryDB();
  leader_storage->SetValueWithVersion("k1", "v1", 0);
  leader_storage->Flush();

  ResDBConfig leader_cfg = MakeConfig(leader_log_, /*self_id=*/1);
  RaftRecovery leader_recovery(leader_cfg, nullptr, leader_storage.get(),
                               nullptr);
  MockSignatureVerifier leader_verifier;
  MockLeaderElectionManager leader_leader_election_manager(leader_cfg);
  MockReplicaCommunicator leader_replica_communicator;
  MockSendMessageFunction leader_send_message;

  Raft leader(1, 1, 4, &leader_verifier, &leader_leader_election_manager,
              &leader_replica_communicator, &leader_recovery, leader_cfg);
  leader.SetStateForTest(
      {.current_term = 3,
       .role = Role::LEADER,
       .log = CreateLogEntries({}, true),
       CreateProgressPatch({
           .next_index = std::vector<uint64_t>{1, 1, 1, 1, 1},
           .match_index = std::vector<uint64_t>{0, 0, 0, 0, 0},
       })});
  leader.SetSnapshotLastIndexAndTerm(2, 3, /*write_metadata=*/false);
  leader.SetSingleCallFunc(
      [&](int type, const google::protobuf::Message& msg, int node_id) {
        return leader_send_message.Call(type, msg, node_id);
      });

  ResDBConfig follower_cfg = MakeConfig(follower_log_, /*self_id=*/2);
  auto follower_storage = resdb::storage::NewMemoryDB();
  RaftRecovery follower_recovery(follower_cfg, nullptr, follower_storage.get(),
                                 nullptr);
  MockSignatureVerifier follower_verifier;
  MockLeaderElectionManager follower_leader_election_manager(follower_cfg);
  MockReplicaCommunicator follower_replica_communicator;
  MockSendMessageFunction follower_send_message;
  MockCommitFunction follower_commit;

  Raft follower(2, 1, 4, &follower_verifier, &follower_leader_election_manager,
                &follower_replica_communicator, &follower_recovery,
                follower_cfg);
  follower.SetStateForTest({.current_term = 3, .role = Role::FOLLOWER});
  follower.SetSingleCallFunc(
      [&](int type, const google::protobuf::Message& msg, int node_id) {
        return follower_send_message.Call(type, msg, node_id);
      });
  follower.SetCommitFunc([&](const google::protobuf::Message& msg) {
    return follower_commit.Commit(msg);
  });

  EXPECT_CALL(follower_leader_election_manager, OnHeartbeat())
      .Times(AnyNumber());
  EXPECT_CALL(leader_leader_election_manager, OnHeartbeat()).Times(AnyNumber());
  EXPECT_CALL(leader_leader_election_manager, OnRoleChange())
      .Times(AnyNumber());
  EXPECT_CALL(follower_leader_election_manager, OnRoleChange())
      .Times(AnyNumber());
  ASSERT_EQ(leader_recovery.GetStorage(), leader_storage.get())
      << "RaftRecovery::GetStorage() must return the same Storage passed in";
  auto items = leader_storage->GetAllItems();
  ASSERT_FALSE(items.empty()) << "leader_storage has no items before transfer";

  RunTransfer(leader, follower, 2, 1, leader_send_message,
              follower_send_message);

  // The committed snapshot file must exist on the follower's disk.
  const std::string snap_path = follower.GetSnapshotFilePath();
  EXPECT_TRUE(FileExists(snap_path))
      << "Expected snapshot file at " << snap_path;

  // The temp file must have been cleaned up (renamed away).
  auto temp_path = follower.GetSnapshotFilePath() + ".recv.tmp";
  EXPECT_FALSE(FileExists(temp_path))
      << "Temp file should not exist after completed transfer: " << temp_path;

  // Data must round-trip correctly.
  EXPECT_EQ(follower_storage->GetValue("k1"), "v1");
}

// Test 2: A follower that has not received the final chunk starts fresh on the
// next transfer.
TEST_F(RaftSnapshotFileTest, PartialTransferDoesNotCorruptSubsequentTransfer) {
  auto leader_storage = resdb::storage::NewMemoryDB();
  leader_storage->SetValueWithVersion("real_key", "real_value", 0);

  ResDBConfig leader_cfg = MakeConfig(leader_log_, 1);
  RaftRecovery leader_recovery(leader_cfg, nullptr, leader_storage.get(),
                               nullptr);
  MockSignatureVerifier leader_verifier;
  MockLeaderElectionManager leader_leader_election_manager(leader_cfg);
  MockReplicaCommunicator leader_replica_communicator;

  Raft leader(1, 1, 4, &leader_verifier, &leader_leader_election_manager,
              &leader_replica_communicator, &leader_recovery, leader_cfg);
  leader.SetStateForTest(
      {.current_term = 4,
       .role = Role::LEADER,
       .log = CreateLogEntries({}, true),
       CreateProgressPatch({
           .next_index = std::vector<uint64_t>{1, 1, 1, 1, 1},
           .match_index = std::vector<uint64_t>{0, 0, 0, 0, 0},
       })});
  leader.SetSnapshotLastIndexAndTerm(5, 4, /*write_metadata=*/false);

  ResDBConfig follower_cfg = MakeConfig(follower_log_, 2);
  auto follower_storage = resdb::storage::NewMemoryDB();
  RaftRecovery follower_recovery(follower_cfg, nullptr, follower_storage.get(),
                                 nullptr);
  MockSignatureVerifier follower_verifier;
  MockLeaderElectionManager follower_leader_election_manager(follower_cfg);
  MockReplicaCommunicator follower_replica_communicator;
  MockSendMessageFunction follower_send_message;
  MockCommitFunction follower_commit;

  Raft follower(2, 1, 4, &follower_verifier, &follower_leader_election_manager,
                &follower_replica_communicator, &follower_recovery,
                follower_cfg);
  follower.SetStateForTest({.current_term = 4, .role = Role::FOLLOWER});
  follower.SetSingleCallFunc(
      [&](int type, const google::protobuf::Message& msg, int node_id) {
        return follower_send_message.Call(type, msg, node_id);
      });
  follower.SetCommitFunc([&](const google::protobuf::Message& msg) {
    return follower_commit.Commit(msg);
  });

  EXPECT_CALL(follower_leader_election_manager, OnHeartbeat())
      .Times(AnyNumber());
  EXPECT_CALL(leader_leader_election_manager, OnHeartbeat()).Times(AnyNumber());
  EXPECT_CALL(leader_leader_election_manager, OnRoleChange())
      .Times(AnyNumber());
  EXPECT_CALL(follower_leader_election_manager, OnRoleChange())
      .Times(AnyNumber());

  // Simulate a partial transfer: send only the first chunk (done == false) and
  // do not deliver any follow-up. The follower will have an open temp file
  // in pending state.
  {
    std::vector<InstallSnapshotResponse> responses;
    EXPECT_CALL(follower_send_message, Call(_, _, _))
        .WillRepeatedly(::testing::Return(0));
    EXPECT_CALL(follower_send_message,
                Call(MessageType::InstallSnapshotResponseMsg, _, _))
        .WillRepeatedly(
            Invoke([&](int, const google::protobuf::Message& msg, int) {
              responses.push_back(
                  dynamic_cast<const InstallSnapshotResponse&>(msg));
              return 0;
            }));

    // Inject a fake first chunk with done == false. Since the real leader
    // serializes to a file, we use a raw chunk to avoid needing the leader's
    // file path.
    auto partial = MakeChunk(4, /*leader_id=*/3, /*last_index=*/5,
                             /*last_term=*/4, /*offset=*/0,
                             std::string(256, 'P'), /*done=*/false);
    follower.ReceiveInstallSnapshot(
        std::make_unique<InstallSnapshot>(std::move(partial)));

    ASSERT_FALSE(responses.empty());
    EXPECT_TRUE(responses[0].need_snapshot());
    EXPECT_EQ(responses[0].bytes_stored(), 256u);
    auto temp_path = follower.GetSnapshotFilePath() + ".recv.tmp";
    EXPECT_TRUE(FileExists(temp_path)) << "Path being checked: " << temp_path;
  }

  // Now run a complete, fresh transfer from the beginning. The fresh offset ==
  // 0 chunk must discard the previous partial state and open a new temp file.
  MockSendMessageFunction leader_send_message2;
  leader.SetSingleCallFunc(
      [&](int type, const google::protobuf::Message& msg, int node_id) {
        return leader_send_message2.Call(type, msg, node_id);
      });

  RunTransfer(leader, follower, 2, 1, leader_send_message2,
              follower_send_message);

  // After completion the committed file must exist.
  EXPECT_TRUE(FileExists(follower.GetSnapshotFilePath()));
  // Temp file must be gone.
  auto temp_path = follower.GetSnapshotFilePath() + ".recv.tmp";
  EXPECT_FALSE(FileExists(temp_path));
  EXPECT_EQ(follower_storage->GetValue("real_key"), "real_value");
}

// Test 3: When the leader's snapshot_last_index advances between two transfers,
// the leader writes a new snapshot file at offset == 0 that reflects the
// updated state, not the stale one.
TEST_F(RaftSnapshotFileTest, LeaderReserializesWhenSnapshotIndexAdvances) {
  auto leader_storage = resdb::storage::NewMemoryDB();
  leader_storage->SetValueWithVersion("v1_key", "v1_val", 0);

  ResDBConfig leader_cfg = MakeConfig(leader_log_, 1);
  RaftRecovery leader_recovery(leader_cfg, nullptr, leader_storage.get(),
                               nullptr);
  MockSignatureVerifier leader_verifier;
  MockLeaderElectionManager leader_leader_election_manager(leader_cfg);
  MockReplicaCommunicator leader_replica_communicator;
  MockSendMessageFunction leader_send_message;

  Raft leader(1, 1, 4, &leader_verifier, &leader_leader_election_manager,
              &leader_replica_communicator, &leader_recovery, leader_cfg);
  leader.SetStateForTest(
      {.current_term = 2,
       .role = Role::LEADER,
       .log = CreateLogEntries({}, true),
       CreateProgressPatch({
           .next_index = std::vector<uint64_t>{1, 1, 1, 1, 1},
           .match_index = std::vector<uint64_t>{0, 0, 0, 0, 0},
       })});
  leader.SetSnapshotLastIndexAndTerm(3, 2, /*write_metadata=*/false);
  leader.SetSingleCallFunc(
      [&](int type, const google::protobuf::Message& msg, int node_id) {
        return leader_send_message.Call(type, msg, node_id);
      });
  // Transfer the first chunk of snapshot to follower A
  const std::string follower_a_log = follower_log_ + "_a";
  std::filesystem::remove_all(
      std::filesystem::path(follower_a_log).parent_path());
  std::filesystem::create_directories(
      std::filesystem::path(follower_a_log).parent_path());
  ResDBConfig follower_a_cfg = MakeConfig(follower_a_log, 2);
  auto follower_a_storage = resdb::storage::NewMemoryDB();
  RaftRecovery follower_a_recovery(follower_a_cfg, nullptr,
                                   follower_a_storage.get(), nullptr);
  MockSignatureVerifier follower_a_verifier;
  MockLeaderElectionManager follower_a_leader_election_manager(follower_a_cfg);
  MockReplicaCommunicator follower_a_replica_communicator;
  MockSendMessageFunction follower_a_send_message;
  MockCommitFunction follower_a_commit;

  Raft follower_a(
      2, 1, 4, &follower_a_verifier, &follower_a_leader_election_manager,
      &follower_a_replica_communicator, &follower_a_recovery, follower_a_cfg);
  follower_a.SetStateForTest({.current_term = 2, .role = Role::FOLLOWER});
  follower_a.SetSingleCallFunc(
      [&](int type, const google::protobuf::Message& msg, int node_id) {
        return follower_a_send_message.Call(type, msg, node_id);
      });
  follower_a.SetCommitFunc([&](const google::protobuf::Message& msg) {
    return follower_a_commit.Commit(msg);
  });

  EXPECT_CALL(follower_a_leader_election_manager, OnHeartbeat())
      .Times(AnyNumber());
  EXPECT_CALL(leader_leader_election_manager, OnHeartbeat()).Times(AnyNumber());
  EXPECT_CALL(leader_leader_election_manager, OnRoleChange())
      .Times(AnyNumber());
  EXPECT_CALL(follower_a_leader_election_manager, OnRoleChange())
      .Times(AnyNumber());

  RunTransfer(leader, follower_a, 2, 1, leader_send_message,
              follower_a_send_message);
  EXPECT_EQ(follower_a_storage->GetValue("v1_key"), "v1_val");

  // Capture the file contents written during the first transfer.
  const std::string leader_snap_path = leader.GetSnapshotFilePath();
  std::string first_snapshot_bytes = ReadFileContents(leader_snap_path);
  ASSERT_FALSE(first_snapshot_bytes.empty());

  // Advance the leader's snapshot point and update storage.
  leader_storage->SetValueWithVersion("v2_key", "v2_val", 0);
  leader.SetSnapshotLastIndexAndTerm(7, 2, /*write_metadata=*/false);

  // After a new snapshot has been taken, send the first snapshot chunk to
  // follower B.
  const std::string follower_b_log = follower_log_ + "_b";
  std::filesystem::remove_all(
      std::filesystem::path(follower_b_log).parent_path());
  std::filesystem::create_directories(
      std::filesystem::path(follower_b_log).parent_path());
  ResDBConfig follower_b_cfg = MakeConfig(follower_b_log, 3);
  auto follower_b_storage = resdb::storage::NewMemoryDB();
  RaftRecovery follower_b_recovery(follower_b_cfg, nullptr,
                                   follower_b_storage.get(), nullptr);
  MockSignatureVerifier follower_b_verifier;
  MockLeaderElectionManager follower_b_leader_election_manager(follower_b_cfg);
  MockReplicaCommunicator follower_b_replica_communicator;
  MockSendMessageFunction follower_b_send_message;
  MockCommitFunction follower_b_commit;

  Raft follower_b(
      3, 1, 4, &follower_b_verifier, &follower_b_leader_election_manager,
      &follower_b_replica_communicator, &follower_b_recovery, follower_b_cfg);
  follower_b.SetStateForTest({.current_term = 2, .role = Role::FOLLOWER});
  follower_b.SetSingleCallFunc(
      [&](int type, const google::protobuf::Message& msg, int node_id) {
        return follower_b_send_message.Call(type, msg, node_id);
      });
  follower_b.SetCommitFunc([&](const google::protobuf::Message& msg) {
    return follower_b_commit.Commit(msg);
  });

  EXPECT_CALL(follower_b_leader_election_manager, OnHeartbeat())
      .Times(AnyNumber());
  EXPECT_CALL(follower_b_leader_election_manager, OnRoleChange())
      .Times(AnyNumber());

  MockSendMessageFunction leader_send_message2;
  leader.SetSingleCallFunc(
      [&](int type, const google::protobuf::Message& msg, int node_id) {
        return leader_send_message2.Call(type, msg, node_id);
      });

  RunTransfer(leader, follower_b, 3, 1, leader_send_message2,
              follower_b_send_message);

  // The snapshot file on disk must have been updated (different bytes).
  std::string second_snapshot_bytes = ReadFileContents(leader_snap_path);
  EXPECT_NE(first_snapshot_bytes, second_snapshot_bytes)
      << "Leader should have written a new snapshot file after advancing its "
         "snapshot_last_index";

  // follower_b must have both keys; follower_a must only have v1_key.
  EXPECT_EQ(follower_b_storage->GetValue("v1_key"), "v1_val");
  EXPECT_EQ(follower_b_storage->GetValue("v2_key"), "v2_val");
  EXPECT_EQ(follower_a_storage->GetValue("v2_key"), "");
}

// Test 4: A follower installs a snapshot, then restarts, reading the metadata
// and snapshot.
TEST_F(RaftSnapshotFileTest, FollowerMetadataSurvivesRestartAfterSnapshot) {
  auto leader_storage = resdb::storage::NewMemoryDB();
  leader_storage->SetValueWithVersion("persist_key", "persist_value", 0);
  leader_storage->SetValueWithVersion("another_key", "another_value", 0);

  ResDBConfig leader_cfg = MakeConfig(leader_log_, 1);
  RaftRecovery leader_recovery(leader_cfg, nullptr, leader_storage.get(),
                               nullptr);
  MockSignatureVerifier leader_verifier;
  MockLeaderElectionManager leader_leader_election_manager(leader_cfg);
  MockReplicaCommunicator leader_replica_communicator;
  MockSendMessageFunction leader_send_message;

  Raft leader(1, 1, 4, &leader_verifier, &leader_leader_election_manager,
              &leader_replica_communicator, &leader_recovery, leader_cfg);
  leader.SetStateForTest(
      {.current_term = 6,
       .role = Role::LEADER,
       .log = CreateLogEntries({}, true),
       .progress = CreateProgressPatch({
           .next_index = std::vector<uint64_t>{1, 1, 1, 1, 1},
           .match_index = std::vector<uint64_t>{0, 0, 0, 0, 0},
       })});
  leader.SetSnapshotLastIndexAndTerm(10, 6, /*write_metadata=*/false);
  leader.SetSingleCallFunc(
      [&](int type, const google::protobuf::Message& msg, int node_id) {
        return leader_send_message.Call(type, msg, node_id);
      });

  ResDBConfig follower_cfg = MakeConfig(follower_log_, 2);
  // Use a real MemoryDB; it is the same object across both lifetimes so we
  // can observe that Clear() + SetValue() happened (MemoryDB keeps state in
  // memory, but WriteMetadata() persists snapshot_last_index to disk).
  auto follower_storage = resdb::storage::NewMemoryDB();
  follower_storage->SetValueWithVersion("stale_data", "will_be_cleared", 0);

  {
    RaftRecovery follower_recovery(follower_cfg, nullptr,
                                   follower_storage.get(), nullptr);
    MockSignatureVerifier follower_verifier;
    MockLeaderElectionManager follower_leader_election_manager(follower_cfg);
    MockReplicaCommunicator follower_replica_communicator;
    MockSendMessageFunction follower_send_message;
    MockCommitFunction follower_commit;

    Raft follower(
        2, 1, 4, &follower_verifier, &follower_leader_election_manager,
        &follower_replica_communicator, &follower_recovery, follower_cfg);
    follower.SetStateForTest({.current_term = 6, .role = Role::FOLLOWER});
    follower.SetSingleCallFunc(
        [&](int type, const google::protobuf::Message& msg, int node_id) {
          return follower_send_message.Call(type, msg, node_id);
        });
    follower.SetCommitFunc([&](const google::protobuf::Message& msg) {
      return follower_commit.Commit(msg);
    });

    EXPECT_CALL(follower_leader_election_manager, OnHeartbeat())
        .Times(AnyNumber());
    EXPECT_CALL(leader_leader_election_manager, OnHeartbeat())
        .Times(AnyNumber());
    EXPECT_CALL(leader_leader_election_manager, OnRoleChange())
        .Times(AnyNumber());
    EXPECT_CALL(follower_leader_election_manager, OnRoleChange())
        .Times(AnyNumber());

    RunTransfer(leader, follower, 2, 1, leader_send_message,
                follower_send_message);

    // Verify storage is correct while follower is still alive.
    EXPECT_EQ(follower_storage->GetValue("persist_key"), "persist_value");
    EXPECT_EQ(follower_storage->GetValue("stale_data"), "");
    EXPECT_EQ(follower.GetSnapshotLastIndex(), 10u);
  }

  {
    RaftRecovery recovery2(follower_cfg, nullptr, follower_storage.get(),
                           nullptr);
    MockSignatureVerifier fv2;
    MockLeaderElectionManager fl2(follower_cfg);
    MockReplicaCommunicator fc2;

    Raft follower2(2, 1, 4, &fv2, &fl2, &fc2, &recovery2, follower_cfg);
    RecoverFromLogs(recovery2, follower2);

    EXPECT_EQ(follower2.GetSnapshotLastIndex(), 10u)
        << "snapshot_last_index must survive process restart";
    EXPECT_EQ(follower2.GetCurrentTerm(), 6u)
        << "current_term must survive process restart";

    // The storage data is still in-memory (same object), but this verifies the
    // stale key is gone.
    EXPECT_EQ(follower_storage->GetValue("persist_key"), "persist_value");
    EXPECT_EQ(follower_storage->GetValue("another_key"), "another_value");
    EXPECT_EQ(follower_storage->GetValue("stale_data"), "");
  }
}

// Test 5: A follower receives the first chunk of a different snapshot while a
// prior temp file is open correctly discards the old temp file and starts
// fresh.
TEST_F(RaftSnapshotFileTest, FollowerDiscardsOldTempFileOnNewOffsetZeroChunk) {
  // Use real RaftRecovery so snapshot file paths are real.
  ResDBConfig follower_cfg = MakeConfig(follower_log_, 2);
  auto follower_storage = resdb::storage::NewMemoryDB();
  RaftRecovery follower_recovery(follower_cfg, nullptr, follower_storage.get(),
                                 nullptr);
  MockSignatureVerifier follower_verifier;
  MockLeaderElectionManager follower_leader_election_manager(follower_cfg);
  MockReplicaCommunicator follower_replica_communicator;
  MockSendMessageFunction follower_send_message;
  MockCommitFunction follower_commit;

  Raft follower(2, 1, 4, &follower_verifier, &follower_leader_election_manager,
                &follower_replica_communicator, &follower_recovery,
                follower_cfg);
  follower.SetStateForTest({.current_term = 5, .role = Role::FOLLOWER});
  follower.SetSingleCallFunc(
      [&](int type, const google::protobuf::Message& msg, int node_id) {
        return follower_send_message.Call(type, msg, node_id);
      });
  follower.SetCommitFunc([&](const google::protobuf::Message& msg) {
    return follower_commit.Commit(msg);
  });

  EXPECT_CALL(follower_leader_election_manager, OnHeartbeat())
      .Times(AnyNumber());
  EXPECT_CALL(follower_leader_election_manager, OnRoleChange())
      .Times(AnyNumber());

  std::vector<InstallSnapshotResponse> responses;
  EXPECT_CALL(follower_send_message, Call(_, _, _))
      .WillRepeatedly(::testing::Return(0));
  EXPECT_CALL(follower_send_message,
              Call(MessageType::InstallSnapshotResponseMsg, _, _))
      .WillRepeatedly(Invoke([&](int, const google::protobuf::Message& msg,
                                 int) {
        responses.push_back(dynamic_cast<const InstallSnapshotResponse&>(msg));
        return 0;
      }));

  // First attempt: send offset == 0 chunk only (partial).
  const std::string first_data(200, 'A');
  auto c1 = MakeChunk(5, 1, 15, 5, 0, first_data, /*done=*/false);
  follower.ReceiveInstallSnapshot(
      std::make_unique<InstallSnapshot>(std::move(c1)));
  ASSERT_EQ(responses.size(), 1u);
  EXPECT_EQ(responses[0].bytes_stored(), 200u);

  // Simulate leader restarting the transfer. The follower must accept it and
  // discard the old partial snapshot.
  const std::string second_data(100, 'B');
  auto c2 = MakeChunk(5, 1, 15, 5, 0, second_data, /*done=*/true);
  follower.ReceiveInstallSnapshot(
      std::make_unique<InstallSnapshot>(std::move(c2)));

  ASSERT_EQ(responses.size(), 2u);
  EXPECT_TRUE(responses[1].transfer_complete());
  EXPECT_FALSE(responses[1].need_snapshot());

  // Snapshot index must have advanced.
  EXPECT_EQ(follower.GetSnapshotLastIndex(), 15u);

  // The committed snapshot file on disk should contain exactly 'B'*100.
  std::string on_disk = ReadFileContents(follower.GetSnapshotFilePath());
  EXPECT_EQ(on_disk.size(), 100u);
  EXPECT_TRUE(on_disk.find_first_not_of('B') == std::string::npos)
      << "Committed snapshot file should contain only the second attempt's "
         "data";
}

// Test 6: A follower rejects a snapshot covering only entries that it has
// already committed.
TEST_F(RaftSnapshotFileTest, StaleSnapshotRejected) {
  ResDBConfig cfg = MakeConfig(follower_log_, /*self_id=*/2);
  auto storage = resdb::storage::NewMemoryDB();
  RaftRecovery recovery(cfg, nullptr, storage.get(), nullptr);

  MockSignatureVerifier verifier;
  MockLeaderElectionManager leader_election_manager(cfg);
  MockReplicaCommunicator replica_communicator;
  MockSendMessageFunction send_message;
  MockCommitFunction commit;

  Raft follower(2, 1, 4, &verifier, &leader_election_manager,
                &replica_communicator, &recovery, cfg);
  follower.SetSingleCallFunc(
      [&](int type, const google::protobuf::Message& msg, int node_id) {
        return send_message.Call(type, msg, node_id);
      });
  follower.SetCommitFunc(
      [&](const google::protobuf::Message& msg) { return commit.Commit(msg); });

  // Follower already has committed up to index 20.
  follower.SetStateForTest({
      .current_term = 5,
      .commit_index = 20,
      .last_committed = 20,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries({}, /*used_for_log_patch=*/true),
  });
  follower.SetSnapshotLastIndexAndTerm(20, 5, /*write_metadata=*/false);

  EXPECT_CALL(leader_election_manager, OnHeartbeat()).Times(AnyNumber());

  // Capture the response sent back to the leader.
  InstallSnapshotResponse captured;
  EXPECT_CALL(send_message, Call(MessageType::InstallSnapshotResponseMsg, _, _))
      .WillOnce(Invoke([&](int, const google::protobuf::Message& msg, int) {
        captured = dynamic_cast<const InstallSnapshotResponse&>(msg);
        return 0;
      }));

  // Stale snapshot for index == 15, older than the follower's commit_index.
  auto stale = MakeChunk(5, 1, 15, 4, 0, std::string(128, 'S'), /*done=*/true);
  follower.ReceiveInstallSnapshot(std::make_unique<InstallSnapshot>(stale));

  EXPECT_EQ(follower.GetSnapshotLastIndex(), 20u)
      << "Follower snapshot_last_index_ must not regress on a stale snapshot";
  EXPECT_EQ(follower.GetCommitIndex(), 20u) << "commit_index must not regress";
  EXPECT_FALSE(captured.need_snapshot());
  EXPECT_FALSE(captured.transfer_complete());
}

// Test 7: Snapshot completes but we already contain all entries it covers.
TEST_F(RaftSnapshotFileTest, LogAlreadyContainsSnapshotEntries) {
  ResDBConfig cfg = MakeConfig(follower_log_, /*self_id=*/2);
  auto storage = resdb::storage::NewMemoryDB();
  RaftRecovery recovery(cfg, nullptr, storage.get(), nullptr);

  MockSignatureVerifier verifier;
  MockLeaderElectionManager leader_election_manager(cfg);
  MockReplicaCommunicator replica_communicator;
  MockSendMessageFunction send_message;
  MockCommitFunction commit;

  Raft follower(2, 1, 4, &verifier, &leader_election_manager,
                &replica_communicator, &recovery, cfg);
  follower.SetSingleCallFunc(
      [&](int type, const google::protobuf::Message& msg, int node_id) {
        return send_message.Call(type, msg, node_id);
      });
  follower.SetCommitFunc(
      [&](const google::protobuf::Message& msg) { return commit.Commit(msg); });

  // Follower has snapshotted up to index 10.
  follower.SetSnapshotLastIndexAndTerm(10, 4, /*write_metadata=*/false);
  follower.SetStateForTest({
      .current_term = 4,
      .commit_index = 10,
      .last_committed = 10,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries({{4, "Transaction 11"},
                               {4, "Transaction 12"},
                               {4, "Transaction 13"},
                               {4, "Transaction 14"},
                               {4, "Transaction 15"}},
                              true),
  });

  EXPECT_CALL(leader_election_manager, OnHeartbeat()).Times(AnyNumber());

  // Capture the response sent back to the leader.
  InstallSnapshotResponse captured;
  EXPECT_CALL(send_message, Call(MessageType::InstallSnapshotResponseMsg, _, _))
      .WillOnce(Invoke([&](int, const google::protobuf::Message& msg, int) {
        captured = dynamic_cast<const InstallSnapshotResponse&>(msg);
        return 0;
      }));

  // Snapshot containing entries we already have.
  auto stale = MakeChunk(5, 1, 15, 4, 0, std::string(128, 'S'), /*done=*/true);
  follower.ReceiveInstallSnapshot(std::make_unique<InstallSnapshot>(stale));

  // Must not have changed state.
  EXPECT_EQ(follower.GetSnapshotLastIndex(), 10u)
      << "Follower snapshot_last_index_ must not change when snapshot is "
         "covered.";
  EXPECT_EQ(follower.GetCommitIndex(), 10u) << "commit_index must not regress";
  // The response must NOT signal transfer_complete=true for a state-machine
  // reset, and need_snapshot must be false.
  EXPECT_FALSE(captured.need_snapshot());
  EXPECT_FALSE(captured.transfer_complete());
}

// Test 8: Successfully transfer a snapshot that is multiple chunks.
TEST_F(RaftSnapshotFileTest, MultiChunkSnapshot) {
  ResDBConfig cfg = MakeConfig(follower_log_, /*self_id=*/2);
  auto storage = resdb::storage::NewMemoryDB();
  RaftRecovery recovery(cfg, nullptr, storage.get(), nullptr);

  MockSignatureVerifier verifier;
  MockLeaderElectionManager leader_election_manager(cfg);
  MockReplicaCommunicator replica_communicator;
  MockSendMessageFunction send_message;
  MockCommitFunction commit;

  Raft follower(2, 1, 4, &verifier, &leader_election_manager,
                &replica_communicator, &recovery, cfg);
  follower.SetSingleCallFunc(
      [&](int type, const google::protobuf::Message& msg, int node_id) {
        return send_message.Call(type, msg, node_id);
      });
  follower.SetCommitFunc(
      [&](const google::protobuf::Message& msg) { return commit.Commit(msg); });

  follower.SetStateForTest({
      .current_term = 4,
      .role = Role::FOLLOWER,
  });

  EXPECT_CALL(leader_election_manager, OnHeartbeat()).Times(AnyNumber());
  EXPECT_CALL(leader_election_manager, OnRoleChange()).Times(AnyNumber());

  // Capture the response sent back to the leader.
  std::vector<InstallSnapshotResponse> responses;
  EXPECT_CALL(send_message, Call(_, _, _)).WillRepeatedly(::testing::Return(0));
  EXPECT_CALL(send_message, Call(MessageType::InstallSnapshotResponseMsg, _, _))
      .WillRepeatedly(Invoke([&](int, const google::protobuf::Message& msg,
                                 int) {
        responses.push_back(dynamic_cast<const InstallSnapshotResponse&>(msg));
        return 0;
      }));

  // Build a 300-byte payload.
  const size_t chunk_size = 100;
  std::string full_payload;
  full_payload += std::string(chunk_size, 'A');  // chunk 0
  full_payload += std::string(chunk_size, 'B');  // chunk 1
  full_payload += std::string(chunk_size, 'C');  // chunk 2

  // Deliver chunks one at a time.
  for (size_t i = 0; i < 3; ++i) {
    bool done = (i == 2);
    std::string data = full_payload.substr(i * chunk_size, chunk_size);
    auto chunk = MakeChunk(4, 1, 30, 4, i * chunk_size, data, done);
    follower.ReceiveInstallSnapshot(std::make_unique<InstallSnapshot>(chunk));
  }

  ASSERT_EQ(responses.size(), 3u);
  EXPECT_TRUE(responses.back().transfer_complete());
  EXPECT_EQ(responses.back().bytes_stored(), full_payload.size());

  // The committed file must be byte-for-byte identical to full_payload.
  const std::string on_disk = ReadFileContents(follower.GetSnapshotFilePath());
  EXPECT_EQ(on_disk, full_payload)
      << "Multi-chunk snapshot must reconstruct identically to the original "
         "payload";
}

// Test 9: A follower rejects out-of-order chunks.
TEST_F(RaftSnapshotFileTest, FollowerRejectsOutOfOrderChunks) {
  ResDBConfig cfg = MakeConfig(follower_log_, /*self_id=*/2);
  auto storage = resdb::storage::NewMemoryDB();
  RaftRecovery recovery(cfg, nullptr, storage.get(), nullptr);

  MockSignatureVerifier verifier;
  MockLeaderElectionManager leader_election_manager(cfg);
  MockReplicaCommunicator replica_communicator;
  MockSendMessageFunction send_message;
  MockCommitFunction commit;

  Raft follower(2, 1, 4, &verifier, &leader_election_manager,
                &replica_communicator, &recovery, cfg);
  follower.SetSingleCallFunc(
      [&](int type, const google::protobuf::Message& msg, int node_id) {
        return send_message.Call(type, msg, node_id);
      });
  follower.SetCommitFunc(
      [&](const google::protobuf::Message& msg) { return commit.Commit(msg); });

  follower.SetStateForTest({
      .current_term = 4,
      .role = Role::FOLLOWER,
  });

  EXPECT_CALL(leader_election_manager, OnHeartbeat()).Times(AnyNumber());
  EXPECT_CALL(leader_election_manager, OnRoleChange()).Times(AnyNumber());

  // Capture the response sent back to the leader.
  std::vector<InstallSnapshotResponse> responses;
  EXPECT_CALL(send_message, Call(_, _, _)).WillRepeatedly(::testing::Return(0));
  EXPECT_CALL(send_message, Call(MessageType::InstallSnapshotResponseMsg, _, _))
      .WillRepeatedly(Invoke([&](int, const google::protobuf::Message& msg,
                                 int) {
        responses.push_back(dynamic_cast<const InstallSnapshotResponse&>(msg));
        return 0;
      }));

  // Build a 300-byte payload.
  const size_t chunk_size = 100;
  std::string full_payload;
  full_payload += std::string(chunk_size, 'A');  // chunk 0
  const size_t size_after_chunk0 = full_payload.size();
  full_payload += std::string(chunk_size, 'B');  // chunk 1
  full_payload += std::string(chunk_size, 'C');  // chunk 2

  // Deliver chunk 1 without chunk 0.
  std::string data1 = full_payload.substr(chunk_size, chunk_size);
  auto chunk1 = MakeChunk(4, 1, 30, 4, chunk_size, data1, false);
  follower.ReceiveInstallSnapshot(std::make_unique<InstallSnapshot>(chunk1));
  // Deliver chunk 0 and 2, skipping 1.
  for (size_t i = 0; i < 3; i += 2) {
    bool done = (i == 2);
    std::string data = full_payload.substr(i * chunk_size, chunk_size);
    auto chunk = MakeChunk(4, 1, 30, 4, i * chunk_size, data, done);
    follower.ReceiveInstallSnapshot(std::make_unique<InstallSnapshot>(chunk));
  }

  ASSERT_EQ(responses.size(), 3u);
  EXPECT_EQ(responses[0].bytes_stored(), 0);
  EXPECT_TRUE(responses[0].need_snapshot());
  EXPECT_FALSE(responses[0].transfer_complete());

  EXPECT_FALSE(responses.back().transfer_complete());
  EXPECT_TRUE(responses.back().need_snapshot());
  EXPECT_EQ(responses.back().bytes_stored(), size_after_chunk0);
}

// Test 10: A follower rejects a chunk from a different snapshot.
TEST_F(RaftSnapshotFileTest, FollowerRejectsChunksFromADifferentSnapshot) {
  ResDBConfig cfg = MakeConfig(follower_log_, /*self_id=*/2);
  auto storage = resdb::storage::NewMemoryDB();
  RaftRecovery recovery(cfg, nullptr, storage.get(), nullptr);

  MockSignatureVerifier verifier;
  MockLeaderElectionManager leader_election_manager(cfg);
  MockReplicaCommunicator replica_communicator;
  MockSendMessageFunction send_message;
  MockCommitFunction commit;

  Raft follower(2, 1, 4, &verifier, &leader_election_manager,
                &replica_communicator, &recovery, cfg);
  follower.SetSingleCallFunc(
      [&](int type, const google::protobuf::Message& msg, int node_id) {
        return send_message.Call(type, msg, node_id);
      });
  follower.SetCommitFunc(
      [&](const google::protobuf::Message& msg) { return commit.Commit(msg); });

  follower.SetStateForTest({
      .current_term = 4,
      .role = Role::FOLLOWER,
  });

  EXPECT_CALL(leader_election_manager, OnHeartbeat()).Times(AnyNumber());
  EXPECT_CALL(leader_election_manager, OnRoleChange()).Times(AnyNumber());

  // Capture the response sent back to the leader.
  std::vector<InstallSnapshotResponse> responses;
  EXPECT_CALL(send_message, Call(_, _, _)).WillRepeatedly(::testing::Return(0));
  EXPECT_CALL(send_message, Call(MessageType::InstallSnapshotResponseMsg, _, _))
      .WillRepeatedly(Invoke([&](int, const google::protobuf::Message& msg,
                                 int) {
        responses.push_back(dynamic_cast<const InstallSnapshotResponse&>(msg));
        return 0;
      }));

  // Build a 300-byte payload.
  const size_t chunk_size = 100;
  std::string full_payload;
  full_payload += std::string(chunk_size, 'A');  // chunk 0
  full_payload += std::string(chunk_size, 'B');  // chunk 1
  full_payload += std::string(chunk_size, 'C');  // chunk 2

  // Deliver Chunk 0, then chunk 1 of a different snapshot. (different last
  // term/index)
  std::string data0 = full_payload.substr(0, chunk_size);
  auto chunk0 = MakeChunk(4, 1, 30, 4, 0, data0, false);
  follower.ReceiveInstallSnapshot(std::make_unique<InstallSnapshot>(chunk0));

  std::string data1 = full_payload.substr(chunk_size, chunk_size);
  auto chunk1 = MakeChunk(4, 1, 40, 5, chunk_size, data1, false);
  follower.ReceiveInstallSnapshot(std::make_unique<InstallSnapshot>(chunk1));

  ASSERT_EQ(responses.size(), 2u);
  EXPECT_EQ(responses[0].bytes_stored(), chunk_size);
  EXPECT_TRUE(responses[0].need_snapshot());
  EXPECT_FALSE(responses[0].transfer_complete());

  EXPECT_FALSE(responses.back().transfer_complete());
  EXPECT_TRUE(responses.back().need_snapshot());
  EXPECT_EQ(responses.back().bytes_stored(), 0);
}

// Test 11: A follower rejects a snapshot from a stale term.
TEST_F(RaftSnapshotFileTest, FollowerRejectsSnapshotFromStaleTerm) {
  ResDBConfig cfg = MakeConfig(follower_log_, /*self_id=*/2);
  auto storage = resdb::storage::NewMemoryDB();
  RaftRecovery recovery(cfg, nullptr, storage.get(), nullptr);

  MockSignatureVerifier verifier;
  MockLeaderElectionManager leader_election_manager(cfg);
  MockReplicaCommunicator replica_communicator;
  MockSendMessageFunction send_message;
  MockCommitFunction commit;

  Raft follower(2, 1, 4, &verifier, &leader_election_manager,
                &replica_communicator, &recovery, cfg);
  follower.SetSingleCallFunc(
      [&](int type, const google::protobuf::Message& msg, int node_id) {
        return send_message.Call(type, msg, node_id);
      });
  follower.SetCommitFunc(
      [&](const google::protobuf::Message& msg) { return commit.Commit(msg); });

  follower.SetStateForTest({
      .current_term = 4,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries(
          {
              {0, "old-1"},
              {0, "old-2"},
          },
          true),
  });

  EXPECT_CALL(leader_election_manager, OnHeartbeat()).Times(AnyNumber());
  EXPECT_CALL(leader_election_manager, OnRoleChange()).Times(AnyNumber());

  // Capture the response sent back to the leader.
  std::vector<InstallSnapshotResponse> responses;
  EXPECT_CALL(send_message, Call(_, _, _)).WillRepeatedly(::testing::Return(0));
  EXPECT_CALL(send_message, Call(MessageType::InstallSnapshotResponseMsg, _, _))
      .WillRepeatedly(Invoke([&](int, const google::protobuf::Message& msg,
                                 int) {
        responses.push_back(dynamic_cast<const InstallSnapshotResponse&>(msg));
        return 0;
      }));

  const size_t chunk_size = 100;
  std::string full_payload;
  full_payload += std::string(chunk_size, 'A');  // chunk 0

  // Deliver a chunk of a snapshot from an old term.
  std::string data0 = full_payload.substr(0, chunk_size);
  auto chunk0 = MakeChunk(3, 1, 30, 4, 0, data0, false);
  const auto before_log = follower.GetLog();
  follower.ReceiveInstallSnapshot(std::make_unique<InstallSnapshot>(chunk0));
  const auto& after_log = follower.GetLog();

  EXPECT_EQ(before_log, after_log);
  ASSERT_EQ(responses.size(), 1u);
  EXPECT_EQ(responses[0].bytes_stored(), 0);
  EXPECT_FALSE(responses[0].need_snapshot());
  EXPECT_FALSE(responses[0].transfer_complete());
}

// Test 12: A leader receiving a rejected snapshot response sends a probe.
TEST_F(RaftSnapshotFileTest, LeaderFollowsARejectedSnapshotWithAProbe) {
  auto leader_storage = resdb::storage::NewMemoryDB();
  leader_storage->SetValueWithVersion("k1", "v1", 0);
  leader_storage->Flush();

  ResDBConfig leader_cfg = MakeConfig(leader_log_, /*self_id=*/1);
  RaftRecovery leader_recovery(leader_cfg, nullptr, leader_storage.get(),
                               nullptr);
  MockSignatureVerifier leader_verifier;
  MockLeaderElectionManager leader_leader_election_manager(leader_cfg);
  MockReplicaCommunicator leader_replica_communicator;
  MockSendMessageFunction leader_send_message;

  Raft leader(1, 1, 4, &leader_verifier, &leader_leader_election_manager,
              &leader_replica_communicator, &leader_recovery, leader_cfg);
  leader.SetStateForTest(
      {.current_term = 3,
       .role = Role::LEADER,
       .log = CreateLogEntries({}, true),
       CreateProgressPatch(
           {.next_index = std::vector<uint64_t>{1, 1, 1, 1, 1},
            .match_index = std::vector<uint64_t>{0, 0, 0, 0, 0},
            .states = std::vector<ProgressState>{
                ProgressState::PROBE, ProgressState::PROBE,
                ProgressState::SNAPSHOT, ProgressState::SNAPSHOT,
                ProgressState::SNAPSHOT}})});
  leader.SetSnapshotLastIndexAndTerm(2, 3, /*write_metadata=*/false);

  InstallSnapshotResponse isr;

  isr.set_term(3);
  isr.set_id(2);
  isr.set_bytes_stored(0);
  isr.set_need_snapshot(false);
  isr.set_last_included_index(0);
  isr.set_transfer_complete(false);

  EXPECT_CALL(leader_send_message, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 2);
            EXPECT_EQ(ae.prev_log_index(), 0);
            EXPECT_EQ(ae.entries().size(), 0);
            return 0;
          }));
  leader.SetSingleCallFunc(
      [&](int type, const google::protobuf::Message& msg, int node_id) {
        return leader_send_message.Call(type, msg, node_id);
      });
  leader.ReceiveInstallSnapshotResponse(
      std::make_unique<InstallSnapshotResponse>(isr));

  const auto& follower_progress = leader.GetFollowerProgress();
  EXPECT_EQ(follower_progress[2].state, ProgressState::PROBE);
}

// Test 13: A heartbeat from a leader with lagging followers triggers a
// snapshot to be sent to each follower whose next_index has fallen behind
// the truncated log.
TEST_F(RaftSnapshotFileTest, SendHeartbeatTriggersSnapshotForLaggingFollowers) {
  ResDBConfig leader_cfg = MakeConfig(leader_log_, /*self_id=*/1);
  auto leader_storage = resdb::storage::NewMemoryDB();
  RaftRecovery leader_recovery(leader_cfg, nullptr, leader_storage.get(),
                               nullptr);
  MockSignatureVerifier leader_verifier;
  MockLeaderElectionManager leader_election_manager(leader_cfg);
  MockReplicaCommunicator leader_replica_communicator;
  MockSendMessageFunction leader_send_message;

  Raft raft(1, 1, 4, &leader_verifier, &leader_election_manager,
            &leader_replica_communicator, &leader_recovery, leader_cfg);

  raft.SetStateForTest(
      {.current_term = 0,
       .role = Role::LEADER,
       .snapshot_last_index = 4,
       .snapshot_last_term = 0,
       .truncated_last_index = 3,
       .truncated_last_term = 0,
       .log = CreateLogEntries(
           {
               {0, "Term 0 Transaction 4"},
               {0, "Term 0 Transaction 5"},
               {0, "Term 0 Transaction 6"},
               {0, "Term 0 Transaction 7"},
           },
           true),
       .progress = CreateProgressPatch(
           {.next_index = std::vector<uint64_t>{1, 8, 2, 2, 2},
            .match_index = std::vector<uint64_t>{0, 7, 0, 0, 0},
            .states = std::vector<ProgressState>{ProgressState::PROBE,
                                                 ProgressState::REPLICATE,
                                                 ProgressState::SNAPSHOT,
                                                 ProgressState::SNAPSHOT,
                                                 ProgressState::SNAPSHOT}}),
       .enable_batching = false});

  raft.SetSingleCallFunc(
      [&](int type, const google::protobuf::Message& msg, int node_id) {
        return leader_send_message.Call(type, msg, node_id);
      });

  EXPECT_CALL(leader_election_manager, OnHeartbeat()).Times(AnyNumber());
  EXPECT_CALL(leader_election_manager, OnRoleChange()).Times(AnyNumber());
  EXPECT_CALL(leader_election_manager, OnAeBroadcast()).Times(AnyNumber());

  std::mutex snapshot_targets_mutex;
  std::condition_variable snapshot_targets_cv;
  std::set<int> snapshot_targets;

  EXPECT_CALL(leader_send_message, Call(_, _, _))
      .WillRepeatedly(::testing::Return(0));
  EXPECT_CALL(leader_send_message, Call(MessageType::InstallSnapshotMsg, _, _))
      .WillRepeatedly(
          Invoke([&](int, const google::protobuf::Message& msg, int node_id) {
            const auto& install_snapshot =
                dynamic_cast<const InstallSnapshot&>(msg);
            EXPECT_EQ(install_snapshot.leader_id(), 1);
            EXPECT_EQ(install_snapshot.last_included_index(), 4u);
            EXPECT_EQ(install_snapshot.last_included_term(), 0u);
            {
              std::lock_guard<std::mutex> lock(snapshot_targets_mutex);
              snapshot_targets.insert(node_id);
            }
            snapshot_targets_cv.notify_all();
            return 0;
          }));

  raft.SendHeartbeat();

  // Wait for the thread to send the snapshots.
  std::unique_lock<std::mutex> lock(snapshot_targets_mutex);
  const bool got_all_targets =
      snapshot_targets_cv.wait_for(lock, std::chrono::seconds(2), [&] {
        return snapshot_targets.count(2) == 1 &&
               snapshot_targets.count(3) == 1 && snapshot_targets.count(4) == 1;
      });

  EXPECT_TRUE(got_all_targets)
      << "Expected InstallSnapshotMsg to be sent to followers 2, 3, and 4 "
      << "after SendHeartbeat(); observed targets: " << [&] {
           std::string joined;
           for (int target : snapshot_targets) {
             joined += std::to_string(target) + " ";
           }
           return joined;
         }();

  // Ensure that the leader does not try to send a snapshot to itself.
  EXPECT_EQ(snapshot_targets.count(1), 0u);
}

}  // namespace raft
}  // namespace resdb

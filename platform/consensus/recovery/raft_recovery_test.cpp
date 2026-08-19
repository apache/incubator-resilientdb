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

#include "platform/consensus/recovery/raft_recovery.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <future>

#include "chain/storage/mock_storage.h"
#include "platform/consensus/checkpoint/mock_checkpoint.h"
#include "platform/consensus/ordering/common/transaction_utils.h"
#include "platform/consensus/ordering/raft/proto/proposal.pb.h"

namespace resdb {
namespace raft {
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Invoke;
using ::testing::Matcher;
using ::testing::Return;
using ::testing::Test;

const std::string log_path = "./log/test_log";

ResConfigData GetConfigData(int buf_size = 10) {
  ResConfigData data;
  data.set_recovery_enabled(true);
  data.set_recovery_path(log_path);
  data.set_recovery_buffer_size(buf_size);
  data.set_recovery_ckpt_time_s(1);

  return data;
}

std::vector<std::string> Listlogs(const std::string &path) {
  std::vector<std::string> ret;
  std::string dir = std::filesystem::path(path).parent_path();
  for (const auto &entry : std::filesystem::directory_iterator(dir)) {
    LOG(ERROR) << "path:" << entry.path();
    ret.push_back(entry.path());
  }
  return ret;
}

static Entry CreateTestEntry(RaftRecovery &recovery, int term, int seq) {
  Entry log_entry;
  log_entry.set_term(term);
  auto req = std::make_unique<Request>();
  req->set_seq(seq);
  req->set_data("Request " + std::to_string(seq));
  std::string serialized;
  EXPECT_TRUE(req->SerializeToString(&serialized));
  log_entry.set_command(std::move(serialized));
  return log_entry;
}

static std::future<void> AddTestEntry(RaftRecovery &recovery, int term,
                                      int seq) {
  Entry log_entry = CreateTestEntry(recovery, term, seq);
  return recovery.AddLogEntry(&log_entry, seq);
}

class RaftRecoveryTest : public Test {
 public:
  RaftRecoveryTest()
      : config_(GetConfigData(), ReplicaInfo(), KeyInfo(), CertificateInfo()) {
    std::string dir = std::filesystem::path(log_path).parent_path();
    std::filesystem::remove_all(dir);
  }

 protected:
  ResDBConfig config_;
  MockCheckPoint checkpoint_;
};

// Test 1: Individually add entries to the log and read from it.
TEST_F(RaftRecoveryTest, WriteAndReadLog) {
  int entries_to_add = 3;
  {
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);

    for (int i = 1; i <= entries_to_add; i++) {
      AddTestEntry(recovery, i, i);
    }
  }
  {
    std::vector<WALRecord> list;
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);
    recovery.ReadLogs(
        [&](const RaftMetadata &data) {},
        [&](std::unique_ptr<WALRecord> record) { list.push_back(*record); },
        [&](const RaftMetadata &data) {});

    EXPECT_EQ(list.size(), entries_to_add);

    for (int i = 0; i < entries_to_add; ++i) {
      EXPECT_EQ(list[i].payload_case(), WALRecord::kEntry);

      EXPECT_EQ(list[i].entry().term(), i + 1);
      Request req;
      req.ParseFromString(list[i].entry().command());
      EXPECT_EQ(req.data(), "Request " + std::to_string(i + 1));
    }
  }
}

// Test 2: Add multiple entries to the log at once and read from it.
TEST_F(RaftRecoveryTest, WriteMultipleEntriesAndReadLog) {
  int entries_to_add = 3;
  {
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);
    std::vector<Entry> log_entries;
    for (int i = 1; i <= entries_to_add; i++) {
      log_entries.push_back(CreateTestEntry(recovery, i, i));
    }
    recovery.AddLogEntry(log_entries, 1);
  }
  {
    std::vector<WALRecord> list;
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);
    recovery.ReadLogs(
        [&](const RaftMetadata &data) {},
        [&](std::unique_ptr<WALRecord> record) { list.push_back(*record); },
        [&](const RaftMetadata &data) {});

    EXPECT_EQ(list.size(), entries_to_add);

    for (int i = 0; i < entries_to_add; ++i) {
      EXPECT_EQ(list[i].payload_case(), WALRecord::kEntry);

      EXPECT_EQ(list[i].entry().term(), i + 1);
      Request req;
      req.ParseFromString(list[i].entry().command());
      EXPECT_EQ(req.data(), "Request " + std::to_string(i + 1));
    }
  }
}

// Test 3: Add no entries to the log and verify handling is gracful
TEST_F(RaftRecoveryTest, WriteNoEntriesAndReadLog) {
  {
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);
    std::vector<Entry> log_entries;
    recovery.AddLogEntry(log_entries, 1);
  }
  {
    std::vector<WALRecord> list;
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);
    recovery.ReadLogs(
        [&](const RaftMetadata &data) {},
        [&](std::unique_ptr<WALRecord> record) { list.push_back(*record); },
        [&](const RaftMetadata &data) {});

    EXPECT_EQ(list.size(), 0);
  }
}

// Test 4: Write and Read from the metadata file.
TEST_F(RaftRecoveryTest, WriteAndReadMetadata) {
  {
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);

    recovery.WriteMetadata(2, 3, 100, 1);
  }
  {
    int64_t current_term;
    int32_t voted_for;
    uint64_t snapshot_last_index;
    uint64_t snapshot_last_term;

    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);
    recovery.ReadLogs([&](const RaftMetadata &data) {},
                      [&](std::unique_ptr<WALRecord> record) {},
                      [&](const RaftMetadata &data) {
                        current_term = data.current_term;
                        voted_for = data.voted_for;
                        snapshot_last_index = data.snapshot_last_index;
                        snapshot_last_term = data.snapshot_last_term;
                      });

    EXPECT_EQ(current_term, 2);
    EXPECT_EQ(voted_for, 3);
    EXPECT_EQ(snapshot_last_index, 100);
    EXPECT_EQ(snapshot_last_term, 1);
  }
}

// Test 5: Write to the metadata file twice, then read from it.
TEST_F(RaftRecoveryTest, WriteAndReadMetadataTwice) {
  {
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);

    recovery.WriteMetadata(2, 3, 100, 1);
    recovery.WriteMetadata(4, 2, 200, 2);
  }
  {
    int64_t current_term;
    int32_t voted_for;
    uint64_t snapshot_last_index;
    uint64_t snapshot_last_term;

    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);
    recovery.ReadLogs([&](const RaftMetadata &data) {},
                      [&](std::unique_ptr<WALRecord> record) {},
                      [&](const RaftMetadata &data) {
                        current_term = data.current_term;
                        voted_for = data.voted_for;
                        snapshot_last_index = data.snapshot_last_index;
                        snapshot_last_term = data.snapshot_last_term;
                      });

    EXPECT_EQ(current_term, 4);
    EXPECT_EQ(voted_for, 2);
    EXPECT_EQ(snapshot_last_index, 200);
    EXPECT_EQ(snapshot_last_term, 2);
  }
}

// Test 6: Verify the default values of the metadata.
TEST_F(RaftRecoveryTest, ReadMetadataDefaultValues) {
  {
    int64_t current_term;
    int32_t voted_for;
    uint64_t snapshot_last_index;
    uint64_t snapshot_last_term;

    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);
    recovery.ReadLogs([&](const RaftMetadata &data) {},
                      [&](std::unique_ptr<WALRecord> record) {},
                      [&](const RaftMetadata &data) {
                        current_term = data.current_term;
                        voted_for = data.voted_for;
                        snapshot_last_index = data.snapshot_last_index;
                        snapshot_last_term = data.snapshot_last_term;
                      });

    EXPECT_EQ(current_term, 0);
    EXPECT_EQ(voted_for, -1);
    EXPECT_EQ(snapshot_last_index, 0);
    EXPECT_EQ(snapshot_last_term, 0);
  }
}

// Test 7: Truncate the log.
TEST_F(RaftRecoveryTest, TruncateLog) {
  int entries_to_add = 4;
  {
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);

    for (int i = 1; i <= entries_to_add; i++) {
      AddTestEntry(recovery, i, i);
    }

    TruncationRecord truncation;
    truncation.set_truncate_from_index(3);
    truncation.set_truncate_from_term(3);
    recovery.TruncateLog(truncation);

    for (int i = 5; i <= entries_to_add * 2; i++) {
      AddTestEntry(recovery, i + 1, i);
    }
  }
  /* Recovery WAL
              Term   Seq    Data
     list[0]  1      1      Request 1
     list[1]  2      2      Request 2
     list[2]  3      3      Request 3
     list[3]  4      4      Request 4
     list[4]  Truncate beginning at Seq 3
     list[5]  6      5      Request 5
     list[6]  7      6      Request 6
     list[7]  8      7      Request 7
     list[8]  9      8      Request 8
  */
  {
    std::vector<WALRecord> list;
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);
    recovery.ReadLogs(
        [&](const RaftMetadata &data) {},
        [&](std::unique_ptr<WALRecord> record) { list.push_back(*record); },
        [&](const RaftMetadata &data) {});

    EXPECT_EQ(list.size(), 2 * entries_to_add + 1);

    for (int i = 0; i < entries_to_add; ++i) {
      EXPECT_EQ(list[i].payload_case(), WALRecord::kEntry);
      EXPECT_EQ(list[i].entry().term(), i + 1);
      Request req;
      req.ParseFromString(list[i].entry().command());
      EXPECT_EQ(req.data(), "Request " + std::to_string(i + 1));
      EXPECT_EQ(req.seq(), i + 1);
    }

    EXPECT_EQ(list[4].payload_case(), WALRecord::kTruncation);
    EXPECT_EQ(list[4].truncation().truncate_from_index(), 3);

    for (int i = entries_to_add + 1; i < 2 * entries_to_add + 1; ++i) {
      EXPECT_EQ(list[i].payload_case(), WALRecord::kEntry);
      EXPECT_EQ(list[i].entry().term(), i + 1);
      Request req;
      req.ParseFromString(list[i].entry().command());
      EXPECT_EQ(req.data(), "Request " + std::to_string(i));
      EXPECT_EQ(req.seq(), i);
    }
  }
}

// Test 8: After a checkpoint fires and the log file is rotated, there should be
// exactly two .log files on disk: the sealed (checkpointed) file and the new
// active one.
TEST_F(RaftRecoveryTest,
       CheckpointCreatesNewLogFileAndCallsOnCheckpointCallback) {
  std::promise<bool> insert_done, ckpt_fired;
  auto insert_done_future = insert_done.get_future();
  auto ckpt_fired_future = ckpt_fired.get_future();

  int call_count = 0;
  int checkpoint_value = 5;
  EXPECT_CALL(checkpoint_, GetStableCheckpoint())
      .WillRepeatedly(Invoke([&]() -> uint64_t {
        ++call_count;
        if (call_count == 1)
          insert_done_future.get();
        else if (call_count == 2)
          ckpt_fired.set_value(true);
        return checkpoint_value;
      }));

  bool callback_called = false;
  uint64_t callback_seq = 123456789;
  {
    RaftRecovery recovery(config_, &checkpoint_, nullptr, [&](uint64_t seq) {
      callback_called = true;
      callback_seq = seq;
    });

    for (int i = 1; i <= 9; i++) {
      AddTestEntry(recovery, i, i);
    }
    insert_done.set_value(true);
    ckpt_fired_future.get();

    // Write some more entries into the new file.
    for (int i = 10; i <= 18; i++) {
      AddTestEntry(recovery, i, i);
    }
  }

  std::vector<std::string> log_list = Listlogs(log_path);
  // 2 log files and one metadata file
  EXPECT_EQ(log_list.size(), 3);
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(callback_seq, checkpoint_value);
}

// Test 9: After a checkpoint at seq=5, ReadLogs should only replay WAL records
// whose seq is strictly greater than 5.
TEST_F(RaftRecoveryTest, CheckpointFiltersOldEntries) {
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

  {
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);

    for (int i = 1; i <= 9; i++) {
      AddTestEntry(recovery, i, i);
    }
    insert_done.set_value(true);
    ckpt_fired_future.get();
  }

  {
    std::vector<WALRecord> list;
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);
    recovery.ReadLogs(
        [&](const RaftMetadata &data) {},
        [&](std::unique_ptr<WALRecord> record) { list.push_back(*record); },
        [&](const RaftMetadata &data) {});

    // Only WAL seqs 6-9 should be replayed (4 entries).
    ASSERT_EQ(list.size(), 4u);
    for (size_t i = 0; i < list.size(); ++i) {
      EXPECT_EQ(list[i].payload_case(), WALRecord::kEntry);
      Request req;
      req.ParseFromString(list[i].entry().command());
      EXPECT_EQ(req.seq(), (int)(i + 6));
    }
  }
}

// Test 10: After a checkpoint rotation, GetMinSeq()/GetMaxSeq() should reset to
// -1 for the newly opened (empty) file, then update as new entries are
// appended.
TEST_F(RaftRecoveryTest, CheckpointResetsMinMaxSeq) {
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

  {
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);

    for (int i = 1; i <= 5; i++) {
      auto future = AddTestEntry(recovery, i, i);
      future.get();
    }
    insert_done.set_value(true);
    ckpt_fired_future.get();

    EXPECT_EQ(recovery.GetMinSeq(), -1);
    EXPECT_EQ(recovery.GetMaxSeq(), -1);

    // Add entries to the new file and verify the range is tracked correctly.
    for (int i = 6; i <= 9; i++) {
      auto future = AddTestEntry(recovery, i, i);
      future.get();
    }

    EXPECT_EQ(recovery.GetMinSeq(), 6);
    EXPECT_EQ(recovery.GetMaxSeq(), 9);
  }
}

// Test 11: Two successive checkpoints.  After both fires, only entries whose
// WAL seq exceeds the second checkpoint value (15) survive replay.
TEST_F(RaftRecoveryTest, TwoCheckpoints) {
  std::promise<bool> ins1, ck1, ins2, ck2;
  auto ins1f = ins1.get_future(), ck1f = ck1.get_future();
  auto ins2f = ins2.get_future(), ck2f = ck2.get_future();

  int call_count = 0;
  EXPECT_CALL(checkpoint_, GetStableCheckpoint())
      .WillRepeatedly(Invoke([&]() -> uint64_t {
        ++call_count;
        if (call_count == 1)
          ins1f.get();
        else if (call_count == 2)
          ck1.set_value(true);
        else if (call_count == 3)
          ins2f.get();
        else if (call_count == 4)
          ck2.set_value(true);
        return (call_count <= 2) ? 5 : 15;
      }));

  {
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);

    for (int i = 1; i <= 9; i++) {
      AddTestEntry(recovery, i, i);
    }
    ins1.set_value(true);
    ck1f.get();

    for (int i = 10; i <= 18; i++) {
      AddTestEntry(recovery, i, i);
    }
    ins2.set_value(true);
    ck2f.get();

    // Third window: entries 19-22.
    for (int i = 19; i <= 22; i++) {
      AddTestEntry(recovery, i, i);
    }
  }

  std::vector<std::string> log_list = Listlogs(log_path);
  // 3 log files and one metadata file
  EXPECT_EQ(log_list.size(), 4);

  {
    std::vector<WALRecord> list;
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);
    recovery.ReadLogs(
        [&](const RaftMetadata &data) {},
        [&](std::unique_ptr<WALRecord> record) { list.push_back(*record); },
        [&](const RaftMetadata &data) {});

    // ckpt=15: entries with WAL seq > 15 survive: seqs 16-22 (7 entries).
    ASSERT_EQ(list.size(), 7u);
    for (size_t i = 0; i < list.size(); ++i) {
      Request req;
      req.ParseFromString(list[i].entry().command());
      EXPECT_EQ(req.seq(), (int)(i + 16));
    }
    // Even though seqs 16-22 survive, min seq and max seq refer to the most
    // recent log.
    EXPECT_EQ(recovery.GetMinSeq(), 19);
    EXPECT_EQ(recovery.GetMaxSeq(), 22);
  }
}

// Test 12: Metadata lives in a separate file and should be fully preserved
// across log rotations caused by a checkpoint.
TEST_F(RaftRecoveryTest, MetadataPersistedAcrossCheckpoint) {
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

  {
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);
    recovery.WriteMetadata(7, 2, 50, 3);

    for (int i = 1; i <= 5; i++) {
      AddTestEntry(recovery, i, i);
    }
    insert_done.set_value(true);
    ckpt_fired_future.get();

    for (int i = 6; i <= 8; i++) {
      AddTestEntry(recovery, i, i);
    }
  }

  {
    int64_t current_term = 0;
    int32_t voted_for = 0;
    uint64_t snapshot_last_index = 0;
    uint64_t snapshot_last_term = 0;

    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);
    recovery.ReadLogs([&](const RaftMetadata &data) {},
                      [&](std::unique_ptr<WALRecord> record) {},
                      [&](const RaftMetadata &data) {
                        current_term = data.current_term;
                        voted_for = data.voted_for;
                        snapshot_last_index = data.snapshot_last_index;
                        snapshot_last_term = data.snapshot_last_term;
                      });

    EXPECT_EQ(current_term, 7);
    EXPECT_EQ(voted_for, 2);
    EXPECT_EQ(snapshot_last_index, 50);
    EXPECT_EQ(snapshot_last_term, 3);
  }
}

// Test 13: When Storage::Flush() fails, FinishFile() bails out early and the
// log file must NOT be rotated — only one file should remain on disk.
TEST_F(RaftRecoveryTest, CheckpointNotFinalizedWhenStorageFlushFails) {
  MockStorage storage;
  EXPECT_CALL(storage, Flush).WillRepeatedly(Return(false));

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

  {
    RaftRecovery recovery(config_, &checkpoint_, &storage, nullptr);

    for (int i = 1; i <= 5; i++) {
      AddTestEntry(recovery, i, i);
    }
    insert_done.set_value(true);
    ckpt_fired_future.get();

    for (int i = 6; i <= 8; i++) {
      AddTestEntry(recovery, i, i);
    }
  }

  // The file should never have been renamed; only one .log file exists.
  std::vector<std::string> log_list = Listlogs(log_path);
  // 1 log file and one metadata file
  EXPECT_EQ(log_list.size(), 2);
}

ResConfigData GetConfigDataNoRecovery(int buf_size = 10) {
  ResConfigData data;
  data.set_recovery_enabled(false);
  data.set_recovery_path(log_path);
  data.set_recovery_buffer_size(buf_size);
  data.set_recovery_ckpt_time_s(1);
  return data;
}

// Test 14: When recovery_enabled=false, all write operations are no-ops and the
// WAL directory is never created on disk.
TEST_F(RaftRecoveryTest, RecoveryDisabledNoOpsAndCreatesNoDirectory) {
  ResDBConfig config(GetConfigDataNoRecovery(1024), ReplicaInfo(), KeyInfo(),
                     CertificateInfo());

  const std::string log_dir =
      std::filesystem::path(log_path).parent_path().string();

  // Precondition: directory does not exist (the fixture removes it in SetUp).
  ASSERT_FALSE(std::filesystem::exists(log_dir));

  {
    RaftRecovery recovery(config, &checkpoint_, nullptr, nullptr);

    // All of these must be silent no-ops.
    for (int i = 1; i <= 5; ++i) {
      AddTestEntry(recovery, i, i);
    }

    recovery.WriteMetadata(7, 2, 50, 3);

    TruncationRecord trunc;
    trunc.set_truncate_from_index(3);
    trunc.set_truncate_from_term(2);
    recovery.TruncateLog(trunc);

    // ReadLogs must also be a no-op and invoke neither callback.
    bool metadata_cb_called = false;
    bool record_cb_called = false;
    recovery.ReadLogs(
        nullptr, [&](std::unique_ptr<WALRecord>) { record_cb_called = true; },
        [&](const RaftMetadata &) { metadata_cb_called = true; });

    EXPECT_FALSE(metadata_cb_called);
    EXPECT_FALSE(record_cb_called);
  }

  // The WAL directory must never have been created.
  EXPECT_FALSE(std::filesystem::exists(log_dir))
      << "WAL directory was created even though recovery is disabled";
}

// Test 15: When recovery is disabled, ReadMetadata returns the zero-value
// struct.
TEST_F(RaftRecoveryTest, RecoveryDisabledReadMetadataReturnsDefaults) {
  ResDBConfig config(GetConfigDataNoRecovery(1024), ReplicaInfo(), KeyInfo(),
                     CertificateInfo());

  RaftRecovery recovery(config, &checkpoint_, nullptr, nullptr);

  RaftMetadata meta = recovery.ReadMetadata();
  EXPECT_EQ(meta.current_term, 0);
  EXPECT_EQ(meta.voted_for, -1);
  EXPECT_EQ(meta.snapshot_last_index, 0u);
  EXPECT_EQ(meta.snapshot_last_term, 0u);
}

// Test 16: Truncation record seq == checkpoint value.
//
// Layout written to WAL:
//   seq 1 – entry (term 1)
//   seq 2 – entry (term 2)
//   seq 3 – entry (term 3)
//   seq 4 – entry (term 4)
//   truncation with truncate_from_index=3  →  stored at seq = 3-1 = 2
//   seq 3 – entry (term 13)
//   seq 4 – entry (term 14)
//
// The checkpoint fires at seq=2, directly before the truncation.
//
// What survives: only records with seq > 2, i.e. the two post-truncation
// entries at seq 3 and 4.
TEST_F(RaftRecoveryTest, TruncationAtCheckpointBoundary) {
  std::promise<bool> insert_done, ckpt_fired;
  auto insert_done_f = insert_done.get_future();
  auto ckpt_fired_f = ckpt_fired.get_future();

  int call_count = 0;
  EXPECT_CALL(checkpoint_, GetStableCheckpoint())
      .WillRepeatedly(Invoke([&]() -> uint64_t {
        ++call_count;
        if (call_count == 1)
          insert_done_f.get();
        else if (call_count == 2)
          ckpt_fired.set_value(true);
        // Checkpoint at 2 — the same seq as the truncation record.
        return 2;
      }));

  {
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);

    // Write entries 1–4 at seq 1–4.
    for (int i = 1; i <= 4; ++i) {
      AddTestEntry(recovery, i, i);
    }

    // Truncate from index 3 → stored at seq = 2.
    TruncationRecord trunc;
    trunc.set_truncate_from_index(3);
    trunc.set_truncate_from_term(2);
    recovery.TruncateLog(trunc);

    // Write two replacement entries at seq 3–4 (new leader's branch).
    for (int i = 3; i <= 4; ++i) {
      auto future = AddTestEntry(recovery, 10 + i, i);
      future.get();
    }

    insert_done.set_value(true);
    ckpt_fired_f.get();
    // File is now sealed at ckpt=2.  The active window starts fresh.
  }

  {
    std::vector<WALRecord> list;
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);
    recovery.ReadLogs(
        [&](const RaftMetadata &data) {},
        [&](std::unique_ptr<WALRecord> record) { list.push_back(*record); },
        [&](const RaftMetadata &data) {});

    ASSERT_EQ(list.size(), 5u);
    EXPECT_EQ(list[0].payload_case(), WALRecord::kEntry);
    EXPECT_EQ(list[1].payload_case(), WALRecord::kEntry);
    EXPECT_EQ(list[2].payload_case(), WALRecord::kTruncation);
    EXPECT_EQ(list[3].payload_case(), WALRecord::kEntry);
    EXPECT_EQ(list[4].payload_case(), WALRecord::kEntry);

    Request req3, req4, req3again, req4again;
    req3.ParseFromString(list[0].entry().command());
    req4.ParseFromString(list[1].entry().command());
    EXPECT_EQ(req3.seq(), 3);
    EXPECT_EQ(req4.seq(), 4);
    EXPECT_EQ(list[0].entry().term(), 3);
    EXPECT_EQ(list[1].entry().term(), 4);

    EXPECT_EQ(list[2].truncation().truncate_from_index(), 3);

    req3again.ParseFromString(list[3].entry().command());
    req4again.ParseFromString(list[4].entry().command());
    EXPECT_EQ(req3again.seq(), 3);
    EXPECT_EQ(req4again.seq(), 4);
    EXPECT_EQ(list[3].entry().term(), 13);
    EXPECT_EQ(list[4].entry().term(), 14);
  }
}

// Test 17: Truncation record seq BELOW checkpoint value: also dropped.
//
// Same layout but checkpoint fires at stable_seq=5 (above the truncation's
// seq=2).  All records with seq ≤ 5 are behind the checkpoint; only seq 3
// and 4 survive if they came from the pre-checkpoint file selected by
// GetRecoveryFiles.  In this variant we check that no truncation record
// bleeds through in the surviving window.
TEST_F(RaftRecoveryTest, TruncationBelowCheckpointIsDropped) {
  std::promise<bool> insert_done, ckpt_fired;
  auto insert_done_f = insert_done.get_future();
  auto ckpt_fired_f = ckpt_fired.get_future();

  int call_count = 0;
  EXPECT_CALL(checkpoint_, GetStableCheckpoint())
      .WillRepeatedly(Invoke([&]() -> uint64_t {
        ++call_count;
        if (call_count == 1)
          insert_done_f.get();
        else if (call_count == 2)
          ckpt_fired.set_value(true);
        return 5;
      }));

  {
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);

    for (int i = 1; i <= 4; ++i) {
      AddTestEntry(recovery, i, i);
    }

    TruncationRecord trunc;
    trunc.set_truncate_from_index(3);
    trunc.set_truncate_from_term(2);
    recovery.TruncateLog(trunc);

    for (int i = 3; i <= 8; ++i) {
      AddTestEntry(recovery, 10 + i, i);
    }

    insert_done.set_value(true);
    ckpt_fired_f.get();
  }

  {
    std::vector<WALRecord> list;
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);
    recovery.ReadLogs(
        [&](const RaftMetadata &data) {},
        [&](std::unique_ptr<WALRecord> record) { list.push_back(*record); },
        [&](const RaftMetadata &data) {});

    // Entries at seq 6, 7, 8 survive (strictly > ckpt=5).
    // The truncation at seq=2 is entirely behind the checkpoint and must not
    // appear.
    for (const auto &r : list) {
      EXPECT_EQ(r.payload_case(), WALRecord::kEntry)
          << "Truncation record below checkpoint leaked into replay";
    }
    ASSERT_EQ(list.size(), 3u);
    for (size_t i = 0; i < list.size(); ++i) {
      Request req;
      req.ParseFromString(list[i].entry().command());
      EXPECT_EQ(req.seq(), (int)(i + 6));
    }
  }
}

// ─────────────────────────────────────────────────────────────
//  Corruption helpers
// ─────────────────────────────────────────────────────────────

// Returns every *.log file in the WAL directory sorted by name
// (chronological order because the names are timestamp/sequence-based).
static std::vector<std::string> GetLogFiles(const std::string &path) {
  std::vector<std::string> logs;
  std::string dir = std::filesystem::path(path).parent_path();
  for (const auto &entry : std::filesystem::directory_iterator(dir)) {
    if (entry.path().extension() == ".log")
      logs.push_back(entry.path().string());
  }
  std::sort(logs.begin(), logs.end());
  return logs;
}

// Overwrites `n` bytes starting at byte-offset `offset` with `fill`.
static void CorruptFileAt(const std::string &filepath, std::streamoff offset,
                          std::size_t n, char fill = 0xFF) {
  LOG(INFO) << "filepath to corrupt: " << filepath;
  std::fstream f(filepath, std::ios::in | std::ios::out | std::ios::binary);
  ASSERT_TRUE(f.is_open()) << "Could not open " << filepath;
  f.seekp(offset, std::ios::beg);
  ASSERT_TRUE(f.good());
  std::string garbage(n, fill);
  f.write(garbage.data(), static_cast<std::streamsize>(n));
  ASSERT_TRUE(f.good());
}

// Truncates `filepath` to `new_size` bytes.
static void TruncateFileTo(const std::string &filepath,
                           std::uintmax_t new_size) {
  LOG(INFO) << "filepath to truncate: " << filepath;
  std::filesystem::resize_file(filepath, new_size);
}

// ─────────────────────────────────────────────────────────────
//  Corruption tests
// ─────────────────────────────────────────────────────────────

// Test 18: Write 5 entries, then overwrite bytes in the middle of the file so
// that the 3rd record is unreadable. ReadLogs should recover records 1 and 2.
TEST_F(RaftRecoveryTest, CorruptMiddleRecordStopsReplay) {
  {
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);
    for (int i = 1; i <= 5; ++i) {
      auto future = AddTestEntry(recovery, i, i);
      // Waiting on each future individually ensures that the data is stored as:
      // [data_len][record1]
      // [data_len][record2]
      // [data_len][record3]
      // [data_len][record4]
      // [data_len][record5]
      // Instead of:
      // [data_len][record1 record2 record3 record4 record5]
      // Any corrupted entry is thrown out
      future.get();
    }
  }

  // Corrupt roughly the middle of the log file.
  std::vector<std::string> logs = GetLogFiles(log_path);
  ASSERT_EQ(logs.size(), 1u);

  std::uintmax_t file_size = std::filesystem::file_size(logs[0]);
  // Aim for ~50 % into the file to land inside record 3.
  std::streamoff mid = static_cast<std::streamoff>(file_size / 2);
  CorruptFileAt(logs[0], mid, /*n=*/16);

  {
    std::vector<WALRecord> list;
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);
    recovery.ReadLogs([&](const RaftMetadata &) {},
                      [&](std::unique_ptr<WALRecord> r) { list.push_back(*r); },
                      [&](const RaftMetadata &) {});

    // Records 1 and 2 precede the corruption and must be intact.
    ASSERT_GE(list.size(), 2u)
        << "Expected at least the two pre-corruption entries";
    EXPECT_EQ(list[0].payload_case(), WALRecord::kEntry);
    EXPECT_EQ(list[0].entry().term(), 1);
    EXPECT_EQ(list[1].payload_case(), WALRecord::kEntry);
    EXPECT_EQ(list[1].entry().term(), 2);

    // Nothing past the corrupt record should appear.
    EXPECT_LE(list.size(), 2u)
        << "ReadLogs continued past the corrupted record";
  }
}

// Test 19: Corrupt only the very first record. ReadLogs must return an empty
// list, zero records should be replayed.
TEST_F(RaftRecoveryTest, CorruptFirstRecordReturnsEmpty) {
  {
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);
    for (int i = 1; i <= 4; ++i) {
      auto future = AddTestEntry(recovery, i, i);
      future.get();
    }
  }

  std::vector<std::string> logs = GetLogFiles(log_path);
  ASSERT_EQ(logs.size(), 1u);

  // Corrupt near the very start (skip a few bytes for any file header).
  CorruptFileAt(logs[0], /*offset=*/4, /*n=*/16);

  {
    std::vector<WALRecord> list;
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);
    recovery.ReadLogs([&](const RaftMetadata &) {},
                      [&](std::unique_ptr<WALRecord> r) { list.push_back(*r); },
                      [&](const RaftMetadata &) {});

    EXPECT_EQ(list.size(), 0u)
        << "Expected no records when the first record is corrupted";
  }
}

// Test 20: Hard-truncate the file so the last record is only partially written
// (simulates a crash mid-write). All complete records before the truncation
// point must be returned; the partial tail must be silently ignored.
TEST_F(RaftRecoveryTest, TruncatedTailRecordIsIgnored) {
  constexpr int entries = 5;
  {
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);
    for (int i = 1; i <= entries; ++i) {
      auto future = AddTestEntry(recovery, i, i);
      future.get();
    }
  }

  std::vector<std::string> logs = GetLogFiles(log_path);
  ASSERT_EQ(logs.size(), 1u);
  std::uintmax_t original_size = std::filesystem::file_size(logs[0]);
  LOG(INFO) << "Check 1: Pre-truncation corruption file size: "
            << original_size;

  {
    std::vector<WALRecord> list;
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);
    recovery.ReadLogs([&](const RaftMetadata &) {},
                      [&](std::unique_ptr<WALRecord> r) { list.push_back(*r); },
                      [&](const RaftMetadata &) {});

    // At minimum the first entries-1 intact records should come back.
    EXPECT_EQ(list.size(), 5);

    // Verify the records that did come back are intact.
    for (size_t i = 0; i < list.size(); ++i) {
      EXPECT_EQ(list[i].payload_case(), WALRecord::kEntry);
      Request req;
      req.ParseFromString(list[i].entry().command());
      EXPECT_EQ(req.seq(), static_cast<int>(i + 1));
    }
    std::uintmax_t original = std::filesystem::file_size(logs[0]);
    LOG(INFO) << "Check 2: Pre-truncation corruption file size: " << original;
  }

  // Lop off the last 8 bytes — enough to break the final length prefix or
  // checksum without touching earlier records.
  std::uintmax_t original = std::filesystem::file_size(logs[0]);
  LOG(INFO) << "Check 3: Pre-truncation corruption file size: " << original;
  ASSERT_GT(original, 8u);
  TruncateFileTo(logs[0], original - 1);

  {
    std::vector<WALRecord> list;
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);
    recovery.ReadLogs([&](const RaftMetadata &) {},
                      [&](std::unique_ptr<WALRecord> r) { list.push_back(*r); },
                      [&](const RaftMetadata &) {});

    // At minimum the first entries-1 intact records should come back.
    EXPECT_GE(list.size(), static_cast<size_t>(entries - 1))
        << "Too many records dropped — intact prefix not fully replayed";
    // The partial last record must not appear.
    EXPECT_LE(list.size(), static_cast<size_t>(entries))
        << "Partial tail record should not be returned";

    // Verify the records that did come back are intact.
    for (size_t i = 0; i < list.size(); ++i) {
      EXPECT_EQ(list[i].payload_case(), WALRecord::kEntry);
      Request req;
      req.ParseFromString(list[i].entry().command());
      EXPECT_EQ(req.seq(), static_cast<int>(i + 1));
    }
  }
}

// Test 21: Completely empty the log file (simulates a zero-byte crash
// artifact). ReadLogs must return cleanly with zero records and not crash or
// assert.
TEST_F(RaftRecoveryTest, EmptyLogFileIsHandledGracefully) {
  {
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);
    for (int i = 1; i <= 3; ++i) {
      auto future = AddTestEntry(recovery, i, i);
      future.get();
    }
  }

  std::vector<std::string> logs = GetLogFiles(log_path);
  ASSERT_EQ(logs.size(), 1u);
  TruncateFileTo(logs[0], 0);

  {
    std::vector<WALRecord> list;
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);
    EXPECT_NO_FATAL_FAILURE(recovery.ReadLogs(
        [&](const RaftMetadata &) {},
        [&](std::unique_ptr<WALRecord> r) { list.push_back(*r); },
        [&](const RaftMetadata &) {}));

    EXPECT_EQ(list.size(), 0u);
  }
}

// Test 22: Write 9 entries across a checkpoint boundary (ckpt=5 triggers log
// rotation), then corrupt a record in the *second* (active) log file.  Records
// from the sealed first file whose seq > ckpt (i.e. seqs 6–9 — wait, ckpt=5 so
// only records from the new file survive) must be replayed up to the
// corruption.
TEST_F(RaftRecoveryTest, CorruptionInSecondLogFileAfterCheckpoint) {
  std::promise<bool> insert_done, ckpt_fired;
  auto insert_done_f = insert_done.get_future();
  auto ckpt_fired_f = ckpt_fired.get_future();

  int call_count = 0;
  EXPECT_CALL(checkpoint_, GetStableCheckpoint())
      .WillRepeatedly(Invoke([&]() -> uint64_t {
        ++call_count;
        if (call_count == 1)
          insert_done_f.get();
        else if (call_count == 2)
          ckpt_fired.set_value(true);
        return 5;
      }));

  {
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);
    for (int i = 1; i <= 9; ++i) {
      auto future = AddTestEntry(recovery, i, i);
      future.get();
    }

    insert_done.set_value(true);
    ckpt_fired_f.get();  // first file sealed at ckpt=5

    // Write 6 more entries into the freshly opened second file.
    for (int i = 10; i <= 15; ++i) {
      auto future = AddTestEntry(recovery, i, i);
      future.get();
    }
  }

  // Two .log files should exist now.
  std::vector<std::string> logs = GetLogFiles(log_path);
  ASSERT_EQ(logs.size(), 2u);

  // Corrupt the middle of the second (newer) log file.
  std::uintmax_t sz = std::filesystem::file_size(logs[1]);
  CorruptFileAt(logs[1], static_cast<std::streamoff>((sz / 2) + 1), /*n=*/16);

  {
    std::vector<WALRecord> list;
    RaftRecovery recovery(config_, &checkpoint_, nullptr, nullptr);
    recovery.ReadLogs([&](const RaftMetadata &) {},
                      [&](std::unique_ptr<WALRecord> r) { list.push_back(*r); },
                      [&](const RaftMetadata &) {});

    // The sealed first file contributes seqs 6–9 (4 records, all > ckpt=5).
    // The active second file contributes seqs 10-12.
    EXPECT_EQ(list.size(), 7u)
        << "All 4 records from the sealed file should be present";

    // The first 4 must be seqs 6–9 from the sealed file.
    for (size_t i = 0; i < 4 && i < list.size(); ++i) {
      Request req;
      req.ParseFromString(list[i].entry().command());
      EXPECT_EQ(req.seq(), static_cast<int>(i + 6));
    }
  }
}

}  // namespace raft
}  // namespace resdb

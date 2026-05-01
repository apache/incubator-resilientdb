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

#pragma once

#include <thread>

#include "chain/storage/storage.h"
#include "platform/config/resdb_config.h"
#include "platform/consensus/checkpoint/checkpoint.h"
#include "platform/consensus/ordering/raft/framework/raft_checkpoint_manager.h"
#include "platform/consensus/ordering/raft/proto/proposal.pb.h"
#include "platform/consensus/recovery/recovery.h"
#include "platform/networkstrate/server_comm.h"
#include "platform/proto/resdb.pb.h"

namespace resdb {

namespace raft {

struct RaftMetadata {
  int64_t current_term = 0;
  int32_t voted_for = -1;
  uint64_t snapshot_last_index = 0;
  uint64_t snapshot_last_term = 0;
};

using CallbackType = std::function<void(std::unique_ptr<WALRecord>)>;

class RaftRecovery
    : public RecoveryBase<RaftRecovery, RaftMetadata, CallbackType> {
  friend class RecoveryBase<RaftRecovery, RaftMetadata, CallbackType>;

 public:
  RaftRecovery(const ResDBConfig& config, CheckPoint* checkpoint,
               Storage* storage, std::function<void(uint64_t)> on_checkpoint);
  ~RaftRecovery();

  RaftMetadata ReadMetadata();
  void Init();
  void WriteMetadata(int64_t current_term, int32_t voted_for,
                     uint64_t snapshot_last_index, uint64_t snapshot_last_term);
  void AddLogEntry(const Entry* entry, int64_t seq);
  void AddLogEntry(std::vector<Entry>& entries_to_add, int64_t seq);
  void TruncateLog(TruncationRecord truncate_beginning_at);

#ifdef RAFT_RECOVERY_TEST_MODE
  std::string GetMetadataFilePath() { return meta_file_path_; }

  std::string GetFilePath() { return file_path_; }
#endif

 private:
  void OpenMetadataFile();
  void WriteSystemInfo();
  std::vector<std::unique_ptr<WALRecord>> ParseDataListItem(
      std::vector<std::string>& data_list);
  void WriteLog(const WALRecord& record);

  void PerformCallback(
      std::vector<std::unique_ptr<WALRecord>>& request_list,
      std::function<void(std::unique_ptr<WALRecord> record)> call_back,
      int64_t ckpt);

  void HandleSystemInfo(
      int /*fd*/, std::function<void(const RaftMetadata&)> system_callback);

  int metadata_fd_;
  std::string meta_file_path_;
  RaftMetadata metadata_;
};

}  // namespace raft
}  // namespace resdb

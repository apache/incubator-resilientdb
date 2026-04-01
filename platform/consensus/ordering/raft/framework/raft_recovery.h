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
#include "platform/consensus/execution/system_info.h"
#include "platform/networkstrate/server_comm.h"
#include "platform/proto/resdb.pb.h"
#include "platform/consensus/recovery/recovery.h"
#include "platform/proto/system_info_data.pb.h"
#include "platform/consensus/ordering/raft/proto/proposal.pb.h"

namespace resdb {

namespace raft {

struct RaftMetadata {
  int64_t current_term = 0;
  int32_t voted_for = -1;
};

class RaftRecovery : public RecoveryBase<RaftRecovery> {
  friend class RecoveryBase<RaftRecovery>;

 public:
  RaftRecovery(const ResDBConfig& config, CheckPoint* checkpoint,
               Storage* storage);
  ~RaftRecovery();

  RaftMetadata ReadMetadata();
  void Init();
  void WriteMetadata(int64_t current_term, int32_t voted_for);
  void AddLogEntry(const Entry* entry);
  void AddLogEntry(std::vector<Entry>& entries_to_add);

 private:
  void OpenMetadataFile();
  void WriteSystemInfo();
  std::vector<std::unique_ptr<Entry>> ParseDataListItem(
      std::vector<std::string>& data_list);
  void WriteLog(const Entry* entry);

  void PerformCallback(
      std::vector<std::unique_ptr<Entry>>& request_list,
      std::function<void(std::unique_ptr<Entry> entry)> call_back,
      int64_t ckpt);

  bool PerformSystemCallback(
      std::vector<std::string> data_list,
      std::function<void(const RaftMetadata&)> system_callback);

  int metadata_fd_;
  std::string meta_file_path_;
  RaftMetadata metadata_;
};

}  // namespace raft
}  // namespace resdb
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

#include <fcntl.h>
#include <glog/logging.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

#include "chain/storage/storage.h"
#include "common/utils/utils.h"
#include "platform/config/resdb_config.h"
#include "platform/consensus/checkpoint/checkpoint.h"
#include "platform/networkstrate/server_comm.h"
#include "platform/proto/resdb.pb.h"

namespace resdb {

template <typename TDerived, typename TSystemInfoData, typename TCallback>
class RecoveryBase {
 public:
  RecoveryBase(const ResDBConfig& config, CheckPoint* checkpoint,
               Storage* storage,
               std::function<void(uint64_t)> on_checkpoint = nullptr);
  ~RecoveryBase();

  void ReadLogs(
      std::function<void(const TSystemInfoData& data)> system_callback,
      TCallback call_back, std::function<void(int)> start_point);

  int64_t GetMaxSeq();
  int64_t GetMinSeq();

 protected:
  std::vector<std::pair<int64_t, std::string>> GetSortedRecoveryFiles(
      uint64_t need_min_seq, uint64_t need_max_seq);

  std::vector<std::string> ParseRawData(const std::string& data);

 private:
  auto ParseData(const std::string& data);

  void MayFlush();

  void Write(const char* data, size_t len);
  
  std::string GenerateFile(int64_t seq, int64_t min_seq, int64_t max_seq);

  void FinishFile(int64_t seq);

  void InsertCache(const Context& context, const Request& request);

 protected:
  void GetLastFile();
  void UpdateStableCheckPoint();
  void Flush();

  void AppendData(const std::string& data);
  bool Read(int fd, size_t len, char* data);
  std::pair<std::vector<std::pair<int64_t, std::string>>, int64_t> GetRecoveryFiles(int64_t ckpt);

  void SwitchFile(const std::string& path, TCallback call_back);
  void OpenFile(const std::string& path);

  void ReadLogsFromFiles(
      const std::string& path, int64_t ckpt, int file_idx,
      std::function<void(const TSystemInfoData& data)> system_callback,
      TCallback call_back);

  std::string file_path_;
  ResDBConfig config_;
  // Derived class must implement these
  auto ParseDataListItem(std::vector<std::string>& data_list);

  template <typename RequestList>
  void PerformCallback(RequestList& request_list, TCallback call_back);

  void WriteSystemInfo();

  CheckPoint* checkpoint_;
  std::thread ckpt_thread_;
  bool recovery_enabled_ = false;
  std::string buffer_;

  std::string base_file_path_;
  size_t buffer_size_ = 0;
  int fd_;
  std::mutex mutex_, data_mutex_;

  int64_t last_ckpt_;
  int64_t min_seq_, max_seq_;
  std::mutex ckpt_mutex_;
  std::atomic<bool> stop_;
  int recovery_ckpt_time_s_;
  Storage* storage_;
  std::function<void(uint64_t)> on_checkpoint_callback_;
};

#include "platform/consensus/recovery/recovery_impl.h"

}  // namespace resdb

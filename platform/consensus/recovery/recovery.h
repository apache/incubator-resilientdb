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
#include "platform/proto/system_info_data.pb.h"

namespace resdb {

class Recovery {
 public:
  Recovery(const ResDBConfig& config, CheckPoint* checkpoint,
           SystemInfo* system_info, Storage* storage);
  virtual ~Recovery();

  void Init();

  virtual void AddRequest(const Context* context, const Request* request);

  void ReadLogs(std::function<void(const SystemInfoData& data)> system_callback,
                std::function<void(std::unique_ptr<Context> context,
                                   std::unique_ptr<Request> request)>
                    call_back);

  int64_t GetMaxSeq();
  int64_t GetMinSeq();

 private:
  struct RecoveryData {
    std::unique_ptr<Context> context;
    std::unique_ptr<Request> request;
  };

  void WriteLog(const Context* context, const Request* request);
  void AppendData(const std::string& data);
  std::vector<std::unique_ptr<RecoveryData>> ParseData(const std::string& data);
  std::vector<std::string> ParseRawData(const std::string& data);
  void Flush();
  void MayFlush();

  void Write(const char* data, size_t len);
  bool Read(int fd, size_t len, char* data);

  std::string GenerateFile(int64_t seq, int64_t min_seq, int64_t max_seq);
  void GetLastFile();
  void WriteSystemInfo();

  void OpenFile(const std::string& path);
  void FinishFile(int64_t seq);
  void SwitchFile(const std::string& path);

  void UpdateStableCheckPoint();
  std::pair<std::vector<std::pair<int64_t, std::string>>, int64_t>
  GetRecoveryFiles();
  void ReadLogsFromFiles(
      const std::string& path, int64_t ckpt, int file_idx,
      std::function<void(const SystemInfoData& data)> system_callback,
      std::function<void(std::unique_ptr<Context> context,
                         std::unique_ptr<Request> request)>
          call_back);

 protected:
  ResDBConfig config_;
  CheckPoint* checkpoint_;
  std::thread ckpt_thread_;
  bool recovery_enabled_ = false;
  std::string buffer_;
  std::string file_path_, base_file_path_;
  size_t buffer_size_ = 0;
  int fd_;
  std::mutex mutex_;

  int64_t last_ckpt_;
  int64_t min_seq_, max_seq_;
  std::mutex ckpt_mutex_;
  std::atomic<bool> stop_;
  int recovery_ckpt_time_s_;
  SystemInfo* system_info_;
  Storage* storage_;
};

}  // namespace resdb

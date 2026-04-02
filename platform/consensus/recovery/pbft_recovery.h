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

#include "platform/consensus/execution/system_info.h"
#include "platform/consensus/recovery/recovery.h"
#include "platform/proto/system_info_data.pb.h"

namespace resdb {
using CallbackType =
    std::function<void(std::unique_ptr<Context>, std::unique_ptr<Request>)>;

class PBFTRecovery
    : public RecoveryBase<PBFTRecovery, SystemInfoData, CallbackType> {
  friend class RecoveryBase<PBFTRecovery, SystemInfoData, CallbackType>;

 public:
  PBFTRecovery(const ResDBConfig& config, CheckPoint* checkpoint,
               SystemInfo* system_info, Storage* storage);
  ~PBFTRecovery() = default;

  void AddRequest(const Context* context, const Request* request);

  std::map<uint64_t, std::vector<std::pair<std::unique_ptr<Context>,
                                           std::unique_ptr<Request>>>>
  GetDataFromRecoveryFiles(uint64_t need_min_seq, uint64_t need_max_seq);

  int GetData(const RecoveryRequest& request, RecoveryResponse& response);

 private:
  struct RecoveryData {
    std::unique_ptr<Context> context;
    std::unique_ptr<Request> request;
  };

  void Init();
  void WriteLog(const Context* context, const Request* request);
  void WriteSystemInfo();

  std::vector<std::unique_ptr<RecoveryData>> ParseDataListItem(
      std::vector<std::string>& data_list);

  void PerformCallback(std::vector<std::unique_ptr<RecoveryData>>& request_list,
                       CallbackType call_back, int64_t ckpt);

  bool PerformSystemCallback(
      std::vector<std::string> data_list,
      std::function<void(const SystemInfoData&)> system_callback);

  void HandleSystemInfo(
      int fd, std::function<void(const SystemInfoData&)> system_callback);

  SystemInfo* system_info_;
};

}  // namespace resdb

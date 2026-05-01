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

#include "platform/consensus/recovery/pbft_recovery.h"

#include <fcntl.h>
#include <glog/logging.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <iostream>

#include "common/utils/utils.h"

namespace resdb {

using CallbackType =
    std::function<void(std::unique_ptr<Context>, std::unique_ptr<Request>)>;

PBFTRecovery::PBFTRecovery(const ResDBConfig& config, CheckPoint* checkpoint,
                           SystemInfo* system_info, Storage* storage)
    : RecoveryBase<PBFTRecovery, SystemInfoData, CallbackType>(
          config, checkpoint, storage),
      system_info_(system_info) {
  Init();
}

void PBFTRecovery::Init() {
  if (recovery_enabled_ == false) {
    LOG(INFO) << "recovery is not enabled:" << recovery_enabled_;
    return;
  }

  LOG(ERROR) << " init";
  GetLastFile();

  CallbackType callback = [this](std::unique_ptr<Context> context,
                                 std::unique_ptr<Request> request) {
    min_seq_ = (min_seq_ == -1)
                   ? request->seq()
                   : std::min(min_seq_, static_cast<int64_t>(request->seq()));
    max_seq_ = std::max(max_seq_, static_cast<int64_t>(request->seq()));
  };

  SwitchFile(file_path_, callback);

  LOG(ERROR) << " init done";

  ckpt_thread_ = std::thread([this] { this->UpdateStableCheckPoint(); });
}

void PBFTRecovery::WriteSystemInfo() {
  int view = system_info_->GetCurrentView();
  int primary_id = system_info_->GetPrimaryId();
  LOG(ERROR) << "write system info:" << primary_id << " view:" << view;
  SystemInfoData data;
  data.set_view(view);
  data.set_primary_id(primary_id);

  std::string data_str;
  data.SerializeToString(&data_str);

  AppendData(data_str);
  Flush();
}

void PBFTRecovery::AddRequest(const Context* context, const Request* request) {
  if (recovery_enabled_ == false) {
    return;
  }
  switch (request->type()) {
    case Request::TYPE_PRE_PREPARE:
    case Request::TYPE_PREPARE:
    case Request::TYPE_COMMIT:
    case Request::TYPE_NEWVIEW:
      return WriteLog(context, request);
    default:
      break;
  }
}

void PBFTRecovery::WriteLog(const Context* context, const Request* request) {
  std::string data;
  if (request) {
    request->SerializeToString(&data);
  }

  std::string sig;
  if (context) {
    context->signature.SerializeToString(&sig);
  }

  std::unique_lock<std::mutex> lk(mutex_);
  min_seq_ = min_seq_ == -1
                 ? request->seq()
                 : std::min(min_seq_, static_cast<int64_t>(request->seq()));
  max_seq_ = std::max(max_seq_, static_cast<int64_t>(request->seq()));
  AppendData(data);
  AppendData(sig);

  Flush();
}

std::vector<std::unique_ptr<PBFTRecovery::RecoveryData>>
PBFTRecovery::ParseDataListItem(std::vector<std::string>& data_list) {
  std::vector<std::unique_ptr<RecoveryData>> request_list;

  for (size_t i = 0; i < data_list.size(); i += 2) {
    std::unique_ptr<RecoveryData> recovery_data =
        std::make_unique<RecoveryData>();
    recovery_data->request = std::make_unique<Request>();
    recovery_data->context = std::make_unique<Context>();

    if (!recovery_data->request->ParseFromString(data_list[i])) {
      LOG(ERROR) << "Parse from data fail";
      break;
    }

    if (!recovery_data->context->signature.ParseFromString(data_list[i + 1])) {
      LOG(ERROR) << "Parse from data fail";
      break;
    }

    request_list.push_back(std::move(recovery_data));
  }
  return request_list;
}

void PBFTRecovery::PerformCallback(
    std::vector<std::unique_ptr<RecoveryData>>& request_list,
    CallbackType call_back, int64_t ckpt) {
  uint64_t max_seq = 0;
  for (std::unique_ptr<RecoveryData>& recovery_data : request_list) {
    // LOG(ERROR)<<" ckpt :"<<ckpt<<" recovery data
    // seq:"<<recovery_data->request->seq()<<"
    // type:"<<recovery_data->request->type();
    if (ckpt < recovery_data->request->seq() ||
        recovery_data->request->type() == Request::TYPE_NEWVIEW) {
      recovery_data->request->set_is_recovery(true);
      max_seq = recovery_data->request->seq();
      call_back(std::move(recovery_data->context),
                std::move(recovery_data->request));
    }
  }

  LOG(ERROR) << " recovery max seq:" << max_seq;
}

bool PBFTRecovery::PerformSystemCallback(
    std::vector<std::string> data_list,
    std::function<void(const SystemInfoData&)> system_callback) {
  SystemInfoData info;
  if (data_list.empty() || !info.ParseFromString(data_list[0])) {
    return false;
  }
  LOG(ERROR) << "read system info:" << info.DebugString();
  system_callback(info);
  return true;
}

void PBFTRecovery::HandleSystemInfo(
    int fd, std::function<void(const SystemInfoData&)> system_callback) {
  size_t data_len = 0;
  Read(fd, sizeof(data_len), reinterpret_cast<char*>(&data_len));
  std::string data;
  char* buf = new char[data_len];
  if (!Read(fd, data_len, buf)) {
    LOG(ERROR) << "Read system info fail";
    return;
  }
  data = std::string(buf, data_len);
  delete buf;
  std::vector<std::string> data_list = ParseRawData(data);

  bool successful_callback = PerformSystemCallback(data_list, system_callback);

  if (!successful_callback) {
    LOG(ERROR) << "parse info fail:" << data.size();
  }
}

std::map<
    uint64_t,
    std::vector<std::pair<std::unique_ptr<Context>, std::unique_ptr<Request>>>>
PBFTRecovery::GetDataFromRecoveryFiles(uint64_t need_min_seq,
                                       uint64_t need_max_seq) {
  auto list = GetSortedRecoveryFiles(need_min_seq, need_max_seq);

  std::map<uint64_t, std::vector<std::pair<std::unique_ptr<Context>,
                                           std::unique_ptr<Request>>>>
      res;
  for (const auto& path : list) {
    CallbackType callback = [&](std::unique_ptr<Context> context,
                                std::unique_ptr<Request> request) {
      if (request->seq() >= need_min_seq && request->seq() <= need_max_seq) {
        LOG(ERROR) << "get data from recovery file seq:" << request->seq();
        res[request->seq()].push_back(
            std::make_pair(std::move(context), std::move(request)));
      }
    };

    ReadLogsFromFiles(
        path.second, need_min_seq - 1, 0, [&](const SystemInfoData& data) {},
        callback);
  }

  return res;
}

int PBFTRecovery::GetData(const RecoveryRequest& request,
                          RecoveryResponse& response) {
  auto res = GetDataFromRecoveryFiles(request.min_seq(), request.max_seq());

  for (const auto& it : res) {
    for (const auto& req : it.second) {
      *response.add_signature() = req.first->signature;
      *response.add_request() = *req.second;
    }
  }
  return 0;
}

}  // namespace resdb

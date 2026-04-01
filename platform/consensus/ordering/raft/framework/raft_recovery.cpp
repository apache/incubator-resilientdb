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

#include "platform/consensus/ordering/raft/framework/raft_recovery.h"

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
namespace raft {

using CallbackType = std::function<void(std::unique_ptr<Entry>)>;

RaftRecovery::RaftRecovery(const ResDBConfig& config, CheckPoint* checkpoint,
                           Storage* storage)
    : RecoveryBase<RaftRecovery>(config, checkpoint, storage) {
  Init();
}

void RaftRecovery::Init() {
  if (recovery_enabled_ == false) {
    LOG(INFO) << "recovery is not enabled:" << recovery_enabled_;
    return;
  }

  LOG(ERROR) << " init";
  GetLastFile();

  CallbackType callback = [this](std::unique_ptr<Entry> entry) {
    min_seq_ == -1 ? min_seq_ = entry->term()
                   : std::min(min_seq_, static_cast<int64_t>(entry->term()));
    max_seq_ = std::max(max_seq_, static_cast<int64_t>(entry->term()));
  };

  SwitchFile<RaftMetadata, CallbackType>(file_path_, callback);
  LOG(ERROR) << " init done";

  meta_file_path_ = std::filesystem::path(base_file_path_).parent_path() 
                    / "raft_metadata.dat";
  LOG(INFO) << "Meta file path: " << meta_file_path_;
  OpenMetadataFile();

  ckpt_thread_ = std::thread([this] { this->UpdateStableCheckPoint(); });
}

RaftRecovery::~RaftRecovery() {
  LOG(INFO) << "Debug at " << __FILE__ << ":" << __LINE__ << " in function "
            << __func__ << "\n";
  if (recovery_enabled_ == false) {
    return;
  }
  Flush();
  if (metadata_fd_ >= 0) {
    close(metadata_fd_);
  }
}

void RaftRecovery::OpenMetadataFile() {
  LOG(INFO) << "Debug at " << __FILE__ << ":" << __LINE__ << " in function "
            << __func__ << "\n";
  metadata_fd_ = open(meta_file_path_.c_str(), O_CREAT | O_RDWR, 0666);
  if (metadata_fd_ < 0) {
    LOG(ERROR) << "Failed to open metadata file: " << strerror(errno);
    return;
  }

  // Read existing metadata if it exists, otherwise defaults are used
  metadata_ = ReadMetadata();
  LOG(INFO) << "Opened metadata file: term: " << metadata_.current_term
            << " votedFor: " << metadata_.voted_for;
}

void RaftRecovery::WriteMetadata(int64_t current_term, int32_t voted_for) {
  if (metadata_fd_ < 0) {
    LOG(ERROR) << "Metadata file not open";
    return;
  }

  metadata_.current_term = current_term;
  metadata_.voted_for = voted_for;

  lseek(metadata_fd_, 0, SEEK_SET);
  write(metadata_fd_, &metadata_, sizeof(metadata_));
  fsync(metadata_fd_);

  LOG(INFO) << "Wrote metadata: term: " << current_term
            << " votedFor: " << voted_for;
  LOG(INFO) << "METADATA location: " << meta_file_path_;
}

RaftMetadata RaftRecovery::ReadMetadata() {
  LOG(INFO) << "Debug at " << __FILE__ << ":" << __LINE__ << " in function "
            << __func__ << "\n";
  RaftMetadata metadata;
  if (metadata_fd_ < 0) {
    LOG(ERROR) << "Metadata file not open";
    return metadata;
  }

  lseek(metadata_fd_, 0, SEEK_SET);
  int bytes = read(metadata_fd_, &metadata, sizeof(metadata));
  if (bytes != sizeof(metadata)) {
    LOG(INFO) << "No existing metadata, using defaults";
    return RaftMetadata{};
  }
  return metadata;
}

void RaftRecovery::WriteSystemInfo() {}

void RaftRecovery::AddLogEntry(const Entry* entry) {
  if (recovery_enabled_ == false) {
    return;
  }

  WriteLog(entry);
  Flush();
}

void RaftRecovery::AddLogEntry(std::vector<Entry>& entries_to_add) {
  if (recovery_enabled_ == false) {
    return;
  }
  for (const auto& entry : entries_to_add) {
    WriteLog(&entry);
  }
  Flush();
}

void RaftRecovery::WriteLog(const Entry* entry) {
  LOG(INFO) << "Debug at " << __FILE__ << ":" << __LINE__ << " in function "
            << __func__ << "\n";
  std::string data;
  if (entry) {
    entry->SerializeToString(&data);
  }

  std::unique_lock<std::mutex> lk(mutex_);
  min_seq_ = min_seq_ == -1
                 ? entry->term()
                 : std::min(min_seq_, static_cast<int64_t>(entry->term()));
  max_seq_ = std::max(max_seq_, static_cast<int64_t>(entry->term()));
  AppendData(data);
}

std::vector<std::unique_ptr<Entry>> RaftRecovery::ParseDataListItem(
    std::vector<std::string>& data_list) {
  std::vector<std::unique_ptr<Entry>> request_list;

  for (size_t i = 0; i < data_list.size(); i++) {
    std::unique_ptr<Entry> entry = std::make_unique<Entry>();

    if (!entry->ParseFromString(data_list[i])) {
      LOG(ERROR) << "Parse from data fail";
      break;
    }

    request_list.push_back(std::move(entry));
  }
  return request_list;
}

void RaftRecovery::PerformCallback(
    std::vector<std::unique_ptr<Entry>>& request_list, CallbackType call_back,
    int64_t ckpt) {
  uint64_t max_seq = 0;
  for (std::unique_ptr<Entry>& entry : request_list) {
    // LOG(ERROR)<<" ckpt :"<<ckpt<<" recovery data
    // seq:"<<recovery_data->request->seq()<<"
    // type:"<<recovery_data->request->type();
    if (ckpt < entry->term()) {
      max_seq = entry->term();
      call_back(std::move(entry));
    }
  }

  LOG(ERROR) << " recovery max seq:" << max_seq;
}

bool RaftRecovery::PerformSystemCallback(
    std::vector<std::string> data_list,
    std::function<void(const RaftMetadata&)> system_callback) {
  RaftMetadata info = ReadMetadata();
  system_callback(info);
  return true;
}

}  // namespace raft

template class RecoveryBase<raft::RaftRecovery>;

template void RecoveryBase<raft::RaftRecovery>::ReadLogs<raft::RaftMetadata,
                                                         raft::CallbackType>(
    std::function<void(const raft::RaftMetadata&)>, raft::CallbackType,
    std::function<void(int)>);

template void RecoveryBase<raft::RaftRecovery>::SwitchFile<
    raft::RaftMetadata, raft::CallbackType>(const std::string&,
                                            raft::CallbackType);

template void RecoveryBase<raft::RaftRecovery>::ReadLogsFromFiles<
    raft::RaftMetadata, raft::CallbackType>(
    const std::string&, int64_t, int,
    std::function<void(const raft::RaftMetadata&)>, raft::CallbackType);
}  // namespace resdb

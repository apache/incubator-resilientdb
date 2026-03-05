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

RaftRecovery::RaftRecovery(const ResDBConfig& config, CheckPoint* checkpoint, SystemInfo* system_info, Storage* storage)
  : Recovery(config, checkpoint, system_info, storage) {
  LOG(INFO) << "Raft Recovery constructor";
  Init();
};

void RaftRecovery::Init() {
  // Recovery::Init();
  
  meta_file_path_ = std::filesystem::path(base_file_path_).parent_path() 
                    / "raft_metadata.dat";
  LOG(INFO) << "Meta file path: " << meta_file_path_;
  OpenMetadataFile();
}

RaftRecovery::~RaftRecovery() {
  if (recovery_enabled_ == false) {
    return;
  }
  if (metadata_fd_ >= 0) {
    close(metadata_fd_);
  }
}

void RaftRecovery::OpenMetadataFile() {
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

void RaftRecovery::SwitchFile(const std::string& file_path) {
  std::unique_lock<std::mutex> lk(mutex_);

  min_seq_ = -1;
  max_seq_ = -1;

  ReadLogsFromFiles(
      file_path, 0, 0, [&](const RaftMetadata& data) {},
      [&](std::unique_ptr<Request> request) {
        min_seq_ == -1
            ? min_seq_ = request->seq()
            : std::min(min_seq_, static_cast<int64_t>(request->seq()));
        max_seq_ = std::max(max_seq_, static_cast<int64_t>(request->seq()));
      });

  OpenFile(file_path);
  LOG(INFO) << "switch to file:" << file_path << " seq:"
            << "[" << min_seq_ << "," << max_seq_ << "]";
}

void RaftRecovery::AddLogEntry(const Entry* entry) {
  if (recovery_enabled_ == false) {
    return;
  }
  return WriteLog(entry);
}

void RaftRecovery::WriteLog(const Entry* entry) {
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

  Flush();
}

std::vector<std::unique_ptr<RaftRecovery::RecoveryData>> RaftRecovery::ParseData(const std::string& data) {
  std::vector<std::unique_ptr<RecoveryData>> request_list;

  std::vector<std::string> data_list;
  int pos = 0;
  while (pos < data.size()) {
    size_t len;
    memcpy(&len, data.c_str() + pos, sizeof(len));
    pos += sizeof(len);

    std::string item = data.substr(pos, len);
    pos += len;
    data_list.push_back(item);
  }

  for (size_t i = 0; i < data_list.size(); i += 1) {
    std::unique_ptr<RecoveryData> recovery_data =
        std::make_unique<RecoveryData>();
    recovery_data->request = std::make_unique<Request>();

    if (!recovery_data->request->ParseFromString(data_list[i])) {
      LOG(ERROR) << "Parse from data fail";
      break;
    }

    request_list.push_back(std::move(recovery_data));
  }
  return request_list;
}

void RaftRecovery::ReadLogs(
    std::function<void(const RaftMetadata& data)> system_callback,
    std::function<void(std::unique_ptr<Request> request)>
        call_back,
    std::function<void(int)> set_start_point) {
  if (recovery_enabled_ == false) {
    return;
  }
  assert(storage_);
  int64_t storage_ckpt = storage_->GetLastCheckpoint();
  LOG(ERROR) << " storage ckpt:" << storage_ckpt;
  std::unique_lock<std::mutex> lk(mutex_);

  system_callback(ReadMetadata());
  auto recovery_files_pair = GetRecoveryFiles(storage_ckpt);
  int64_t ckpt = recovery_files_pair.second;
  if (set_start_point) {
    set_start_point(ckpt);
  }
  int idx = 0;
  for (auto path : recovery_files_pair.first) {
    ReadLogsFromFiles(path.second, ckpt, idx++, system_callback, call_back);
  }
}

void RaftRecovery::ReadLogsFromFiles(
    const std::string& path, int64_t ckpt, int file_idx,
    std::function<void(const RaftMetadata& data)> system_callback,
    std::function<void(std::unique_ptr<Request> request)>
        call_back) {
  int fd = open(path.c_str(), O_CREAT | O_RDONLY, 0666);
  if (fd < 0) {
    LOG(ERROR) << " open file fail:" << path;
  }
  LOG(INFO) << "read logs:" << path << " pos:" << lseek(fd, 0, SEEK_CUR);
  assert(fd >= 0);

  size_t data_len = 0;
  std::vector<std::unique_ptr<RecoveryData>> request_list;

  while (Read(fd, sizeof(data_len), reinterpret_cast<char*>(&data_len))) {
    std::string data;
    char* buf = new char[data_len];
    if (!Read(fd, data_len, buf)) {
      LOG(ERROR) << "Read data log fail";
      break;
    }
    data = std::string(buf, data_len);
    delete buf;

    std::vector<std::unique_ptr<RecoveryData>> list = ParseData(data);
    if (list.size() == 0) {
      request_list.clear();
      break;
    }
    for (auto& l : list) {
      request_list.push_back(std::move(l));
    }
  }
  if (request_list.size() == 0) {
    ftruncate(fd, 0);
  }
  uint64_t max_seq = 0;
  for (std::unique_ptr<RecoveryData>& recovery_data : request_list) {
    // LOG(ERROR)<<" ckpt :"<<ckpt<<" recovery data
    // seq:"<<recovery_data->request->seq()<<"
    // type:"<<recovery_data->request->type();
    if (ckpt < recovery_data->request->seq() ||
        recovery_data->request->type() == Request::TYPE_NEWVIEW) {
      recovery_data->request->set_is_recovery(true);
      max_seq = recovery_data->request->seq();
      call_back(std::move(recovery_data->request));
    }
  }

  LOG(ERROR) << "read log from files:" << path << " done"
             << " recovery max seq:" << max_seq;

  close(fd);
}

std::map<uint64_t, std::vector<std::unique_ptr<Request>>> RaftRecovery::GetDataFromRecoveryFiles(uint64_t need_min_seq, uint64_t need_max_seq) {
  std::string dir = std::filesystem::path(file_path_).parent_path();

  std::vector<std::pair<int64_t, std::string>> list;
  std::vector<std::pair<int64_t, std::string>> e_list;

  for (const auto& entry : std::filesystem::directory_iterator(dir)) {
    std::string dir = std::filesystem::path(entry.path()).parent_path();
    std::string file_name = std::filesystem::path(entry.path()).stem();
    std::string ext = std::filesystem::path(entry.path()).extension();
    if (ext != ".log") continue;
    int pos = file_name.rfind("_");

    int max_seq_pos = file_name.rfind("_", pos - 1);
    int64_t max_seq =
        std::stoll(file_name.substr(max_seq_pos + 1, pos - max_seq_pos - 1));

    int min_seq_pos = file_name.rfind("_", max_seq_pos - 1);
    int64_t min_seq = std::stoll(
        file_name.substr(min_seq_pos + 1, max_seq_pos - min_seq_pos - 1));

    int time_pos = file_name.rfind("_", min_seq_pos - 1);
    int64_t time =
        std::stoll(file_name.substr(time_pos + 1, min_seq_pos - time_pos - 1));

    // LOG(ERROR)<<" min seq:"<<min_seq << " max seq:"<<max_seq<<"
    // need:"<<need_min_seq<<" "<<need_max_seq;
    if (min_seq == -1) {
      e_list.push_back(std::make_pair(time, entry.path()));
    } else if (max_seq < need_min_seq || min_seq > need_max_seq) {
      continue;
    }
    // LOG(ERROR)<<" get min seq:"<<min_seq << " max seq:"<<max_seq<<"
    // need:"<<need_min_seq<<" "<<need_max_seq;
    list.push_back(std::make_pair(time, entry.path()));
  }

  sort(e_list.begin(), e_list.end());
  list.push_back(e_list.back());
  sort(list.begin(), list.end());

  std::map<uint64_t, std::vector<std::unique_ptr<Request>>> res;
  for (const auto& path : list) {
    ReadLogsFromFiles(
        path.second, need_min_seq - 1, 0, [&](const RaftMetadata& data) {},
        [&](std::unique_ptr<Request> request) {
          // LOG(ERROR) << "check get data from recovery file seq:"
          //           << request->seq();
          if (request->seq() >= need_min_seq &&
              request->seq() <= need_max_seq) {
            LOG(ERROR) << "get data from recovery file seq:" << request->seq();
            res[request->seq()].push_back(std::move(request));
          }
        });
  }

  return res;
}

}  // namespace raft
}  // namespace resdb

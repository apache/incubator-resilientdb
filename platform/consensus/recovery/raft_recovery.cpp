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

using CallbackType = std::function<void(std::unique_ptr<WALRecord>)>;

RaftRecovery::RaftRecovery(const ResDBConfig& config, CheckPoint* checkpoint,
                           Storage* storage)
    : RecoveryBase<RaftRecovery, RaftMetadata, CallbackType>(config, checkpoint,
                                                             storage) {
  Init();
}

void RaftRecovery::Init() {
  if (recovery_enabled_ == false) {
    LOG(INFO) << "recovery is not enabled:" << recovery_enabled_;
    return;
  }

  wal_seq_ = 0;

  LOG(ERROR) << " init";
  GetLastFile();

  meta_file_path_ = std::filesystem::path(base_file_path_).parent_path() /
                    "raft_metadata.dat";
  LOG(INFO) << "Meta file path: " << meta_file_path_;
  OpenMetadataFile();

  CallbackType callback = [this](std::unique_ptr<WALRecord> record) {
    min_seq_ == -1 ? min_seq_ = record->seq()
                   : std::min(min_seq_, static_cast<int64_t>(record->seq()));
    max_seq_ = std::max(max_seq_, static_cast<int64_t>(record->seq()));
  };

  SwitchFile(file_path_, callback);
  LOG(ERROR) << " init done";

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

  close(metadata_fd_);
  metadata_fd_ = -1;

  LOG(INFO) << "Opened metadata file: term: " << metadata_.current_term
            << " votedFor: " << metadata_.voted_for;
}

void RaftRecovery::WriteMetadata(int64_t current_term, int32_t voted_for) {
  if (recovery_enabled_ == false) {
    return;
  }

  std::string tmp_path = meta_file_path_ + ".tmp";
  LOG(INFO) << "tmp_path = [" << tmp_path << "]";
  LOG(INFO) << "meta_file_path_ = [" << meta_file_path_ << "]";

  int temp_fd = open(tmp_path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0666);
  if (temp_fd < 0) {
    LOG(ERROR) << "Failed to open tmp metadata file: " << strerror(errno);
    return;
  }

  RaftMetadata new_metadata;
  new_metadata.current_term = current_term;
  new_metadata.voted_for = voted_for;

  ssize_t bytes_written = write(temp_fd, &new_metadata, sizeof(new_metadata));
  if (bytes_written != static_cast<ssize_t>(sizeof(new_metadata))) {
    LOG(ERROR) << "Failed to write metadata (wrote " << bytes_written << " of "
               << sizeof(new_metadata) << " bytes): " << strerror(errno);
    close(temp_fd);
    unlink(tmp_path.c_str());
    return;
  }

  if (fsync(temp_fd) < 0) {
    LOG(ERROR) << "Failed to fsync tmp metadata file: " << strerror(errno);
    close(temp_fd);
    unlink(tmp_path.c_str());
    return;
  }
  close(temp_fd);

  if (rename(tmp_path.c_str(), meta_file_path_.c_str()) < 0) {
    LOG(ERROR) << "Failed to rename tmp metadata file: " << strerror(errno);
    unlink(tmp_path.c_str());
    return;
  }

  // Only fsync and close the dir if it opens properly
  std::string dir_path = std::filesystem::path(meta_file_path_).parent_path().string();
  int dir_fd = open(dir_path.c_str(), O_RDONLY);
  if (dir_fd < 0) {
    LOG(ERROR) << "Failed to open directory for fsync: " << strerror(errno);
  } else {
    if (fsync(dir_fd) < 0) {
      LOG(ERROR) << "Failed to fsync directory: " << strerror(errno);
    }
    close(dir_fd);
  }

  metadata_ = new_metadata;

  LOG(INFO) << "Wrote metadata: term: " << current_term
            << " votedFor: " << voted_for;
  LOG(INFO) << "METADATA location: " << meta_file_path_;
}

RaftMetadata RaftRecovery::ReadMetadata() {
  if (recovery_enabled_ == false) {
    return RaftMetadata{};
  }

  RaftMetadata metadata;
  if (metadata_fd_ < 0) {
    LOG(ERROR) << "Metadata file either never opened or already closed "
                  "(meaning ReadMetadata() has been called before)";
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

  std::unique_lock<std::mutex> lk(mutex_);
  WALRecord record;
  *record.mutable_entry() = *entry;
  record.set_seq(++wal_seq_);
  WriteLog(record);
  Flush();
}

void RaftRecovery::AddLogEntry(std::vector<Entry>& entries_to_add) {
  if (recovery_enabled_ == false || entries_to_add.size() == 0) {
    return;
  }

  std::unique_lock<std::mutex> lk(mutex_);
  for (const auto& entry : entries_to_add) {
    LOG(INFO) << "Debug at " << __FILE__ << ":" << __LINE__ << " in function " << __func__ << "\n";
    WALRecord record;
    *record.mutable_entry() = entry;
    record.set_seq(++wal_seq_);
    WriteLog(record);
  }
  Flush();
}

void RaftRecovery::TruncateLog(TruncationRecord truncate_beginning_at) {
  if (recovery_enabled_ == false) {
    return;
  }

  std::unique_lock<std::mutex> lk(mutex_);

  WALRecord record;
  record.set_seq(++wal_seq_);
  *record.mutable_truncation() = std::move(truncate_beginning_at);

  WriteLog(record);
  Flush();
}

void RaftRecovery::WriteLog(const WALRecord& record) {
  LOG(INFO) << "Debug at " << __FILE__ << ":" << __LINE__ << " in function "
            << __func__ << "\n";
  std::string data;
  
  record.SerializeToString(&data);
  

  switch (record.payload_case()) {
    case WALRecord::kEntry:
      min_seq_ = min_seq_ == -1
                  ? record.seq()
                  : std::min(min_seq_, static_cast<int64_t>(record.seq()));
      max_seq_ = std::max(max_seq_, static_cast<int64_t>(record.seq()));
      break;
    case WALRecord::kTruncation:
      max_seq_ = record.seq();
      break;
    case WALRecord::PAYLOAD_NOT_SET:
      assert(false && "WALRecord does not contain Truncation or Entry");
      break;
  }

  AppendData(data);
}

std::vector<std::unique_ptr<WALRecord>> RaftRecovery::ParseDataListItem(
    std::vector<std::string>& data_list) {
  std::vector<std::unique_ptr<WALRecord>> record_list;

  for (size_t i = 0; i < data_list.size(); i++) {
    std::unique_ptr<WALRecord> record = std::make_unique<WALRecord>();

    if (!record->ParseFromString(data_list[i])) {
      LOG(ERROR) << "Parse from data fail";
      break;
    }

    record_list.push_back(std::move(record));
  }
  return record_list;
}

void RaftRecovery::PerformCallback(
    std::vector<std::unique_ptr<WALRecord>>& record_list, CallbackType call_back,
    int64_t ckpt) {
  uint64_t max_seq = 0;
  for (std::unique_ptr<WALRecord>& record : record_list) {
    if (ckpt < record->seq()) {
      max_seq = record->seq();
      call_back(std::move(record));
    }
  }

  LOG(ERROR) << " recovery max seq:" << max_seq;
}

void RaftRecovery::HandleSystemInfo(
    int /*fd*/, std::function<void(const RaftMetadata&)> system_callback) {
  RaftMetadata info = ReadMetadata();
  LOG(ERROR) << " info.voted_for: " << info.voted_for << "\ninfo.current_term "
             << info.current_term;
  system_callback(info);
}

}  // namespace raft

}  // namespace resdb

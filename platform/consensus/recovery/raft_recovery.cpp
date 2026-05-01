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
                           Storage* storage,
                           std::function<void(uint64_t)> on_checkpoint)
    : RecoveryBase<RaftRecovery, RaftMetadata, CallbackType>(
          config, checkpoint, storage, on_checkpoint) {
  Init();
}

void RaftRecovery::Init() {
  if (recovery_enabled_ == false) {
    LOG(INFO) << "recovery is not enabled:" << recovery_enabled_;
    return;
  }

  LOG(ERROR) << " init";
  GetLastFile();

  meta_file_path_ = std::filesystem::path(base_file_path_).parent_path() /
                    "raft_metadata.dat";
  LOG(INFO) << "Meta file path: " << meta_file_path_;
  OpenMetadataFile();

  CallbackType callback = [this](std::unique_ptr<WALRecord> record) {
    min_seq_ = min_seq_ == -1
                   ? record->seq()
                   : std::min(min_seq_, static_cast<int64_t>(record->seq()));
    max_seq_ = std::max(max_seq_, static_cast<int64_t>(record->seq()));
  };

  SwitchFile(file_path_, callback);
  LOG(ERROR) << " init done";

  ckpt_thread_ = std::thread([this] { this->UpdateStableCheckPoint(); });
}

RaftRecovery::~RaftRecovery() {
  if (recovery_enabled_ == false) {
    return;
  }
  Flush();
  if (metadata_fd_ >= 0) {
    close(metadata_fd_);
  }
}

void RaftRecovery::OpenMetadataFile() {
  LOG(INFO) << "Opening Metadata File";
  metadata_fd_ = open(meta_file_path_.c_str(), O_CREAT | O_RDWR, 0666);
  if (metadata_fd_ < 0) {
    LOG(ERROR) << "Failed to open metadata file: " << strerror(errno);
    return;
  }
}

void RaftRecovery::WriteMetadata(int64_t current_term, int32_t voted_for,
                                 uint64_t snapshot_last_index,
                                 uint64_t snapshot_last_term) {
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
  new_metadata.snapshot_last_index = snapshot_last_index;
  new_metadata.snapshot_last_term = snapshot_last_term;

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
            << " votedFor: " << voted_for
            << " snapshot last index: " << snapshot_last_index
            << " snapshot last term: " << snapshot_last_term;
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

  LOG(INFO) << "Read metadata file: term: " << metadata.current_term
            << " votedFor: " << metadata.voted_for
            << " snapshot_last_index: " << metadata.snapshot_last_index
            << " snapshot_last_term: " << metadata.snapshot_last_term;
  return metadata;
}

void RaftRecovery::WriteSystemInfo() {}

void RaftRecovery::AddLogEntry(const Entry* entry, int64_t seq) {
  if (recovery_enabled_ == false) {
    return;
  }

  std::unique_lock<std::mutex> lk(mutex_);
  WALRecord record;
  *record.mutable_entry() = *entry;
  record.set_seq(seq);
  WriteLog(record);
  Flush();
}

void RaftRecovery::AddLogEntry(std::vector<Entry>& entries_to_add,
                               int64_t seq) {
  if (recovery_enabled_ == false || entries_to_add.size() == 0) {
    return;
  }

  std::unique_lock<std::mutex> lk(mutex_);
  for (const auto& entry : entries_to_add) {
    WALRecord record;
    *record.mutable_entry() = entry;
    record.set_seq(seq++);
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
  record.set_seq(truncate_beginning_at.truncate_from_index() - 1);
  *record.mutable_truncation() = std::move(truncate_beginning_at);

  WriteLog(record);
  Flush();
}

void RaftRecovery::WriteLog(const WALRecord& record) {
  std::string data;
  
  record.SerializeToString(&data);
  

  switch (record.payload_case()) {
    case WALRecord::kEntry: {
      min_seq_ = min_seq_ == -1
                  ? record.seq()
                  : std::min(min_seq_, static_cast<int64_t>(record.seq()));
      max_seq_ = std::max(max_seq_, static_cast<int64_t>(record.seq()));
      break;
    }
    case WALRecord::kTruncation: {
      int64_t keep_up_to = static_cast<int64_t>(record.seq());
      if (max_seq_ > keep_up_to) {
        max_seq_ = keep_up_to;
      }
      // If we truncate everything, reset min and max seq
      if (max_seq_ <= last_ckpt_) {
        min_seq_ = -1;
        max_seq_ = -1;
      } else {
        min_seq_ =
            (min_seq_ == -1) ? keep_up_to : std::min(min_seq_, keep_up_to);
      }
      break;
    }
    case WALRecord::PAYLOAD_NOT_SET: {
      assert(false && "WALRecord does not contain Truncation or Entry");
      break;
    }
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
    // Only replay entries that are after the latest checkpoint.
    // Since truncation records store the seq of the last index remaining in the
    // log, it could be equal to the ckpt, meaning that everything since the
    // checkpoint is to be truncated.
    if (ckpt < record->seq() ||
        (ckpt == record->seq() &&
         record->payload_case() == WALRecord::kTruncation)) {
      max_seq = record->seq();
      call_back(std::move(record));
    }
  }

  LOG(ERROR) << " recovery max seq:" << max_seq;
}

void RaftRecovery::HandleSystemInfo(
    int /*fd*/, std::function<void(const RaftMetadata&)> system_callback) {
  metadata_ = ReadMetadata();
  LOG(ERROR) << " metadata_.voted_for: " << metadata_.voted_for
             << "\nmetadata_.current_term " << metadata_.current_term;
  system_callback(metadata_);
}

}  // namespace raft

}  // namespace resdb

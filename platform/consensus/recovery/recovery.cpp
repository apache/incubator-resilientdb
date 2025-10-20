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

#include "platform/consensus/recovery/recovery.h"

#include <fcntl.h>
#include <glog/logging.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstddef>
#include <filesystem>

#include "common/utils/utils.h"

namespace resdb {

Recovery::Recovery(const ResDBConfig& config, CheckPoint* checkpoint,
                   SystemInfo* system_info, Storage* storage)
    : config_(config),
      checkpoint_(checkpoint),
      system_info_(system_info),
      storage_(storage) {
  recovery_enabled_ = config_.GetConfigData().recovery_enabled();
  file_path_ = config_.GetConfigData().recovery_path();
  if (file_path_.empty()) {
    file_path_ = "./wal_log/log";
  }
  base_file_path_ = file_path_;

  if (config_.GetPublicKeyCertificateInfo()
          .public_key()
          .public_key_info()
          .type() == CertificateKeyInfo::CLIENT) {
    recovery_enabled_ = false;
  }

  if (recovery_enabled_ == false) {
    return;
  }

  buffer_size_ = config_.GetConfigData().recovery_buffer_size();
  if (buffer_size_ == 0) {
    buffer_size_ = 1024;
  }

  LOG(INFO) << "file path:" << file_path_
            << " dir:" << std::filesystem::path(file_path_).parent_path();

  recovery_ckpt_time_s_ = config_.GetConfigData().recovery_ckpt_time_s();
  if (recovery_ckpt_time_s_ == 0) {
    recovery_ckpt_time_s_ = 60;
  }

  int ret =
      mkdir(std::filesystem::path(file_path_).parent_path().c_str(), 0777);
  if (ret) {
    LOG(INFO) << "mkdir fail:" << ret << " error:" << strerror(errno);
  }

  fd_ = -1;
  stop_ = false;
  Init();
}

void Recovery::Init() {
  GetLastFile();
  SwitchFile(file_path_);

  ckpt_thread_ = std::thread(&Recovery::UpdateStableCheckPoint, this);
}

Recovery::~Recovery() {
  if (recovery_enabled_ == false) {
    return;
  }
  Flush();
  close(fd_);
  stop_ = true;
  if (ckpt_thread_.joinable()) {
    ckpt_thread_.join();
  }
}

int64_t Recovery::GetMaxSeq() { return max_seq_; }

int64_t Recovery::GetMinSeq() { return min_seq_; }

void Recovery::UpdateStableCheckPoint() {
  if (checkpoint_ == nullptr) {
    return;
  }
  while (!stop_) {
    int64_t latest_ckpt = checkpoint_->GetStableCheckpoint();
    LOG(ERROR) << "get stable ckpt:" << latest_ckpt;
    if (last_ckpt_ == latest_ckpt) {
      sleep(recovery_ckpt_time_s_);
      continue;
    }
    last_ckpt_ = latest_ckpt;
    FinishFile(latest_ckpt);
  }
}

void Recovery::GetLastFile() {
  std::string dir = std::filesystem::path(file_path_).parent_path();
  last_ckpt_ = -1;
  int m_time_s = 0;
  for (const auto& entry : std::filesystem::directory_iterator(dir)) {
    std::string dir = std::filesystem::path(entry.path()).parent_path();
    std::string file_name = std::filesystem::path(entry.path()).stem();
    std::string ext = std::filesystem::path(entry.path()).extension();
    if (ext != ".log") continue;
    int pos = file_name.rfind("_");
    int64_t ckpt = std::stoll(file_name.substr(pos + 1));
    int max_seq_pos = file_name.rfind("_", pos - 1);
    int min_seq_pos = file_name.rfind("_", max_seq_pos - 1);
    int time_pos = file_name.rfind("_", min_seq_pos - 1);

    int64_t min_seq = std::stoll(
        file_name.substr(min_seq_pos + 1, max_seq_pos - min_seq_pos - 1));

    int64_t time_s =
        std::stoll(file_name.substr(time_pos + 1, min_seq_pos - time_pos - 1));
    if (min_seq == -1) {
      if (last_ckpt_ == -1 || m_time_s < time_s) {
        file_path_ = entry.path();
        last_ckpt_ = ckpt;
        m_time_s = time_s;
        LOG(ERROR) << "get last path:" << file_name << " min:" << min_seq;
      }
    }
  }
  if (last_ckpt_ == -1) {
    last_ckpt_ = 0;
    file_path_ = GenerateFile(last_ckpt_, -1, -1);
  }
}

std::string Recovery::GenerateFile(int64_t seq, int64_t min_seq,
                                   int64_t max_seq) {
  std::string dir = std::filesystem::path(file_path_).parent_path();
  std::string file_name = std::filesystem::path(base_file_path_).stem();
  int64_t time = GetCurrentTime();
  file_name = file_name + "_" + std::to_string(time) + "_" +
              std::to_string(min_seq) + "_" + std::to_string(max_seq) + "_" +
              std::to_string(seq);
  std::string ext = std::filesystem::path(base_file_path_).extension();
  if (ext == "") ext = "log";
  return dir + "/" + file_name + "." + ext;
}

void Recovery::FinishFile(int64_t seq) {
  std::unique_lock<std::mutex> lk(mutex_);
  Flush();
  if (storage_) {
    if (!storage_->Flush()) {
      return;
    }
  }
  std::string new_file_path = GenerateFile(seq, min_seq_, max_seq_);
  close(fd_);

  min_seq_ = -1;
  max_seq_ = -1;

  std::rename(file_path_.c_str(), new_file_path.c_str());

  LOG(INFO) << "rename:" << file_path_ << " to:" << new_file_path;
  std::string next_file_path = GenerateFile(seq, -1, -1);
  file_path_ = next_file_path;

  OpenFile(file_path_);
}

void Recovery::SwitchFile(const std::string& file_path) {
  std::unique_lock<std::mutex> lk(mutex_);

  min_seq_ = -1;
  max_seq_ = -1;

  ReadLogsFromFiles(
      file_path, 0, 0, [&](const SystemInfoData& data) {},
      [&](std::unique_ptr<Context> context, std::unique_ptr<Request> request) {
        min_seq_ == -1
            ? min_seq_ = request->seq()
            : std::min(min_seq_, static_cast<int64_t>(request->seq()));
        max_seq_ = std::max(max_seq_, static_cast<int64_t>(request->seq()));
      });

  OpenFile(file_path);
  LOG(INFO) << "switch to file:" << file_path << " seq:"
            << "[" << min_seq_ << "," << max_seq_ << "]";
}

void Recovery::OpenFile(const std::string& path) {
  if (fd_ >= 0) {
    close(fd_);
  }
  fd_ = open(path.c_str(), O_CREAT | O_WRONLY, 0666);
  if (fd_ < 0) {
    LOG(ERROR) << "open file fail:" << path << " error:" << strerror(errno);
  }

  int pos = lseek(fd_, 0, SEEK_END);
  LOG(INFO) << "file path:" << path << " len:" << pos << " fd:" << fd_;

  if (pos == 0) {
    WriteSystemInfo();
  }

  lseek(fd_, 0, SEEK_END);
  LOG(INFO) << "open file:" << path << " pos:" << lseek(fd_, 0, SEEK_CUR)
            << " fd:" << fd_;
  assert(fd_ >= 0);
}

void Recovery::WriteSystemInfo() {
  int view = system_info_->GetCurrentView();
  int primary_id = system_info_->GetPrimaryId();
  SystemInfoData data;
  data.set_view(view);
  data.set_primary_id(primary_id);

  std::string data_str;
  data.SerializeToString(&data_str);

  AppendData(data_str);
  Flush();
}

void Recovery::AddRequest(const Context* context, const Request* request) {
  if (recovery_enabled_ == false) {
    return;
  }
  switch (request->type()) {
    case Request::TYPE_PRE_PREPARE:
    case Request::TYPE_PREPARE:
    case Request::TYPE_COMMIT:
    case Request::TYPE_CHECKPOINT:
    case Request::TYPE_NEWVIEW:
      return WriteLog(context, request);
    default:
      break;
  }
}

uint64_t Recovery::get_latest_executed_seq_recov(){
  return checkpoint_->GetLastExecutedSeq();
}

void Recovery::WriteLog(const Context* context, const Request* request) {
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
  uint64_t latest_executed_seq = get_latest_executed_seq_recov();
  AppendData(std::to_string(latest_executed_seq));

  Flush();
}

void Recovery::AppendData(const std::string& data) {
  size_t len = data.size();
  buffer_.append(reinterpret_cast<const char*>(&len), sizeof(len));
  buffer_.append(data);
}

std::vector<std::unique_ptr<Recovery::RecoveryData>> Recovery::ParseData(
    const std::string& data) {
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

std::vector<std::string> Recovery::ParseRawData(const std::string& data) {
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
  return data_list;
}

void Recovery::MayFlush() {
  if (buffer_.size() > buffer_size_) {
    Flush();
  }
}

void Recovery::Flush() {
  size_t len = buffer_.size();
  if (len == 0) {
    return;
  }

  Write(reinterpret_cast<const char*>(&len), sizeof(len));
  Write(reinterpret_cast<const char*>(buffer_.c_str()), len);
  buffer_.clear();
  fsync(fd_);
}

void Recovery::Write(const char* data, size_t len) {
  int pos = 0;
  while (len > 0) {
    int write_len = write(fd_, data + pos, len);
    len -= write_len;
    pos += write_len;
  }
}

bool Recovery::Read(int fd, size_t len, char* data) {
  int pos = 0;
  while (len > 0) {
    int read_len = read(fd, data + pos, len);
    if (read_len <= 0) {
      return false;
    }
    len -= read_len;
    pos += read_len;
  }
  return true;
}

std::pair<std::vector<std::pair<int64_t, std::string>>, int64_t>
Recovery::GetRecoveryFiles() {
  std::string dir = std::filesystem::path(file_path_).parent_path();
  int64_t last_ckpt = 0;
  for (const auto& entry : std::filesystem::directory_iterator(dir)) {
    std::string dir = std::filesystem::path(entry.path()).parent_path();
    std::string file_name = std::filesystem::path(entry.path()).stem();
    std::string ext = std::filesystem::path(entry.path()).extension();
    if (ext != ".log") continue;
    int pos = file_name.rfind("_");
    int64_t ckpt = std::stoll(file_name.substr(pos + 1));
    if (ckpt > last_ckpt) {
      last_ckpt = ckpt;
    }
  }
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

    if (min_seq == -1) {
      e_list.push_back(std::make_pair(time, entry.path()));
    } else if ((min_seq <= last_ckpt && max_seq >= last_ckpt)) {
      list.push_back(std::make_pair(time, entry.path()));
    }
  }

  sort(e_list.begin(), e_list.end());
  list.push_back(e_list.back());

  sort(list.begin(), list.end());
  return std::make_pair(list, last_ckpt);
}

void Recovery::ReadLogs(
    std::function<void(const SystemInfoData& data)> system_callback,
    std::function<void(std::unique_ptr<Context> context,
                       std::unique_ptr<Request> request)>
        call_back) {
  if (recovery_enabled_ == false) {
    return;
  }
  std::unique_lock<std::mutex> lk(mutex_);
  auto recovery_files_pair = GetRecoveryFiles();
  int64_t ckpt = recovery_files_pair.second;
  int idx = 0;
  for (auto path : recovery_files_pair.first) {
    ReadLogsFromFiles(path.second, ckpt, idx++, system_callback, call_back);
  }
}

void Recovery::ReadLogsFromFiles(
    const std::string& path, int64_t ckpt, int file_idx,
    std::function<void(const SystemInfoData& data)> system_callback,
    std::function<void(std::unique_ptr<Context> context,
                       std::unique_ptr<Request> request)>
        call_back) {
  int fd = open(path.c_str(), O_CREAT | O_RDONLY, 0666);
  if (fd < 0) {
    LOG(ERROR) << " open file fail:" << path;
  }
  LOG(INFO) << "read logs:" << path << " pos:" << lseek(fd, 0, SEEK_CUR);
  assert(fd >= 0);

  size_t data_len = 0;
  Read(fd, sizeof(data_len), reinterpret_cast<char*>(&data_len));
  {
    std::string data;
    char* buf = new char[data_len];
    if (!Read(fd, data_len, buf)) {
      LOG(ERROR) << "Read system info fail";
      return;
    }
    data = std::string(buf, data_len);
    delete buf;
    std::vector<std::string> data_list = ParseRawData(data);

    SystemInfoData info;
    if (data_list.empty() || !info.ParseFromString(data_list[0])) {
      LOG(ERROR) << "parse info fail:" << data.size();
      return;
    }
    LOG(INFO) << "read system info:" << info.DebugString();
    if (file_idx == 0) {
      system_callback(info);
    }
  }

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
    if (ckpt < recovery_data->request->seq()) {
      recovery_data->request->set_is_recovery(true);
      max_seq = recovery_data->request->seq();
      call_back(std::move(recovery_data->context),
                std::move(recovery_data->request));
    }
  }

  LOG(ERROR) << "read log from files:" << path << " done"
             << " recovery max seq:" << max_seq;

  close(fd);
}

}  // namespace resdb

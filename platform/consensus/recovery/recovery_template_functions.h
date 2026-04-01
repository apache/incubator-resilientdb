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

template<typename TDerived>
template<typename TSystemInfoData, typename TCallback>
void RecoveryBase<TDerived>::ReadLogs(std::function<void(const TSystemInfoData& data)> system_callback,
                TCallback call_back,
                std::function<void(int)> set_start_point) {
  if (recovery_enabled_ == false) {
    return;
  }

  int64_t storage_ckpt = 0;
  if(storage_) {
    storage_ckpt = storage_->GetLastCheckpoint();
  }
  std::unique_lock<std::mutex> lk(mutex_);

  auto recovery_files_pair = GetRecoveryFiles(storage_ckpt);
  int64_t ckpt = recovery_files_pair.second;
  if (set_start_point) {
    set_start_point(ckpt);
  }
  int idx = 0;
  for (auto path : recovery_files_pair.first) {
    ReadLogsFromFiles<TSystemInfoData, TCallback>(path.second, ckpt, idx++, system_callback, call_back);
  }
}

template<typename TDerived>
template<typename TSystemInfoData, typename TCallback>
void RecoveryBase<TDerived>::SwitchFile(const std::string& file_path, TCallback call_back) {
  std::unique_lock<std::mutex> lk(mutex_);

  min_seq_ = -1;
  max_seq_ = -1;
  ReadLogsFromFiles<TSystemInfoData, TCallback>(
      file_path, 0, 0,
      [&](const TSystemInfoData& data) {},
      call_back);
  OpenFile(file_path);
  LOG(INFO) << "switch to file:" << file_path << " seq:"
            << "[" << min_seq_ << "," << max_seq_ << "]";
}

template<typename TDerived>
void RecoveryBase<TDerived>::OpenFile(const std::string& path) {
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
    static_cast<TDerived*>(this)->WriteSystemInfo();
  }

  lseek(fd_, 0, SEEK_END);
  LOG(ERROR) << "open file:" << path << " pos:" << lseek(fd_, 0, SEEK_CUR)
             << " fd:" << fd_;
  assert(fd_ >= 0);
}

template<typename TDerived>
template<typename TSystemInfoData, typename TCallback>
void RecoveryBase<TDerived>::ReadLogsFromFiles(
    const std::string& path, int64_t ckpt, int file_idx,
    std::function<void(const TSystemInfoData& data)> system_callback,
    TCallback call_back) {
  int fd = open(path.c_str(), O_CREAT | O_RDONLY, 0666);
  if (fd < 0) {
    LOG(ERROR) << " open file fail:" << path;
  }
  LOG(INFO) << "read logs:" << path << " pos:" << lseek(fd, 0, SEEK_CUR);
  assert(fd >= 0);

  size_t data_len = 0;
  if constexpr (std::is_same_v<TSystemInfoData, SystemInfoData>) {
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

      bool successful_callback = static_cast<TDerived*>(this)->PerformSystemCallback(data_list, system_callback);

      if (!successful_callback) {
        LOG(ERROR) << "parse info fail:" << data.size();
      }
    }
  }

  decltype(ParseData(std::string{})) request_list;

  while (Read(fd, sizeof(data_len), reinterpret_cast<char*>(&data_len))) {
    std::string data;
    char* buf = new char[data_len];
    if (!Read(fd, data_len, buf)) {
      LOG(ERROR) << "Read data log fail";
      break;
    }
    data = std::string(buf, data_len);
    delete buf;

    auto list = ParseData(data);
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

  static_cast<TDerived*>(this)->PerformCallback(request_list, call_back, ckpt);

  LOG(ERROR) << "read log from files:" << path << " done";
  close(fd);
}
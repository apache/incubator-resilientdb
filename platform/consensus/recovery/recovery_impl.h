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

template <typename TDerived, typename TSystemInfoData, typename TCallback>
RecoveryBase<TDerived, TSystemInfoData, TCallback>::RecoveryBase(
    const ResDBConfig& config, CheckPoint* checkpoint, Storage* storage,
    std::function<void(uint64_t)> on_checkpoint)
    : config_(config),
      checkpoint_(checkpoint),
      storage_(storage),
      on_checkpoint_callback_(on_checkpoint) {
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
    LOG(INFO) << "recovery is not enabled:" << recovery_enabled_;
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
}

template <typename TDerived, typename TSystemInfoData, typename TCallback>
RecoveryBase<TDerived, TSystemInfoData, TCallback>::~RecoveryBase() {
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

template <typename TDerived, typename TSystemInfoData, typename TCallback>
int64_t RecoveryBase<TDerived, TSystemInfoData, TCallback>::GetMaxSeq() {
  return max_seq_;
}

template <typename TDerived, typename TSystemInfoData, typename TCallback>
int64_t RecoveryBase<TDerived, TSystemInfoData, TCallback>::GetMinSeq() {
  return min_seq_;
}

template <typename TDerived, typename TSystemInfoData, typename TCallback>
void RecoveryBase<TDerived, TSystemInfoData,
                  TCallback>::UpdateStableCheckPoint() {
  if (checkpoint_ == nullptr) {
    return;
  }
  while (!stop_) {
    int64_t latest_ckpt = checkpoint_->GetStableCheckpoint();
    LOG(ERROR) << "get stable ckpt:" << latest_ckpt << " last:" << last_ckpt_;
    if (last_ckpt_ >= latest_ckpt) {
      sleep(recovery_ckpt_time_s_);
      continue;
    }
    last_ckpt_ = latest_ckpt;
    FinishFile(latest_ckpt);
  }
}

template <typename TDerived, typename TSystemInfoData, typename TCallback>
void RecoveryBase<TDerived, TSystemInfoData, TCallback>::GetLastFile() {
  std::string dir = std::filesystem::path(file_path_).parent_path();
  last_ckpt_ = -1;
  uint64_t m_time_s = 0;
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
    LOG(ERROR) << "get path:" << entry.path() << " min:" << min_seq
               << " time:" << time_s;
    if (min_seq == -1) {
      if (m_time_s < time_s) {
        file_path_ = entry.path();
        last_ckpt_ = ckpt;
        LOG(ERROR) << "get last path:" << file_name << " min:" << min_seq
                   << " time_s:" << time_s << " min:" << m_time_s;
        m_time_s = time_s;
      }
    }
  }
  if (last_ckpt_ == -1) {
    last_ckpt_ = 0;
    file_path_ = GenerateFile(last_ckpt_, -1, -1);
  }
}

template <typename TDerived, typename TSystemInfoData, typename TCallback>
std::string RecoveryBase<TDerived, TSystemInfoData, TCallback>::GenerateFile(
    int64_t seq, int64_t min_seq, int64_t max_seq) {
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

template <typename TDerived, typename TSystemInfoData, typename TCallback>
void RecoveryBase<TDerived, TSystemInfoData, TCallback>::FinishFile(
    int64_t seq) {
  {
    std::unique_lock<std::mutex> lk(mutex_);
    Flush();
    if (storage_) {
      if (!storage_->Flush(true)) {
        return;
      }
    }
    std::string new_file_path = GenerateFile(seq, min_seq_, max_seq_);
    close(fd_);

    min_seq_ = -1;
    max_seq_ = -1;

    std::rename(file_path_.c_str(), new_file_path.c_str());

    std::string dir_path =
        std::filesystem::path(file_path_).parent_path().string();
    int dir_fd = open(dir_path.c_str(), O_RDONLY);
    fsync(dir_fd);
    close(dir_fd);

    LOG(INFO) << "rename:" << file_path_ << " to:" << new_file_path;
    std::string next_file_path = GenerateFile(seq, -1, -1);
    file_path_ = next_file_path;

    OpenFile(file_path_);
  }

  if (on_checkpoint_callback_) {
    on_checkpoint_callback_(seq);
  }
}

template <typename TDerived, typename TSystemInfoData, typename TCallback>
void RecoveryBase<TDerived, TSystemInfoData, TCallback>::AppendData(
    const std::string& data) {
  size_t len = data.size();
  buffer_.append(reinterpret_cast<const char*>(&len), sizeof(len));
  buffer_.append(data);
}

template <typename TDerived, typename TSystemInfoData, typename TCallback>
auto RecoveryBase<TDerived, TSystemInfoData, TCallback>::ParseData(
    const std::string& data) {
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

  return static_cast<TDerived*>(this)->ParseDataListItem(data_list);
}

template <typename TDerived, typename TSystemInfoData, typename TCallback>
std::vector<std::string>
RecoveryBase<TDerived, TSystemInfoData, TCallback>::ParseRawData(
    const std::string& data) {
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

template <typename TDerived, typename TSystemInfoData, typename TCallback>
void RecoveryBase<TDerived, TSystemInfoData, TCallback>::MayFlush() {
  if (buffer_.size() > buffer_size_) {
    Flush();
  }
}

template <typename TDerived, typename TSystemInfoData, typename TCallback>
void RecoveryBase<TDerived, TSystemInfoData, TCallback>::Flush() {
  size_t len = buffer_.size();
  if (len == 0) {
    return;
  }

  Write(reinterpret_cast<const char*>(&len), sizeof(len));
  Write(reinterpret_cast<const char*>(buffer_.c_str()), len);
  buffer_.clear();
  fsync(fd_);
}

template <typename TDerived, typename TSystemInfoData, typename TCallback>
void RecoveryBase<TDerived, TSystemInfoData, TCallback>::Write(const char* data,
                                                               size_t len) {
  int pos = 0;
  while (len > 0) {
    int write_len = write(fd_, data + pos, len);
    if (write_len <= 0) break;
    len -= write_len;
    pos += write_len;
  }
}

template <typename TDerived, typename TSystemInfoData, typename TCallback>
bool RecoveryBase<TDerived, TSystemInfoData, TCallback>::Read(int fd,
                                                              size_t len,
                                                              char* data) {
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

template <typename TDerived, typename TSystemInfoData, typename TCallback>
std::pair<std::vector<std::pair<int64_t, std::string>>, int64_t>
RecoveryBase<TDerived, TSystemInfoData, TCallback>::GetRecoveryFiles(
    int64_t ckpt) {
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
  LOG(ERROR) << "file max ckpt:" << last_ckpt << " storage ckpt:" << ckpt;
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
  if (!e_list.empty()) {
    list.push_back(e_list.back());
  }

  sort(list.begin(), list.end());
  return std::make_pair(list, last_ckpt);
}

template <typename TDerived, typename TSystemInfoData, typename TCallback>
std::vector<std::pair<int64_t, std::string>>
RecoveryBase<TDerived, TSystemInfoData, TCallback>::GetSortedRecoveryFiles(
    uint64_t need_min_seq, uint64_t need_max_seq) {
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
  if (!e_list.empty()) {
    list.push_back(e_list.back());
  }
  sort(list.begin(), list.end());
  return list;
}

template <typename TDerived, typename TSystemInfoData, typename TCallback>
void RecoveryBase<TDerived, TSystemInfoData, TCallback>::ReadLogs(
    std::function<void(const TSystemInfoData& data)> system_callback,
    TCallback call_back, std::function<void(int)> set_start_point) {
  if (recovery_enabled_ == false) {
    return;
  }

  int64_t storage_ckpt = 0;
  if (storage_) {
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
    ReadLogsFromFiles(path.second, ckpt, idx++, system_callback, call_back);
  }
}

template <typename TDerived, typename TSystemInfoData, typename TCallback>
void RecoveryBase<TDerived, TSystemInfoData, TCallback>::SwitchFile(
    const std::string& file_path, TCallback call_back) {
  std::unique_lock<std::mutex> lk(mutex_);

  min_seq_ = -1;
  max_seq_ = -1;
  ReadLogsFromFiles(
      file_path, 0, 0, [&](const TSystemInfoData& data) {}, call_back);
  OpenFile(file_path);
  LOG(INFO) << "switch to file:" << file_path << " seq:"
            << "[" << min_seq_ << "," << max_seq_ << "]";
}

template <typename TDerived, typename TSystemInfoData, typename TCallback>
void RecoveryBase<TDerived, TSystemInfoData, TCallback>::OpenFile(
    const std::string& path) {
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

template <typename TDerived, typename TSystemInfoData, typename TCallback>
void RecoveryBase<TDerived, TSystemInfoData, TCallback>::ReadLogsFromFiles(
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
  static_cast<TDerived*>(this)->HandleSystemInfo(fd, system_callback);

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
      break;
    }
    for (auto& l : list) {
      request_list.push_back(std::move(l));
    }
  }
  if (request_list.size() == 0) {
    LOG(ERROR) << " Request list is empty";
    close(fd);
    fd = open(path.c_str(), O_RDWR);
    if (fd < 0) {
      LOG(ERROR) << " open file as O_RDWR to truncate fail:" << path;
    }
    if (ftruncate(fd, 0) != 0) {
      LOG(ERROR) << " Failed to truncate file";
    }
    ftruncate(fd, 0);
  }

  static_cast<TDerived*>(this)->PerformCallback(request_list, call_back, ckpt);

  LOG(ERROR) << "read log from files:" << path << " done";
  close(fd);
}

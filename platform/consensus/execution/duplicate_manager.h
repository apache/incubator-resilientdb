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

#include <stdint.h>
#include <unistd.h>

#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <thread>

#include "platform/config/resdb_config.h"

namespace resdb {

class DuplicateManager {
 public:
  DuplicateManager(const ResDBConfig& config);
  ~DuplicateManager();

  bool CheckIfProposed(const std::string& hash);
  uint64_t CheckIfExecuted(const std::string& hash);
  void AddProposed(const std::string& hash);
  void AddExecuted(const std::string& hash, uint64_t seq);
  void EraseProposed(const std::string& hash);
  void EraseExecuted(const std::string& hash);
  bool CheckAndAddProposed(const std::string& hash);
  bool CheckAndAddExecuted(const std::string& hash, uint64_t seq);
  void UpdateRecentHash();

 private:
  bool IsStop();

 private:
  ResDBConfig config_;
  std::set<std::string> proposed_hash_set_;
  std::set<std::string> executed_hash_set_;
  std::queue<std::pair<std::string, uint64_t>> proposed_hash_time_queue_;
  std::queue<std::pair<std::string, uint64_t>> executed_hash_time_queue_;
  std::map<std::string, uint64_t> executed_hash_seq_;
  std::thread update_thread_;
  std::mutex prop_mutex_;
  std::mutex exec_mutex_;
  uint64_t frequency_useconds_ = 5000000;  // 5s
  uint64_t window_useconds_ = 20000000;    // 20s
  bool stop_ = false;
};

}  // namespace resdb

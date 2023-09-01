/*
 * Copyright (c) 2019-2022 ExpoLab, UC Davis
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
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

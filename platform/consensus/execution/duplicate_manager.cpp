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

#include "platform/consensus/execution/duplicate_manager.h"

#include <glog/logging.h>

#include "common/utils/utils.h"
namespace resdb {
DuplicateManager::DuplicateManager(const ResDBConfig& config)
    : config_(config) {
  if (config.GetConfigData().duplicate_check_frequency_useconds() > 0) {
    frequency_useconds_ =
        config.GetConfigData().duplicate_check_frequency_useconds();
  }
  stop_ = false;
  update_thread_ = std::thread(&DuplicateManager::UpdateRecentHash, this);
}

DuplicateManager::~DuplicateManager() {
  stop_ = true;
  if (update_thread_.joinable()) {
    update_thread_.join();
  }
}

bool DuplicateManager::IsStop() { return stop_; }

bool DuplicateManager::CheckIfProposed(const std::string& hash) {
  std::lock_guard<std::mutex> lk(prop_mutex_);
  return proposed_hash_set_.find(hash) != proposed_hash_set_.end();
}

uint64_t DuplicateManager::CheckIfExecuted(const std::string& hash) {
  std::lock_guard<std::mutex> lk(exec_mutex_);
  if (executed_hash_set_.find(hash) != executed_hash_set_.end()) {
    return executed_hash_seq_[hash];
  } else {
    return 0;
  }
}

void DuplicateManager::AddProposed(const std::string& hash) {
  std::lock_guard<std::mutex> lk(prop_mutex_);
  proposed_hash_time_queue_.push(std::make_pair(hash, GetCurrentTime()));
  proposed_hash_set_.insert(hash);
}

void DuplicateManager::AddExecuted(const std::string& hash, uint64_t seq) {
  std::lock_guard<std::mutex> lk(exec_mutex_);
  executed_hash_time_queue_.push(std::make_pair(hash, GetCurrentTime()));
  executed_hash_set_.insert(hash);
  executed_hash_seq_[hash] = seq;
}

bool DuplicateManager::CheckAndAddProposed(const std::string& hash) {
  std::lock_guard<std::mutex> lk(prop_mutex_);
  if (proposed_hash_set_.find(hash) != proposed_hash_set_.end()) {
    return true;
  }
  proposed_hash_time_queue_.push(std::make_pair(hash, GetCurrentTime()));
  proposed_hash_set_.insert(hash);
  return false;
}

bool DuplicateManager::CheckAndAddExecuted(const std::string& hash,
                                           uint64_t seq) {
  std::lock_guard<std::mutex> lk(exec_mutex_);
  if (executed_hash_set_.find(hash) != executed_hash_set_.end()) {
    return true;
  }
  executed_hash_time_queue_.push(std::make_pair(hash, GetCurrentTime()));
  executed_hash_set_.insert(hash);
  executed_hash_seq_[hash] = seq;
  return false;
}

void DuplicateManager::EraseProposed(const std::string& hash) {
  std::lock_guard<std::mutex> lk(prop_mutex_);
  proposed_hash_set_.erase(hash);
}

void DuplicateManager::EraseExecuted(const std::string& hash) {
  std::lock_guard<std::mutex> lk(exec_mutex_);
  executed_hash_set_.erase(hash);
  executed_hash_seq_.erase(hash);
}

void DuplicateManager::UpdateRecentHash() {
  uint64_t time = GetCurrentTime();
  while (!IsStop()) {
    time = time + frequency_useconds_;
    auto sleep_time = time - GetCurrentTime();
    usleep(sleep_time);
    while (true) {
      std::lock_guard<std::mutex> lk(prop_mutex_);
      if (!proposed_hash_time_queue_.empty()) {
        auto it = proposed_hash_time_queue_.front();
        if (it.second + window_useconds_ < time) {
          proposed_hash_time_queue_.pop();
          proposed_hash_set_.erase(it.first);
        } else {
          break;
        }
      } else {
        break;
      }
    }

    while (true) {
      std::lock_guard<std::mutex> lk(exec_mutex_);
      if (!executed_hash_time_queue_.empty()) {
        auto it = executed_hash_time_queue_.front();
        if (it.second + window_useconds_ < time) {
          executed_hash_time_queue_.pop();
          executed_hash_set_.erase(it.first);
          executed_hash_seq_.erase(it.first);
        } else {
          break;
        }
      } else {
        break;
      }
    }
  }
}
}  // namespace resdb

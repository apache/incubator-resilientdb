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

#include <glog/logging.h>
#include "platform/consensus/execution/duplicate_manager.h"

#include "common/utils/utils.h"
namespace resdb{
  DuplicateManager::DuplicateManager(){
    update_thread_ = std::thread(&DuplicateManager::UpdateRecentHash, this);
  }

  bool DuplicateManager::CheckIfProposed(std::string hash) {
    std::lock_guard<std::mutex> lk(prop_mutex_);
    return proposed_hash_set_.find(hash) != proposed_hash_set_.end();
  }

  uint64_t DuplicateManager::CheckIfExecuted(std::string hash){
    std::lock_guard<std::mutex> lk(exec_mutex_);
    if (executed_hash_set_.find(hash) != executed_hash_set_.end()) {
      return executed_hash_seq_[hash];
    }
    else {
      return 0;
    }
  }

  void DuplicateManager::AddProposed(std::string hash) {
    std::lock_guard<std::mutex> lk(prop_mutex_);
    proposed_hash_time_queue_.push(std::make_pair(hash, GetCurrentTime()));
    proposed_hash_set_.insert(hash);
  }

  void DuplicateManager::AddExecuted(std::string hash, uint64_t seq) {
    std::lock_guard<std::mutex> lk(exec_mutex_);
    executed_hash_time_queue_.push(std::make_pair(hash, GetCurrentTime()));
    executed_hash_set_.insert(hash);
    executed_hash_seq_[hash] = seq;
  }

  bool DuplicateManager::CheckAndAddProposed(std::string hash){
    std::lock_guard<std::mutex> lk(prop_mutex_);
    if (proposed_hash_set_.find(hash) != proposed_hash_set_.end()) {
        return true;
    }
    proposed_hash_time_queue_.push(std::make_pair(hash, GetCurrentTime()));
    proposed_hash_set_.insert(hash);
    return false;
  }

  bool DuplicateManager::CheckAndAddExecuted(std::string hash, uint64_t seq) {
    std::lock_guard<std::mutex> lk(exec_mutex_);
    if (executed_hash_set_.find(hash) != executed_hash_set_.end()) {
        return true;
    }
    executed_hash_time_queue_.push(std::make_pair(hash, GetCurrentTime()));
    executed_hash_set_.insert(hash);
    executed_hash_seq_[hash] = seq;
    return false;
  }

  void DuplicateManager::EraseProposed(std::string hash) {
    std::lock_guard<std::mutex> lk(prop_mutex_);
    proposed_hash_set_.erase(hash);
  }

  void DuplicateManager::EraseExecuted(std::string hash) {
    std::lock_guard<std::mutex> lk(exec_mutex_);
    executed_hash_set_.erase(hash);
    executed_hash_seq_.erase(hash);
  }

  void DuplicateManager::UpdateRecentHash() {
    uint64_t time = GetCurrentTime();
    while(true) {
        time = time + frequency_useconds_;
        auto sleep_time = time - GetCurrentTime();
        usleep(sleep_time);
        while (true) {
          std::lock_guard<std::mutex> lk(prop_mutex_);
          if(!proposed_hash_time_queue_.empty()){
            auto it = proposed_hash_time_queue_.front();
            if (it.second + window_useconds_ < time) {
                proposed_hash_time_queue_.pop();
                proposed_hash_set_.erase(it.first);
            } 
            else {
                break;
            }
          }
          else {
            break;
          }
        }

        while (true) {
          std::lock_guard<std::mutex> lk(exec_mutex_);
          if(!executed_hash_time_queue_.empty()) {
            auto it = executed_hash_time_queue_.front();
            if (it.second + window_useconds_ < time) {
                executed_hash_time_queue_.pop();
                executed_hash_set_.erase(it.first);
                executed_hash_seq_.erase(it.first);
            } 
            else {
                break;
            }
          }
          else {
            break;
          }
      }
    }
  }
}
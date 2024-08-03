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

#include <glog/logging.h>

#include <boost/lockfree/queue.hpp>
#include <condition_variable>

#include "absl/status/statusor.h"

namespace resdb {

template <typename T>
class LockFreeQueue {
 public:
  LockFreeQueue(const std::string& name = "") : name_(name), queue_(4096) {}
  void Push(std::unique_ptr<T> data) {
    T* ptr = data.release();
    while (!queue_.push(ptr)) {
      LOG(ERROR) << "push data:" << name_ << " fail";
    }
    if (need_notify_.load()) {
      bool old_v = true;
      if (need_notify_.compare_exchange_strong(old_v, false,
                                               std::memory_order_acq_rel,
                                               std::memory_order_acquire)) {
        std::lock_guard<std::mutex> lk(mutex_);
        cv_.notify_all();
        return;
      }
    }
  }

  std::unique_ptr<T> Pop(int timeout_ms = 100) {
    T* ret = nullptr;
    if (!queue_.pop(ret)) {
      if (timeout_ms > 0) {
        std::unique_lock<std::mutex> lk(mutex_);
        need_notify_ = true;
        cv_.wait_for(lk, std::chrono::milliseconds(timeout_ms),
                     [&] { return !need_notify_.load(); });
        if (queue_.pop(ret)) {
          return std::unique_ptr<T>(ret);
        }
      }
      return nullptr;
    }
    return std::unique_ptr<T>(ret);
  }
  bool Empty() { return queue_.empty(); }

 private:
  std::string name_;
  boost::lockfree::queue<T*> queue_;
  std::condition_variable cv_;
  std::mutex mutex_;
  std::atomic<bool> need_notify_;
};

}  // namespace resdb

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
                                               std::memory_order_acq_rel)) {
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

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

#include <condition_variable>
#include <list>

namespace resdb {

template <typename T>
class BatchQueue {
  struct BatchQueueItem {
    std::vector<T> list;
  };

 public:
  int num = 0;
  BatchQueue() = default;
  BatchQueue(const std::string& name, int batch_size)
      : name_(name), batch_size_(batch_size) {}
  void Push(T&& data) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (queue_.empty() || queue_.back()->list.size() >= batch_size_) {
      queue_.push_back(std::make_unique<BatchQueueItem>());
      queue_.back()->list.reserve(batch_size_);
    }
    queue_.back()->list.push_back(std::move(data));
    cv_.notify_all();
  }

  size_t Size() { return queue_.size(); }

  std::vector<T> Pop(int timeout_ms) {
    std::unique_ptr<BatchQueueItem> item = nullptr;
    {
      std::unique_lock<std::mutex> lk(mutex_);
      if (timeout_ms > 0) {
        cv_.wait_for(lk, std::chrono::microseconds(timeout_ms), [&] {
          return !queue_.empty() && queue_.front()->list.size() >= batch_size_;
        });
      }
      if (queue_.empty()) {
        return std::vector<T>();
      }
      item = std::move(queue_.front());
      queue_.pop_front();
    }
    return std::move(item->list);
  }

 private:
  std::string name_;
  std::condition_variable cv_;
  std::mutex mutex_;
  //  std::queue<T> queue_ GUARDED_BY(mutex_);
  std::list<std::unique_ptr<BatchQueueItem>> queue_;
  size_t batch_size_;
};

}  // namespace resdb

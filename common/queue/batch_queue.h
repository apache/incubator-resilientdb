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

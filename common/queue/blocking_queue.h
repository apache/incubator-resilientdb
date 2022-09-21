#pragma once

#include <glog/logging.h>

#include <condition_variable>
#include <queue>

#include "absl/status/statusor.h"

namespace resdb {

template <typename T>
class BlockingQueue {
 public:
  BlockingQueue() = default;
  BlockingQueue(const std::string& name) : name_(name) {}
  void Push(T&& data) {
    std::lock_guard<std::mutex> lk(mutex_);
    queue_.push(std::move(data));
    cv_.notify_all();
  }

  void Push(T& data) {
    std::lock_guard<std::mutex> lk(mutex_);
    queue_.push(std::move(data));
    cv_.notify_all();
  }

  absl::StatusOr<T*> Front() {
    std::unique_lock<std::mutex> lk(mutex_);
    if (queue_.empty()) {
      return nullptr;
    }
    return &queue_.front();
  }

  T Pop() {
    std::unique_lock<std::mutex> lk(mutex_);
    cv_.wait_for(lk, std::chrono::microseconds(timeout_ms_),
                 [&] { return !queue_.empty(); });

    if (queue_.empty()) {
      return nullptr;
    }
    auto resp = std::move(queue_.front());
    queue_.pop();
    return resp;
  }

  T Pop(int timeout_ms) {
    std::unique_lock<std::mutex> lk(mutex_);
    if (timeout_ms > 0) {
      cv_.wait_for(lk, std::chrono::microseconds(timeout_ms),
                   [&] { return !queue_.empty(); });
    }
    if (queue_.empty()) {
      return nullptr;
    }
    auto resp = std::move(queue_.front());
    queue_.pop();
    return resp;
  }

  T PopWithSize(int timeout_ms, size_t size) {
    std::unique_lock<std::mutex> lk(mutex_);
    if (timeout_ms > 0) {
      cv_.wait_for(lk, std::chrono::microseconds(timeout_ms),
                   [&] { return queue_.size() >= size; });
    }
    if (queue_.empty()) {
      return nullptr;
    }
    auto resp = std::move(queue_.front());
    queue_.pop();
    return resp;
  }

 private:
  std::string name_;
  std::condition_variable cv_;
  std::mutex mutex_;
  //  std::queue<T> queue_ GUARDED_BY(mutex_);
  std::queue<T> queue_;
  int64_t timeout_ms_ = 500;  // microsecond for timeout.
};

}  // namespace resdb

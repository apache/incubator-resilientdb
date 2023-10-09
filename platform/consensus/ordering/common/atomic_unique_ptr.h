#pragma once

#include <atomic>
#include <memory>
#include <string>

namespace resdb {
namespace common {

template <typename T>
class AtomicUniquePtr {
 public:
  AtomicUniquePtr() : v_(0) {}
  bool Set(std::unique_ptr<T>& new_ptr) {
    int old_v = 0;
    bool res = v_.compare_exchange_strong(old_v, 1, std::memory_order_acq_rel,
                                          std::memory_order_acq_rel);
    if (!res) {
      return false;
    }
    ptr_ = std::move(new_ptr);
    v_ = 2;
    return true;
  }

  T* Reference() {
    int v = v_.load(std::memory_order_acq_rel);
    if (v <= 1) {
      return nullptr;
    }
    return ptr_.get();
  }

 private:
  std::unique_ptr<T> ptr_;
  std::atomic<int> v_;
};

}  // namespace common
}  // namespace resdb

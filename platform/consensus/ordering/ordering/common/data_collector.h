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

#include <bitset>
#include <list>

#include "platform/consensus/execution/transaction_executor.h"
#include "platform/proto/resdb.pb.h"

namespace resdb {

class DataCollectorBasic {
 public:
  enum CollectorResultCode {
    INVALID = -2,
    OK = 0,
    STATE_CHANGED = 1,
  };

  enum TransactionStatue {
    None = 0,
    Prepare = -999,
    READY_PREPARE = 1,
    READY_COMMIT = 2,
    READY_EXECUTE = 3,
    EXECUTED = 4,
  };

  struct RequestInfo {
    std::unique_ptr<Request> request;
  };

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

  template <typename Type>
  static std::unique_ptr<Request> ConvertToRequest(const Type& new_type);
};

class DataCollector : public DataCollectorBasic {
 public:
  DataCollector(uint64_t seq, TransactionExecutor* executor)
      : seq_(seq),
        executor_(executor),
        status_(DataCollector::TransactionStatue::None) {}

  ~DataCollector() = default;

  // Add a message and count by its hash value.
  // After it is done call_back will be triggered.
  int AddRequest(
      std::unique_ptr<Request> request, bool is_main_request,
      std::function<void(const Request&, int received_count,
                         std::atomic<DataCollector::TransactionStatue>* status)>
          call_back);

  uint64_t Seq();

 private:
  int Commit();

 private:
  uint64_t seq_;
  TransactionExecutor* executor_;
  std::atomic<bool> is_committed_ = false;
  std::map<std::string, std::list<std::unique_ptr<RequestInfo>>>
      data_[Request::NUM_OF_TYPE];
  AtomicUniquePtr<RequestInfo> atomic_mian_request_;
  std::atomic<DataCollector::TransactionStatue> status_ =
      DataCollector::TransactionStatue::None;
  std::bitset<128> senders_[Request::NUM_OF_TYPE];
  std::mutex mutex_;
};

template <typename Type>
std::unique_ptr<Request> DataCollectorBasic::ConvertToRequest(
    const Type& new_type) {
  std::unique_ptr<Request> request = std::make_unique<Request>();
  request->set_proxy_id(new_type.proxy_id());
  request->set_sender_id(new_type.sender_id());
  request->set_hash(new_type.hash());
  request->set_type(new_type.type());
  request->set_seq(new_type.seq());
  request->set_data(new_type.data());
  request->set_current_view(new_type.view());
  if (new_type.has_data_signature()) {
    *request->mutable_data_signature() = new_type.data_signature();
  }

  return request;
}

}  // namespace resdb

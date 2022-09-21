#pragma once

#include <bitset>

#include "execution/transaction_executor.h"
#include "proto/resdb.pb.h"
#include "server/server_comm.h"
#include "statistic/stats.h"

namespace resdb {

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
  SignatureInfo signature;
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

class TransactionCollector {
 public:
  TransactionCollector(uint64_t seq, TransactionExecutor* executor,
                       bool need_data_collection = false)
      : seq_(seq),
        executor_(executor),
        status_(TransactionStatue::None),
        need_data_collection_(need_data_collection) {}

  ~TransactionCollector() = default;

  // TODO split the context list.
  // context contains the client channel used for sending back the response.
  int SetContextList(uint64_t seq,
                     std::vector<std::unique_ptr<Context>> context);
  bool HasClientContextList(uint64_t seq) const;
  std::vector<std::unique_ptr<Context>> FetchContextList(uint64_t seq);

  typedef std::list<std::unique_ptr<RequestInfo>> CollectorDataType;

  // Add a message and count by its hash value.
  // After it is done call_back will be triggered.
  int AddRequest(std::unique_ptr<Request> request,
                 const SignatureInfo& signature, bool is_main_request,
                 std::function<void(const Request&, int received_count,
                                    CollectorDataType* data,
                                    std::atomic<TransactionStatue>* status)>
                     call_back);

  uint64_t Seq();

 private:
  int Commit();

 private:
  uint64_t seq_;
  TransactionExecutor* executor_;
  std::atomic<bool> is_committed_ = false;
  std::vector<std::unique_ptr<Context>> context_list_;
  std::map<std::string, std::list<std::unique_ptr<RequestInfo>>>
      data_[Request::NUM_OF_TYPE];
  AtomicUniquePtr<RequestInfo> atomic_mian_request_;
  std::atomic<TransactionStatue> status_ = TransactionStatue::None;
  std::bitset<128> senders_[Request::NUM_OF_TYPE];
  bool need_data_collection_;
  std::mutex mutex_;
};

}  // namespace resdb

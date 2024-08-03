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

#include <bitset>

#include "platform/consensus/execution/transaction_executor.h"
#include "platform/networkstrate/server_comm.h"
#include "platform/proto/resdb.pb.h"
#include "platform/statistic/stats.h"

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

  void Clear() {
    v_ = 0;
    ptr_ = nullptr;
  }

 private:
  std::unique_ptr<T> ptr_;
  std::atomic<int> v_;
};

class TransactionCollector {
 public:
  TransactionCollector(uint64_t seq, TransactionExecutor* executor,
                       bool enable_viewchange = false)
      : seq_(seq),
        executor_(executor),
        status_(TransactionStatue::None),
        enable_viewchange_(enable_viewchange),
        view_(0) {}

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
  int AddRequest(
      std::unique_ptr<Request> request, const SignatureInfo& signature,
      bool is_main_request,
      std::function<void(const Request&, int received_count,
                         CollectorDataType* data,
                         std::atomic<TransactionStatue>* status, bool force)>
          call_back);

  std::vector<RequestInfo> GetPreparedProof();
  TransactionStatue GetStatus() const;

  uint64_t Seq();

  bool IsPrepared();

  std::vector<std::string> GetAllStoredHash();

 private:
  int Commit();

 private:
  uint64_t seq_;
  TransactionExecutor* executor_;
  std::atomic<bool> is_committed_ = false;
  std::atomic<bool> is_prepared_ = false;
  std::vector<std::unique_ptr<Context>> context_list_;
  std::map<std::string, std::list<std::unique_ptr<RequestInfo>>>
      data_[Request::NUM_OF_TYPE];
  std::vector<std::unique_ptr<RequestInfo>> prepared_proof_;
  AtomicUniquePtr<RequestInfo> atomic_mian_request_;
  std::atomic<TransactionStatue> status_ = TransactionStatue::None;
  bool enable_viewchange_;
  std::mutex mutex_;
  std::vector<SignatureInfo> commit_certs_;
  std::map<std::string, std::bitset<128>> senders_[Request::NUM_OF_TYPE];
  std::set<std::unique_ptr<RequestInfo>> other_main_request_;
  uint64_t view_;
};

}  // namespace resdb

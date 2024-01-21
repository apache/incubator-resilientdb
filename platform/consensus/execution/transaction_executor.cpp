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

#include "platform/consensus/execution/transaction_executor.h"

#include <glog/logging.h>
#include "common/utils/utils.h"

namespace resdb {

int mod = 2048;
TransactionExecutor::TransactionExecutor(
    const ResDBConfig& config, PostExecuteFunc post_exec_func,
    SystemInfo* system_info,
    std::unique_ptr<TransactionManager> transaction_manager)
    : config_(config),
      post_exec_func_(post_exec_func),
      system_info_(system_info),
      transaction_manager_(std::move(transaction_manager)),
      commit_queue_("order"),
      execute_queue_("execute"),
      stop_(false),
      duplicate_manager_(nullptr) {

  memset(blucket_, 0, sizeof(blucket_));
  global_stats_ = Stats::GetGlobalStats();
  ordering_thread_ = std::thread(&TransactionExecutor::OrderMessage, this);
  for (int i = 0; i < execute_thread_num_; ++i) {
    execute_thread_.push_back(
        std::thread(&TransactionExecutor::ExecuteMessage, this));
  }

  if (transaction_manager_ && transaction_manager_->IsOutOfOrder()) {
    execute_OOO_thread_ =
        std::thread(&TransactionExecutor::ExecuteMessageOutOfOrder, this);
    LOG(ERROR) << " is out of order:" << transaction_manager_->IsOutOfOrder();
  }
}

TransactionExecutor::~TransactionExecutor() { Stop(); }

void TransactionExecutor::RegisterExecute(int64_t seq) {
  if (execute_thread_num_ == 1) return;
  int idx = seq % blucket_num_;
  std::unique_lock<std::mutex> lk(mutex_);
  // LOG(ERROR)<<"register seq:"<<seq<<" bluck:"<<blucket_[idx];
  assert(!blucket_[idx] || !(blucket_[idx] ^ 3));
  blucket_[idx] = 1;
  // LOG(ERROR)<<"register seq:"<<seq;
}

void TransactionExecutor::WaitForExecute(int64_t seq) {
  if (execute_thread_num_ == 1) return;
  int pre_idx = (seq - 1 + blucket_num_) % blucket_num_;

  while (!IsStop()) {
    std::unique_lock<std::mutex> lk(mutex_);
    cv_.wait_for(lk, std::chrono::milliseconds(10000), [&] {
      return ((blucket_[pre_idx] & 2) || !blucket_[pre_idx]);
    });
    if ((blucket_[pre_idx] & 2) || !blucket_[pre_idx]) {
      break;
    }
  }
  // LOG(ERROR)<<"wait for :"<<seq<<" done";
}

void TransactionExecutor::FinishExecute(int64_t seq) {
  if (execute_thread_num_ == 1) return;
  int idx = seq % blucket_num_;
  std::unique_lock<std::mutex> lk(mutex_);
  // LOG(ERROR)<<"finish :"<<seq<<" done";
  blucket_[idx] = 3;
  cv_.notify_all();
}

void TransactionExecutor::Stop() {
  stop_ = true;
  if (ordering_thread_.joinable()) {
    ordering_thread_.join();
  }
  for (auto& th : execute_thread_) {
    if (th.joinable()) {
      th.join();
    }
  }
  if (execute_OOO_thread_.joinable()) {
    execute_OOO_thread_.join();
  }
}

Storage* TransactionExecutor::GetStorage() {
  return transaction_manager_ ? transaction_manager_->GetStorage() : nullptr;
}

void TransactionExecutor::SetPreExecuteFunc(PreExecuteFunc pre_exec_func) {
  pre_exec_func_ = pre_exec_func;
}

void TransactionExecutor::SetSeqUpdateNotifyFunc(SeqUpdateNotifyFunc func) {
  seq_update_notify_func_ = func;
}

bool TransactionExecutor::IsStop() { return stop_; }

uint64_t TransactionExecutor::GetMaxPendingExecutedSeq() {
  return next_execute_seq_ - 1;
}

bool TransactionExecutor::NeedResponse() {
  return transaction_manager_ == nullptr ||
         transaction_manager_->NeedResponse();
}

int TransactionExecutor::Commit(std::unique_ptr<Request> message) {
  global_stats_->IncPendingExecute();
  if (transaction_manager_ && transaction_manager_->IsOutOfOrder()) {
    // LOG(ERROR)<<"add out of order exe:"<<message->seq()<<" from
    // proxy:"<<message->proxy_id();
    std::unique_ptr<Request> msg = std::make_unique<Request>(*message);
    execute_OOO_queue_.Push(std::move(message));
    commit_queue_.Push(std::move(msg));
  } else {
    commit_queue_.Push(std::move(message));
  }
  return 0;
}

void TransactionExecutor::AddNewData(std::unique_ptr<Request> message) {
  candidates_.insert(std::make_pair(message->seq(), std::move(message)));
}

std::unique_ptr<Request> TransactionExecutor::GetNextData() {
  if (candidates_.empty() || candidates_.begin()->first != next_execute_seq_) {
    return nullptr;
  }
  auto res = std::move(candidates_.begin()->second);
  if (pre_exec_func_) {
    pre_exec_func_(res.get());
  }
  candidates_.erase(candidates_.begin());
  return res;
}

void TransactionExecutor::OrderMessage() {
  while (!IsStop()) {
    auto message = commit_queue_.Pop();
    if (message != nullptr) {
      global_stats_->IncExecute();
      uint64_t seq = message->seq();
      if (next_execute_seq_ > seq) {
        // LOG(INFO) << "request seq:" << seq << " has been executed"
        // << " next seq:" << next_execute_seq_;
        continue;
      }

      AddNewData(std::move(message));
    }

    while (!IsStop()) {
      std::unique_ptr<Request> message = GetNextData();
      if (message == nullptr) {
        break;
      }
      execute_queue_.Push(std::move(message));
      next_execute_seq_++;
      if (seq_update_notify_func_) {
        seq_update_notify_func_(next_execute_seq_);
      }
    }
  }
  return;
}

void TransactionExecutor::AddExecuteMessage(std::unique_ptr<Request> message) {
    execute_queue_.Push(std::move(message));
}

void TransactionExecutor::ExecuteMessage() {
  while (!IsStop()) {
    auto message = execute_queue_.Pop();
    if (message == nullptr) {
      continue;
    }
    bool need_execute = true;
    if (transaction_manager_ && transaction_manager_->IsOutOfOrder()) {
      need_execute = false;
    }
    Execute(std::move(message), need_execute);
  }
}

void TransactionExecutor::ExecuteMessageOutOfOrder() {
  while (!IsStop()) {
    auto message = execute_OOO_queue_.Pop();
    if (message == nullptr) {
      continue;
    }
    OnlyExecute(std::move(message));
  }
}

void TransactionExecutor::OnlyExecute(std::unique_ptr<Request> request) {
  // Only Execute the request.
  BatchUserRequest batch_request;
  if (!batch_request.ParseFromString(request->data())) {
    LOG(ERROR) << "parse data fail";
  }
  batch_request.set_seq(request->seq());
  batch_request.set_hash(request->hash());
  batch_request.set_proxy_id(request->proxy_id());
  if (request->has_committed_certs()) {
    *batch_request.mutable_committed_certs() = request->committed_certs();
  }

  // LOG(INFO) << " get request batch size:"
  //          << batch_request.user_requests_size()<<" proxy
  //          id:"<<request->proxy_id();
  // std::unique_ptr<BatchUserResponse> batch_response =
  //     std::make_unique<BatchUserResponse>();

  std::unique_ptr<BatchUserResponse> response;
  if (transaction_manager_) {
    response = transaction_manager_->ExecuteBatch(batch_request);
  }

  // global_stats_->IncTotalRequest(batch_request.user_requests_size());
  // global_stats_->IncExecuteDone();
}

void TransactionExecutor::Execute(std::unique_ptr<Request> request,
                                  bool need_execute) {
  // Execute the request, then send the response back to the user.
  RegisterExecute(request->seq());
  BatchUserRequest batch_request;
  if (!batch_request.ParseFromString(request->data())) {
    LOG(ERROR) << "parse data fail";
  }
  batch_request.set_seq(request->seq());
  batch_request.set_hash(request->hash());
  batch_request.set_proxy_id(request->proxy_id());
  if (request->has_committed_certs()) {
    *batch_request.mutable_committed_certs() = request->committed_certs();
  }

  //LOG(INFO) << " get request batch size:"
  //         << batch_request.user_requests_size()
  //         <<" proxy id:"<<request->proxy_id()<<" need execute:"<<need_execute<<" create time:"<<batch_request.createtime()<<" current time:"<<GetCurrentTime();
  // std::unique_ptr<BatchUserResponse> batch_response =
  //     std::make_unique<BatchUserResponse>();
  
  std::unique_ptr<BatchUserResponse> response;
  if (transaction_manager_ && need_execute) {
    if (execute_thread_num_ == 1) {
      response = transaction_manager_->ExecuteBatch(batch_request);
    } else {
      std::unique_ptr<std::vector<std::unique_ptr<google::protobuf::Message>>>
        request_v = transaction_manager_->Prepare(batch_request);
    
      assert(request_v!=nullptr);
      WaitForExecute(request->seq());
      std::vector<std::unique_ptr<std::string>> ret = 
        transaction_manager_->ExecuteBatchData(*request_v);
      //LOG(ERROR)<<"execute seq:"<<request->seq();
      FinishExecute(request->seq());

      response = std::make_unique<BatchUserResponse>();
      for (auto& s : ret) {
        response->add_response()->swap(*s);
      }
    }
  }

  if (duplicate_manager_) {
    duplicate_manager_->AddExecuted(batch_request.hash(), batch_request.seq());
  }

  global_stats_->IncTotalRequest(batch_request.user_requests_size());
  if (response == nullptr) {
    response = std::make_unique<BatchUserResponse>();
  }

  response->set_createtime(batch_request.createtime());
  response->set_local_id(batch_request.local_id());
  response->set_hash(batch_request.hash());
  response->set_proxy_id(request->proxy_id());

  post_exec_func_(std::move(request), std::move(response));

  global_stats_->IncExecuteDone();
}

void TransactionExecutor::SetDuplicateManager(DuplicateManager* manager) {
  duplicate_manager_ = manager;
}

}  // namespace resdb

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

#include "platform/consensus/execution/transaction_executor.h"

#include <glog/logging.h>

namespace resdb {

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
  global_stats_ = Stats::GetGlobalStats();
  ordering_thread_ = std::thread(&TransactionExecutor::OrderMessage, this);
  execute_thread_ = std::thread(&TransactionExecutor::ExecuteMessage, this);

  if (transaction_manager_ && transaction_manager_->IsOutOfOrder()) {
    execute_OOO_thread_ =
        std::thread(&TransactionExecutor::ExecuteMessageOutOfOrder, this);
    LOG(ERROR) << " is out of order:" << transaction_manager_->IsOutOfOrder();
  }
}

TransactionExecutor::~TransactionExecutor() { Stop(); }

void TransactionExecutor::Stop() {
  stop_ = true;
  if (ordering_thread_.joinable()) {
    ordering_thread_.join();
  }
  if (execute_thread_.joinable()) {
    execute_thread_.join();
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
  global_stats_->GetTransactionDetails(batch_request);
  if (transaction_manager_) {
    response = transaction_manager_->ExecuteBatch(batch_request);
  }

  // global_stats_->IncTotalRequest(batch_request.user_requests_size());
  // global_stats_->IncExecuteDone();
}

void TransactionExecutor::Execute(std::unique_ptr<Request> request,
                                  bool need_execute) {
  // Execute the request, then send the response back to the user.
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
  //         << batch_request.user_requests_size()<<" proxy
  //         id:"<<request->proxy_id()<<" need execute:"<<need_execute;
  // std::unique_ptr<BatchUserResponse> batch_response =
  //     std::make_unique<BatchUserResponse>();

  std::unique_ptr<BatchUserResponse> response;
  global_stats_->GetTransactionDetails(batch_request);
  if (transaction_manager_ && need_execute) {
    response = transaction_manager_->ExecuteBatch(batch_request);
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

  post_exec_func_(std::move(request), std::move(response));

  global_stats_->IncExecuteDone();
}

void TransactionExecutor::SetDuplicateManager(DuplicateManager* manager) {
  duplicate_manager_ = manager;
}

}  // namespace resdb

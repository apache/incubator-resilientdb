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

#include "execution/transaction_executor.h"

#include <glog/logging.h>

namespace resdb {

TransactionExecutor::TransactionExecutor(
    const ResDBConfig& config, PostExecuteFunc post_exec_func,
    SystemInfo* system_info,
    std::unique_ptr<TransactionExecutorImpl> executor_impl)
    : config_(config),
      post_exec_func_(post_exec_func),
      system_info_(system_info),
      executor_impl_(std::move(executor_impl)),
      commit_queue_("order"),
      execute_queue_("execute"),
      stop_(false) {
  global_stats_ = Stats::GetGlobalStats();
  ordering_thread_ = std::thread(&TransactionExecutor::OrderMessage, this);
  execute_thread_ = std::thread(&TransactionExecutor::ExecuteMessage, this);

  if (executor_impl_ && executor_impl_->IsOutOfOrder()) {
    execute_OOO_thread_ =
        std::thread(&TransactionExecutor::ExecuteMessageOutOfOrder, this);
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

int TransactionExecutor::Commit(std::unique_ptr<Request> message) {
  global_stats_->IncPendingExecute();
  if (executor_impl_ && executor_impl_->IsOutOfOrder()) {
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
        LOG(INFO) << "request seq:" << seq << " has been executed"
                  << " next seq:" << next_execute_seq_;
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
    if (executor_impl_ && executor_impl_->IsOutOfOrder()) {
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
  // Execute the request on user size, then send the response back to the
  // client.
  BatchClientRequest batch_request;
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
  //          << batch_request.client_requests_size()<<" proxy
  //          id:"<<request->proxy_id();
  std::unique_ptr<BatchClientResponse> batch_response =
      std::make_unique<BatchClientResponse>();

  std::unique_ptr<BatchClientResponse> response;
  if (executor_impl_) {
    response = executor_impl_->ExecuteBatch(batch_request);
  }

}

void TransactionExecutor::Execute(std::unique_ptr<Request> request,
                                  bool need_execute) {
  // Execute the request on user size, then send the response back to the
  // client.
  BatchClientRequest batch_request;
  if (!batch_request.ParseFromString(request->data())) {
    LOG(ERROR) << "parse data fail";
  }
  batch_request.set_seq(request->seq());
  batch_request.set_hash(request->hash());
  batch_request.set_proxy_id(request->proxy_id());
  if (request->has_committed_certs()) {
    *batch_request.mutable_committed_certs() = request->committed_certs();
  }

  std::unique_ptr<BatchClientResponse> batch_response =
      std::make_unique<BatchClientResponse>();

  std::unique_ptr<BatchClientResponse> response;
  if (executor_impl_ && need_execute) {
    response = executor_impl_->ExecuteBatch(batch_request);
  }

  global_stats_->IncTotalRequest(batch_request.client_requests_size());
  if (executor_impl_ == nullptr || executor_impl_->NeedResponse()) {
    if (response == nullptr) {
      response = std::make_unique<BatchClientResponse>();
    }

    response->set_createtime(batch_request.createtime());
    response->set_local_id(batch_request.local_id());

    post_exec_func_(std::move(request), std::move(response));
  }

  global_stats_->IncExecuteDone();
}

}  // namespace resdb

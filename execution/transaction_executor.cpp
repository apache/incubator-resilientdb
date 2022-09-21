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
}

TransactionExecutor::~TransactionExecutor() {
  stop_ = true;
  if (ordering_thread_.joinable()) {
    ordering_thread_.join();
  }
  if (execute_thread_.joinable()) {
    execute_thread_.join();
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
  commit_queue_.Push(std::move(message));
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
    Execute(std::move(message));
  }
}

void TransactionExecutor::Execute(std::unique_ptr<Request> request) {
  // Execute the request on user size, then send the response back to the
  // client.
  BatchClientRequest batch_request;
  if (!batch_request.ParseFromString(request->data())) {
    LOG(ERROR) << "parse data fail";
  }

  // LOG(INFO) << " get request batch size:"
  //        << batch_request.client_requests_size();
  std::unique_ptr<BatchClientResponse> batch_response =
      std::make_unique<BatchClientResponse>();

  std::unique_ptr<BatchClientResponse> response;
  if (executor_impl_) {
    response = executor_impl_->ExecuteBatch(batch_request);
  }

  if (response == nullptr) {
    response = std::make_unique<BatchClientResponse>();
  }

  global_stats_->IncTotalRequest(batch_request.client_requests_size());

  response->set_createtime(batch_request.createtime());
  response->set_local_id(batch_request.local_id());

  post_exec_func_(std::move(request), std::move(response));

  global_stats_->IncExecuteDone();
}

}  // namespace resdb

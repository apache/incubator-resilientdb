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
      stop_(false) {
  memset(blucket_, 0, sizeof(blucket_));

  last_group_ = 0;

  global_stats_ = Stats::GetGlobalStats();
  ordering_thread_ = std::thread(&TransactionExecutor::OrderMessage, this);
  transaction_manager_->SetAsyncCallback(
      [&](const BatchUserRequest& batch_request,
          std::unique_ptr<Request> request,
          std::unique_ptr<BatchUserResponse> response) {
        CallBack(batch_request, std::move(request), std::move(response));
      });

  for (int i = 0; i < execute_thread_num_; ++i) {
    execute_thread_.push_back(
        std::thread(&TransactionExecutor::ExecuteMessage, this));
  }

  if (transaction_manager_ && transaction_manager_->IsOutOfOrder()) {
    execute_OOO_thread_ =
        std::thread(&TransactionExecutor::ExecuteMessageOutOfOrder, this);
    // LOG(ERROR) << " is out of order:" <<
    // transaction_manager_->IsOutOfOrder();
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
  if (blucket_num_ == 1) return;
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
  for (auto& th : prepare_thread_) {
    if (th.joinable()) {
      th.join();
    }
  }

  if (execute_OOO_thread_.joinable()) {
    execute_OOO_thread_.join();
  }
  if (gc_thread_.joinable()) {
    gc_thread_.join();
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

void TransactionExecutor::AddExecuteMessage(std::unique_ptr<Request> message) {
  global_stats_->IncCommit();
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
    int64_t start_time = GetCurrentTime();
    Execute(std::move(message), need_execute);
    int64_t end_time = GetCurrentTime();
    if (end_time - start_time > 500) {
      // LOG(ERROR)<<" execute data time:"<<(end_time-start_time);
    }
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
  std::unique_ptr<BatchUserResponse> batch_response =
      std::make_unique<BatchUserResponse>();

  std::unique_ptr<BatchUserResponse> response;
  if (transaction_manager_) {
    response = transaction_manager_->ExecuteBatch(batch_request);
  }

  // global_stats_->IncTotalRequest(batch_request.user_requests_size());
  // global_stats_->IncExecuteDone();
}

void TransactionExecutor::CallBack(
    const BatchUserRequest& batch_request, std::unique_ptr<Request> request,
    std::unique_ptr<BatchUserResponse> response) {
  if (response == nullptr) {
    response = std::make_unique<BatchUserResponse>();
  }
  // LOG(ERROR)<<" call back ????"<<" skip:"<<request->skip();
  if (!request->skip()) {
    global_stats_->IncTotalRequest(batch_request.user_requests_size());
  }
  response->set_proxy_id(batch_request.proxy_id());
  response->set_createtime(batch_request.createtime());
  response->set_local_id(batch_request.local_id());

  assert(request != nullptr);
  response->set_seq(request->seq());

  // LOG(ERROR)<<" execute done:"<<response->proxy_id()<<" local
  // id:"<<response->local_id()<<" from async"<<" seq:"<<response->seq();

  if (post_exec_func_) {
    post_exec_func_(std::move(request), std::move(response));
  }

  global_stats_->IncExecuteDone();
  // LOG(ERROR)<<" execute done:";
}

void TransactionExecutor::Prepare(std::unique_ptr<Request> request) {}

void TransactionExecutor::Execute(std::unique_ptr<Request> request,
                                  bool need_execute) {
  global_stats_->IncExecute();
  BatchUserRequest batch_request;
  request->set_execute_time(GetCurrentTime());
  int shard_id = request->shard_id();
  // assert(shard_id > 0);
  // LOG(ERROR)<<" execute:"<<request->seq()<<" shard:"<<shard_id;
  RegisterExecute(request->seq());
  int64_t seq = request->seq();

  // Execute the request, then send the response back to the user.
  if (!batch_request.ParseFromString(request->data())) {
    LOG(ERROR) << "parse data fail";
    return;
  }
  batch_request.set_hash(request->hash());
  if (request->has_committed_certs()) {
    *batch_request.mutable_committed_certs() = request->committed_certs();
  }
  batch_request.set_seq(request->seq());
  batch_request.set_proxy_id(request->proxy_id());

  std::unique_ptr<BatchUserResponse> batch_response =
      std::make_unique<BatchUserResponse>();

  std::unique_ptr<BatchUserResponse> response;
  // need_execute = false;
  // LOG(ERROR)<<" skip exe:"<<request->skip();
  if (request->skip()) {
    WaitForExecute(seq);
    FinishExecute(seq);
    CallBack(batch_request, std::move(request), nullptr);
    return;
  }

  if (transaction_manager_ && need_execute) {
    if (execute_thread_num_ == 1) {
      response = transaction_manager_->ExecuteBatch(batch_request);

      if (request->flag() > 0) {
        if (user_func_) {
          user_func_(request->flag());
        }
      }

    } else {
      std::unique_ptr<std::vector<std::unique_ptr<google::protobuf::Message>>>
          prepare_v = transaction_manager_->Prepare(batch_request);

      int flag = request->flag();

      WaitForExecute(seq);
      // LOG(ERROR)<<" exe seq:"<<seq<<" shard:"<<request->shard_id();
      transaction_manager_->Attach(std::move(request));
      response = transaction_manager_->ExecutePreparedData(batch_request);
      // LOG(ERROR)<<" exe seq done:"<<seq;

      if (response == nullptr || !response->is_async()) {
        request = transaction_manager_->FetchRequest();
      }
      if (flag > 0) {
        if (user_func_) {
          user_func_(flag);
        }
      }

      FinishExecute(seq);
      // LOG(ERROR)<<" finish seq done:"<<seq;
    }
  }

  if (response == nullptr) {
    response = std::make_unique<BatchUserResponse>();
  }
  // LOG(ERROR)<<"create time is async:"<<response->is_async();
  /*
  if(request->create_time()>0){
    int64_t done_time = GetCurrentTime();
    int64_t commit_time = request->commit_time() - request->create_time();
    int64_t execute_time = done_time - request->execute_time();
    int64_t execute_lat = done_time - request->commit_time();
    global_stats_->AddCommitLatency(commit_time);
    global_stats_->AddExecuteTime(execute_time);
    global_stats_->AddExecuteLatency(execute_lat);
    LOG(ERROR)<<" commit lat:"<<commit_time<<" exe time:"<<execute_time<<" exe
  lat:"<<execute_lat;
  }
  */

  if (response->is_async()) {
  } else {
    global_stats_->IncTotalRequest(batch_request.user_requests_size());
    response->set_proxy_id(batch_request.proxy_id());
    response->set_createtime(batch_request.createtime());
    response->set_local_id(batch_request.local_id());

    response->set_seq(request->seq());

    // LOG(ERROR)<<" execute done:"<<response->proxy_id()<<" local
    // id:"<<response->local_id();

    if (post_exec_func_) {
      post_exec_func_(std::move(request), std::move(response));
    }

    global_stats_->IncExecuteDone();
  }
}

}  // namespace resdb

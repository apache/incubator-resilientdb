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
#include "platform/consensus/ordering/thunderbolt/executor/paral_sm/xexecutor.h"

#include <future>
#include <queue>

#include "common/utils/utils.h"
#include "eEVM/processor.h"
#include "glog/logging.h"
#include "platform/consensus/ordering/thunderbolt/executor/paral_sm/local_state.h"

namespace resdb {
namespace contract {
namespace paral_sm {

XExecutor::XExecutor(
    DataStorage* storage, GlobalState* global_state, int window_size,
    std::function<void(std::unique_ptr<ExecuteResp>)> call_back, int worker_num)
    : storage_(storage),
      gs_(global_state),
      worker_num_(worker_num),
      window_size_(window_size),
      call_back_(call_back) {
  // LOG(ERROR)<<"init window:"<<window_size<<" worker:"<<worker_num_;
  controller_ = std::make_unique<XController>(storage, window_size * 2);
  executor_ = std::make_unique<ContractExecutor>();

  resp_list_.resize(window_size_);
  is_done_.resize(window_size_);
  for (int i = 0; i < window_size_; ++i) {
    is_done_[i] = false;
    resp_list_[i] = nullptr;
  }

  first_id_ = 0;
  last_id_ = 1;
  is_stop_ = false;
  id_ = 1;

  for (int i = 0; i < worker_num_; ++i) {
    workers_.push_back(std::thread([&]() {
      while (!is_stop_) {
        auto request = request_queue_.Pop();
        if (request == nullptr) {
          continue;
        }

        LocalState local_state(controller_.get());
        local_state.Set(
            gs_->GetAccount(
                request->GetContractExecuteInfo()->contract_address),
            request->GetContractExecuteInfo()->commit_id);

        std::unique_ptr<ExecuteResp> resp = std::make_unique<ExecuteResp>();
        auto ret = ExecContract(
            request->GetContractExecuteInfo()->caller_address,
            request->GetContractExecuteInfo()->contract_address,
            request->GetContractExecuteInfo()->func_addr,
            request->GetContractExecuteInfo()->func_params, &local_state);
        resp->state = ret.status();
        resp->contract_address =
            request->GetContractExecuteInfo()->contract_address;
        resp->commit_id = request->GetContractExecuteInfo()->commit_id;
        resp->user_id = request->GetContractExecuteInfo()->user_id;
        // LOG(ERROR)<<"=========   get resp commit
        // id:"<<request->GetContractExecuteInfo()->commit_id<<" param:"<<
        // request->GetContractExecuteInfo()->func_params.DebugString();
        if (ret.ok()) {
          resp->ret = 0;
          resp->result = *ret;
          if (request->IsRedo()) {
            resp->retry_time = request->RedoTime();
          }
          local_state.Flesh(request->GetContractExecuteInfo()->contract_address,
                            request->GetContractExecuteInfo()->commit_id);
        } else {
          LOG(ERROR) << "commit :" << resp->commit_id << " fail";
          resp->ret = -1;
          assert(resp->ret >= 0);
        }
        request->SetResult(std::move(resp));
        resp_queue_.Push(std::move(request));
      }
    }));
  }

  response_ = std::thread([&]() {
    while (!is_stop_) {
      ResponseProcess();
    }
  });
}

void XExecutor::SetExecuteCallBack(
    std::function<void(std::unique_ptr<ExecuteResp>)> func) {
  call_back_ = std::move(func);
}

XExecutor::~XExecutor() {
  // LOG(ERROR)<<"desp";
  is_stop_ = true;
  for (int i = 0; i < worker_num_; ++i) {
    workers_[i].join();
  }
  if (response_.joinable()) {
    response_.join();
  }
}

void XExecutor::AddTask(int64_t commit_id,
                        std::unique_ptr<ExecutionContext> context) {
  context_list_[commit_id] = std::move(context);
}

void XExecutor::RemoveTask(int64_t commit_id) {
  auto it = context_list_.find(commit_id);
  if (it != context_list_.end()) {
    context_list_.erase(it);
  }
}

ExecutionContext* XExecutor::GetTaskContext(int64_t commit_id) {
  auto it = context_list_.find(commit_id);
  return it == context_list_.end() ? nullptr : it->second.get();
}

void XExecutor::CallBack(uint64_t commit_id) {
  const int idx = commit_id % window_size_;
  if (call_back_) {
    call_back_(std::move(resp_list_[idx]));
  } else {
    resp_list_[idx].reset();
  }
  is_done_[idx] = true;
  first_id_ = std::max(first_id_.load(), commit_id);
  cv_.notify_all();
  RemoveTask(commit_id);
}

bool XExecutor::WaitNext() {
  std::unique_lock<std::mutex> lock(mutex_);
  return cv_.wait_for(lock, std::chrono::seconds(10), [&] {
           return is_stop_ ||
                  id_ - first_id_ < static_cast<uint64_t>(window_size_);
         }) &&
         !is_stop_;
}

bool XExecutor::WaitAll() {
  std::unique_lock<std::mutex> lock(mutex_);
  return cv_.wait_for(lock, std::chrono::seconds(10), [&] {
           return is_stop_ || first_id_ >= id_;
         }) &&
         !is_stop_;
}

void XExecutor::ResponseProcess() {
  auto context = resp_queue_.Pop();
  if (context == nullptr) {
    return;
  }

  std::unique_ptr<ExecuteResp> result = context->FetchResult();
  if (result == nullptr) {
    return;
  }

  const int64_t commit_id = result->commit_id;
  resp_list_[commit_id % window_size_] = std::move(result);

  if (!controller_->Commit(commit_id)) {
    auto& redo = controller_->GetRedo();
    if (!redo.empty() && redo.front() == commit_id) {
      ExecutionContext* original = GetTaskContext(commit_id);
      if (original != nullptr) {
        original->SetRedo();
        request_queue_.Push(std::make_unique<ExecutionContext>(
            *original->GetContractExecuteInfo()));
      }
    }
  }

  for (int64_t done_id : controller_->GetDone()) {
    CallBack(done_id);
  }
}

void XExecutor::AsyncExecContract(std::vector<ContractExecuteInfo>& requests) {
  for (auto& request : requests) {
    if (!WaitNext()) {
      return;
    }
    AddTask(request.commit_id, std::make_unique<ExecutionContext>(request));
    request_queue_.Push(std::make_unique<ExecutionContext>(request));
    ++id_;
  }

  return;
}

absl::StatusOr<std::string> XExecutor::ExecContract(
    const Address& caller_address, const Address& contract_address,
    const std::string& func_addr, const Params& func_param, EVMState* state) {
  return executor_->ExecContract(caller_address, contract_address, func_addr,
                                 func_param, state);
}

}  // namespace paral_sm
}  // namespace contract
}  // namespace resdb

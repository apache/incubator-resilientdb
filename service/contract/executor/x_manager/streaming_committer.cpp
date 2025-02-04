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

#include "service/contract/executor/x_manager/streaming_committer.h"

#include <future>
#include <queue>

#include "common/utils/utils.h"
#include "eEVM/processor.h"
#include "glog/logging.h"
#include "service/contract/executor/x_manager/executor_state.h"

//#define DEBUG

namespace resdb {
namespace contract {
namespace x_manager {

StreamingCommitter::StreamingCommitter(DataStorage* storage,
                                       GlobalState* global_state,
                                       std::function<void(int)> call_back,
                                       int worker_num)
    : storage_(storage),
      gs_(global_state),
      worker_num_(worker_num),
      call_back_(call_back) {
  // LOG(ERROR)<<"init window:"<<window_size<<" worker:"<<worker_num_;
  controller_ = std::make_unique<StreamingController>(storage);
  executor_ = std::make_unique<ContractExecutor>();

  is_stop_ = false;
  id_ = 1;
  for (int i = 0; i < worker_num_; ++i) {
    workers_.push_back(std::thread([&]() {
      while (!is_stop_) {
        auto request_ptr = request_queue_.Pop();
        if (request_ptr == nullptr) {
          continue;
        }
        ExecutionContext* request = *request_ptr;

        ExecutorState executor_state(
            controller_.get(), request->GetContractExecuteInfo()->commit_id);
        executor_state.Set(
            gs_->GetAccount(
                request->GetContractExecuteInfo()->contract_address),
            request->GetContractExecuteInfo()->commit_id, request->RedoTime());

        std::unique_ptr<ExecuteResp> resp = std::make_unique<ExecuteResp>();
        // LOG(ERROR)<<" exe ======";
        auto ret = ExecContract(
            request->GetContractExecuteInfo()->caller_address,
            request->GetContractExecuteInfo()->contract_address,
            request->GetContractExecuteInfo()->func_addr,
            request->GetContractExecuteInfo()->func_params, &executor_state);
        resp->state = ret.status();
        resp->contract_address =
            request->GetContractExecuteInfo()->contract_address;
        resp->commit_id = request->GetContractExecuteInfo()->commit_id;
        resp->user_id = request->GetContractExecuteInfo()->user_id;
        // LOG(ERROR)<<" exe done ======:"<<resp->commit_id<<"
        // ret.ok():"<<ret.ok();
        if (ret.ok()) {
          resp->ret = 0;
          resp->result = *ret;
        } else {
          // LOG(ERROR)<<"commit :"<<resp->commit_id<<" fail";
          resp->ret = -1;
          assert(resp->ret >= 0);
        }
        resp_queue_.Push(std::move(resp));
      }
    }));
  }

  response_ = std::thread([&]() {
    while (!is_stop_) {
      ResponseProcess();
    }
  });
}

void StreamingCommitter::SetExecuteCallBack(std::function<void(int)> func) {
  call_back_ = std::move(func);
}

StreamingCommitter::~StreamingCommitter() {
  is_stop_ = true;
  for (int i = 0; i < worker_num_; ++i) {
    workers_[i].join();
  }
  if (response_.joinable()) {
    response_.join();
  }
}

void StreamingCommitter::AddTask(int64_t commit_id,
                                 std::unique_ptr<ExecutionContext> context) {
  assert(context_list_.find(commit_id) == context_list_.end());
  context_list_[commit_id] = std::move(context);
}

void StreamingCommitter::RemoveTask(int64_t commit_id) {
  assert(context_list_.find(commit_id) != context_list_.end());
  context_list_.erase(context_list_.find(commit_id));
}

ExecutionContext* StreamingCommitter::GetTask(int64_t commit_id) {
  return context_list_[commit_id].get();
}

void StreamingCommitter::CallBack(uint64_t exe_id) {
  if (call_back_) {
    call_back_(exe_id);
  }
}

void StreamingCommitter::ResponseProcess() {
  auto resp = resp_queue_.Pop();
  if (resp == nullptr) {
    return;
  }
  int64_t resp_commit_id = resp->commit_id;
  // LOG(ERROR)<<" resp done:"<<resp_commit_id;
  RemoveNode(resp_commit_id);

  bool call = false;

  int exe_id;
  {
    std::unique_lock<std::mutex> lk(req_mutex_);
    exe_id = req_id_[resp->commit_id];
    resp_list_[exe_id]--;
    if (resp_list_[exe_id] == 0) {
      resp_list_.erase(resp_list_.find(exe_id));
      call = true;
    }
    req_id_.erase(req_id_.find(resp_commit_id));
  }
  if (call) {
    // LOG(ERROR)<<" done:"<<exe_id;
    CallBack(exe_id);
  }
  RemoveTask(resp_commit_id);
}

void StreamingCommitter::RemoveNode(int idx) {
  std::unique_lock<std::mutex> lk(g_mutex_);
  for (int d : g_[idx]) {
    din_[d]--;
    if (din_[d] == 0) {
      auto context_ptr = GetTask(d);
      // LOG(ERROR)<<" add task:"<<d;
      request_queue_.Push(std::make_unique<ExecutionContext*>(context_ptr));
    }
  }

  int64_t groups = groups_[idx];
  assert(groups > 0);
  for (int i = 0; i < 64; ++i) {
    if (groups & (1ll << i)) {
      // LOG(ERROR)<<" remove idx :"<<idx<<" from group:"<<i;
      assert(group_[i].front() == idx);
      group_[i].pop();
    }
  }

  g_.erase(g_.find(idx));
  din_.erase(din_.find(idx));
  groups_.erase(groups_.find(idx));
}

void StreamingCommitter::AddEdge(int s_id, int e_idx) {
  // LOG(ERROR)<<" add edge:"<<s_id<<"  end id:"<<e_idx;
  // assert(s_id<4);
  if (group_[s_id].empty()) {
  } else {
    int idx = group_[s_id].back();
    // LOG(ERROR)<<" add edge from:"<<idx<<"  to id:"<<e_idx;
    g_[idx].push_back(e_idx);
    din_[e_idx]++;
  }
  group_[s_id].push(e_idx);
}

void StreamingCommitter::Notify(int idx) {
  if (din_[idx] == 0) {
    auto context_ptr = GetTask(idx);
    // LOG(ERROR)<<" notify:"<<idx;
    request_queue_.Push(std::make_unique<ExecutionContext*>(context_ptr));
  }
}

void StreamingCommitter::AsyncExecContract(
    int exe_id,
    std::vector<std::pair<ContractExecuteInfo, int64_t> >& requests) {
  {
    std::unique_lock<std::mutex> lk(req_mutex_);
    resp_list_[exe_id] = requests.size();
  }

  for (auto& it : requests) {
    auto& request = it.first;
    int64_t groups = it.second;
    int cur_idx = id_;
    request.commit_id = id_++;
    auto context = std::make_unique<ExecutionContext>(request);
    AddTask(cur_idx, std::move(context));
    {
      std::unique_lock<std::mutex> lk(req_mutex_);
      req_id_[request.commit_id] = exe_id;
    }

    {
      std::unique_lock<std::mutex> lk(g_mutex_);
      for (int i = 0; i < 64; ++i) {
        if (groups & (1ll << i)) {
          AddEdge(i, cur_idx);
        }
      }
      groups_[cur_idx] = groups;
      Notify(cur_idx);
    }
    // request_queue_.Push(std::make_unique<ExecutionContext*>(context_ptr));
  }

  return;
}

absl::StatusOr<std::string> StreamingCommitter::ExecContract(
    const Address& caller_address, const Address& contract_address,
    const std::string& func_addr, const Params& func_param, EVMState* state) {
  // LOG(ERROR)<<"start:"<<caller_address;
  return executor_->ExecContract(caller_address, contract_address, func_addr,
                                 func_param, state);
}

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb

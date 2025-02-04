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

#include "service/contract/executor/x_manager/fx_committer.h"

#include <future>
#include <queue>

#include "common/utils/utils.h"
#include "eEVM/exception.h"
#include "eEVM/processor.h"
#include "glog/logging.h"
#include "service/contract/executor/x_manager/executor_state.h"

//#define Debug

namespace resdb {
namespace contract {
namespace x_manager {

namespace {

class TimeTrack {
 public:
  TimeTrack(std::string name = "") {
    name_ = name;
    start_time_ = GetCurrentTime();
  }

  ~TimeTrack() {
    uint64_t end_time = GetCurrentTime();
    // LOG(ERROR) << name_ <<" run:" << (end_time - start_time_)<<"ms";
  }

  double GetRunTime() {
    uint64_t end_time = GetCurrentTime();
    return (end_time - start_time_) / 1000000.0;
  }

 private:
  std::string name_;
  uint64_t start_time_;
};

}  // namespace

FXCommitter::FXCommitter(DataStorage* storage, GlobalState* global_state,
                         int window_size, int worker_num)
    : gs_(global_state), worker_num_(worker_num), window_size_(window_size) {
  // LOG(ERROR)<<"init window:"<<window_size<<" worker:"<<worker_num_;
  controller_ = std::make_unique<FXController>(storage, window_size);
  // controller_ = std::make_unique<StreamingFXController>(storage,
  // window_size*2);
  executor_ = std::make_unique<ContractExecutor>();

  resp_list_.resize(window_size_);
  is_done_.resize(window_size_);
  for (int i = 0; i < window_size_; ++i) {
    is_done_[i] = false;
    resp_list_[i] = nullptr;
  }

  // context_list_.resize(window_size_);
  // tmp_resp_list_.resize(window_size_);
  first_id_ = 0;
  last_id_ = 1;
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
        if (request->GetContractExecuteInfo()->func_params.is_only()) {
          controller_->SetOnly(request->GetContractExecuteInfo()->commit_id);
        }

        TimeTrack track;
        ExecutorState executor_state(
            controller_.get(), request->GetContractExecuteInfo()->commit_id);
        executor_state.Set(
            gs_->GetAccount(
                request->GetContractExecuteInfo()->contract_address),
            request->GetContractExecuteInfo()->commit_id, request->RedoTime());

        std::unique_ptr<ExecuteResp> resp = std::make_unique<ExecuteResp>();
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
        if (ret.ok()) {
          resp->ret = 0;
          resp->result = *ret;
          if (request->IsRedo()) {
            resp->retry_time = request->RedoTime();
          }
          resp->runtime = track.GetRunTime() * 1000;
        } else {
          // LOG(ERROR)<<"commit :"<<resp->commit_id<<" fail";
          resp->ret = -1;
        }
        resp_queue_.Push(std::move(resp));
      }
    }));
  }
}

FXCommitter::~FXCommitter() {
  // LOG(ERROR)<<"desp";
  is_stop_ = true;
  for (int i = 0; i < worker_num_; ++i) {
    workers_[i].join();
  }
}

void FXCommitter::AddTask(int64_t commit_id,
                          std::unique_ptr<ExecutionContext> context) {
  context_list_[commit_id] = std::move(context);
}

ExecutionContext* FXCommitter::GetTaskContext(int64_t commit_id) {
  return context_list_[commit_id].get();
}

void FXCommitter::AsyncExecContract(
    std::vector<ContractExecuteInfo>& requests) {
  return;
}

std::vector<std::unique_ptr<ExecuteResp>> FXCommitter::ExecContract(
    std::vector<ContractExecuteInfo>& requests) {
  // LOG(ERROR)<<"request size:"<<requests.size();
  controller_->Clear();
  int id = 1;
  std::vector<std::unique_ptr<ExecuteResp>> tmp_resp_list;
  for (auto& request : requests) {
    request.commit_id = id++;
    auto context = std::make_unique<ExecutionContext>(request);
    auto context_ptr = context.get();
    context_ptr->start_time = GetCurrentTime();

    AddTask(request.commit_id, std::move(context));
    request_queue_.Push(std::make_unique<ExecutionContext*>(context_ptr));
    tmp_resp_list.push_back(nullptr);
  }
  // LOG(ERROR)<<"wait:"<<requests.size();

  tmp_resp_list.push_back(nullptr);
  std::vector<std::unique_ptr<ExecuteResp>> resp_list;

  int process_num = id - 1;
  // LOG(ERROR)<<"wait num:"<<process_num;
  while (process_num > 0) {
    auto resp = resp_queue_.Pop();
    if (resp == nullptr) {
      continue;
    }

    int64_t resp_id = resp->commit_id;
    int ret = resp->ret;
#ifdef Debug
    LOG(ERROR) << "resp:" << resp_id << " list size:" << tmp_resp_list.size()
               << " ret:" << ret;
#endif
    tmp_resp_list[resp_id] = std::move(resp);

    if (ret == 0) {
#ifdef Debug
      LOG(ERROR) << "commit:" << resp_id;
#endif
      bool commit_ret = controller_->Commit(resp_id);
      if (!commit_ret) {
        controller_->Clear(resp_id);
        auto context_ptr = GetTaskContext(resp_id);
        context_ptr->SetRedo();
#ifdef Debug
        LOG(ERROR) << "redo:" << resp_id;
#endif
        request_queue_.Push(std::make_unique<ExecutionContext*>(context_ptr));
      }
      const auto& redo_list = controller_->GetRedo();
      for (int64_t id : redo_list) {
        controller_->Clear(id);
        auto context_ptr = GetTaskContext(id);
        context_ptr->SetRedo();
#ifdef Debug
        LOG(ERROR) << "redo:" << id;
#endif
        request_queue_.Push(std::make_unique<ExecutionContext*>(context_ptr));
      }

      const auto& done_list = controller_->GetDone();
      for (int id : done_list) {
        tmp_resp_list[id]->rws = controller_->GetChangeList(id);
        tmp_resp_list[id]->delay = controller_->GetDelay(id) / 1000.0;
        // LOG(ERROR)<<"done :"<<id<<" wrs
        // size:"<<tmp_resp_list[id]->rws.size()<<"
        // retry:"<<tmp_resp_list[id]->retry_time;
        resp_list.push_back(std::move(tmp_resp_list[id]));
        process_num--;
        continue;
      }
    } else {
      controller_->Clear(resp_id);
      auto context_ptr = GetTaskContext(resp_id);
      context_ptr->SetRedo();
#ifdef Debug
      LOG(ERROR) << "redo :" << resp_id;
#endif
      request_queue_.Push(std::make_unique<ExecutionContext*>(context_ptr));
    }
  }
  // LOG(ERROR)<<"exe done";
  return resp_list;
  //  LOG(ERROR)<<"last id:"<<last_id_;
}

absl::StatusOr<std::string> FXCommitter::ExecContract(
    const Address& caller_address, const Address& contract_address,
    const std::string& func_addr, const Params& func_param, EVMState* state) {
  // LOG(ERROR)<<"start:"<<caller_address;
  return executor_->ExecContract(caller_address, contract_address, func_addr,
                                 func_param, state);
}

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb

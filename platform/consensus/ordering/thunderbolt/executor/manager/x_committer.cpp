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

#include "service/contract/executor/manager/x_committer.h"

#include <queue>

#include "common/utils/utils.h"
#include "eEVM/processor.h"
#include "glog/logging.h"
#include "service/contract/executor/manager/local_state.h"

namespace resdb {
namespace contract {

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

XCommitter::XCommitter(DataStorage* storage, GlobalState* global_state,
                       int worker_num)
    : storage_(storage), gs_(global_state), worker_num_(worker_num) {
  controller_ = std::make_unique<XController>(storage);
  executor_ = std::make_unique<ContractExecutor>();
  is_stop_ = false;

  for (int i = 0; i < worker_num_; ++i) {
    workers_.push_back(std::thread([&]() {
      while (!is_stop_) {
        auto request_ptr = request_queue_.Pop();
        if (request_ptr == nullptr) {
          continue;
        }
        TimeTrack track;
        ExecutionContext* request = *request_ptr;

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
        if (ret.ok()) {
          resp->ret = 0;
          resp->result = *ret;
          if (request->IsRedo()) {
            resp->retry_time = request->RedoTime();
          }
          local_state.Flesh(request->GetContractExecuteInfo()->contract_address,
                            request->GetContractExecuteInfo()->commit_id);
          resp->runtime = track.GetRunTime() * 1000;
          // if(resp->retry_time>1){
          // LOG(ERROR)<<"runtime:"<<resp->runtime;
          //}
        } else {
          resp->ret = -1;
        }
        resp_queue_.Push(std::move(resp));
      }
    }));
    cpu_set_t cpu_s;
    CPU_ZERO(&cpu_s);
    CPU_SET(i + 1, &cpu_s);
    int rc = pthread_setaffinity_np(workers_[i].native_handle(), sizeof(cpu_s),
                                    &cpu_s);
    assert(rc == 0);
  }
}

XCommitter::~XCommitter() {
  is_stop_ = true;
  for (int i = 0; i < worker_num_; ++i) {
    workers_[i].join();
  }
}

void XCommitter::AddTask(int64_t commit_id,
                         std::unique_ptr<ExecutionContext> context) {
  context_list_[commit_id] = std::move(context);
}

void XCommitter::RemoveTask(int64_t commit_id) {
  context_list_.erase(context_list_.find(commit_id));
}

ExecutionContext* XCommitter::GetTaskContext(int64_t commit_id) {
  return context_list_[commit_id].get();
}

std::vector<std::unique_ptr<ExecuteResp>> XCommitter::ExecContract(
    std::vector<ContractExecuteInfo>& requests) {
  controller_->Clear();
  int process_num = requests.size();
  std::vector<std::unique_ptr<ExecuteResp>> resp_list;

  int id = 1;
  for (auto& request : requests) {
    request.commit_id = id++;
    auto context = std::make_unique<ExecutionContext>(request);
    auto context_ptr = context.get();
    AddTask(request.commit_id, std::move(context));
    request_queue_.Push(std::make_unique<ExecutionContext*>(context_ptr));
  }

  while (process_num) {
    auto resp = resp_queue_.Pop();
    if (resp == nullptr) {
      continue;
    }
    int64_t resp_commit_id = resp->commit_id;
    bool ret = controller_->Commit(resp_commit_id);
    // LOG(ERROR)<<"resp commit:"<<resp_commit_id<<" ret:"<<ret;
    if (ret) {
      // resp->rws = *controller_->GetChangeList(resp_commit_id);
      // LOG(ERROR)<<"get rws:"<<resp_commit_id<<" retry:"<<resp->retry_time;
      resp_list.push_back(std::move(resp));
      RemoveTask(resp_commit_id);
      process_num--;
      continue;
    }
    /*
    else {
        auto context_ptr = GetTaskContext(resp_commit_id);
        context_ptr->SetRedo();
        //LOG(ERROR)<<"redo:"<<next_id;
        request_queue_.Push(std::make_unique<ExecutionContext*>(context_ptr));
    }
    */

    std::vector<int64_t> list = controller_->GetRedo();
    // LOG(ERROR)<<"get list:"<<list.size();
    for (int64_t next_id : list) {
      auto context_ptr = GetTaskContext(next_id);
      context_ptr->SetRedo();
      // LOG(ERROR)<<"redo:"<<next_id;
      request_queue_.Push(std::make_unique<ExecutionContext*>(context_ptr));
    }
  }

  return resp_list;
}

absl::StatusOr<std::string> XCommitter::ExecContract(
    const Address& caller_address, const Address& contract_address,
    const std::string& func_addr, const Params& func_param, EVMState* state) {
  return executor_->ExecContract(caller_address, contract_address, func_addr,
                                 func_param, state);
}

}  // namespace contract
}  // namespace resdb

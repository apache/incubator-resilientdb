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

#include "service/contract/executor/manager/sequential_concurrency_committer.h"

#include <queue>

#include "service/contract/executor/manager/local_state.h"
#include "common/utils/utils.h"

#include "glog/logging.h"
#include "eEVM/processor.h"

namespace resdb {
namespace contract {

/*
ExecutionContext::ExecutionContext(const ContractExecuteInfo & info ) {
  info_ = std::make_unique<ContractExecuteInfo>(info);
}

const ContractExecuteInfo * ExecutionContext::GetContractExecuteInfo() const {
  return info_.get();
}

void ExecutionContext::SetResult(std::unique_ptr<ExecuteResp> result) {
  result_ = std::move(result);
}

bool ExecutionContext::IsRedo() {
  return is_redo_;
}

void ExecutionContext::SetRedo(){
  is_redo_ = true;
}

std::unique_ptr<ExecuteResp> ExecutionContext::FetchResult() {
  return std::move(result_);
}
*/

ExecutionState * SequentialConcurrencyCommitter::GetExecutionState() {
  return &execution_state_;
}

SequentialConcurrencyCommitter:: SequentialConcurrencyCommitter(
    DataStorage * storage,
    GlobalState * global_state, int worker_num):storage_(storage), gs_(global_state),worker_num_(worker_num) {

  controller_ = std::make_unique<SequentialCCController>(storage);
  executor_ = std::make_unique<ContractExecutor>();
  is_stop_ = false;

  SequentialCCController::CallBack callback;
  callback.redo_callback = std::bind(&SequentialConcurrencyCommitter::RedoCallBack, this, std::placeholders::_1, std::placeholders::_2);
  callback.committed_callback = std::bind(&SequentialConcurrencyCommitter::CommitCallBack, this,std::placeholders::_1);

  controller_->SetCallback(callback);

  for (int i = 0; i < worker_num_; ++i) {
    workers_.push_back(std::thread([&]() {
      while (!is_stop_) {
        auto request_ptr = request_queue_.Pop();
        if (request_ptr == nullptr) {
          continue;
        }
        ExecutionContext * request = *request_ptr;
      
        LocalState local_state(controller_.get());
        local_state.Set(gs_->GetAccount(
            request->GetContractExecuteInfo()->contract_address), 
           request->GetContractExecuteInfo()->commit_id);

        std::unique_ptr<ExecuteResp> resp = std::make_unique<ExecuteResp>();
        auto ret = ExecContract(request->GetContractExecuteInfo()->caller_address, 
            request->GetContractExecuteInfo()->contract_address,
            request->GetContractExecuteInfo()->func_addr,
            request->GetContractExecuteInfo()->func_params, &local_state);
        resp->state = ret.status();
        resp->contract_address = request->GetContractExecuteInfo()->contract_address;
        resp->commit_id = request->GetContractExecuteInfo()->commit_id;
        resp->user_id = request->GetContractExecuteInfo()->user_id;
        if(ret.ok()){
          resp->ret = 0;
          resp->result = *ret;
          if(request->IsRedo()){
            resp->retry_time=1;
        //    request->SetResult(std::move(resp));
          }
          local_state.Flesh(request->GetContractExecuteInfo()->contract_address, 
                  request->GetContractExecuteInfo()->commit_id);
        }
        else {
          resp->ret = -1;
        }
        //if(!request->IsRedo()){
          resp_queue_.Push(std::move(resp));
        //}
      }
    }));
  }
}

SequentialConcurrencyCommitter::~SequentialConcurrencyCommitter(){
  is_stop_ = true;
  for (int i = 0; i < worker_num_; ++i) {
    workers_[i].join();
  }
}

void SequentialConcurrencyCommitter::AddTask(int64_t commit_id, std::unique_ptr<ExecutionContext> context){
    context_list_[commit_id] = std::move(context);
}

void SequentialConcurrencyCommitter::RemoveTask(int64_t commit_id){
    context_list_.erase(context_list_.find(commit_id));
}

ExecutionContext* SequentialConcurrencyCommitter::GetTaskContext(int64_t commit_id){
  return context_list_[commit_id].get();
}

void SequentialConcurrencyCommitter::CommitCallBack(int64_t commit_id){
}

void SequentialConcurrencyCommitter::RedoCallBack(int64_t commit_id, int flag){
}

std::vector<std::unique_ptr<ExecuteResp>> SequentialConcurrencyCommitter::ExecContract(
    std::vector<ContractExecuteInfo>& requests) {

  execution_state_.commit_time = 0;
  execution_state_.redo_time = 0;

  int process_num = requests.size();
  std::vector<std::unique_ptr<ExecuteResp>> resp_list;

  std::set<int64_t> commits;
  int id = 1;
  resp_list.resize(process_num);
  for(auto& request: requests) {
    resp_list[id-1] = nullptr;
    request.commit_id = id++;
    auto context = std::make_unique<ExecutionContext>(request);
    AddTask(request.commit_id, std::move(context));
    commits.insert(request.commit_id);
  }

  controller_->Clear();
  for(auto& request: requests) {
    auto context_ptr = GetTaskContext(request.commit_id);
    request_queue_.Push(std::make_unique<ExecutionContext*>(context_ptr));
  }

  auto cur_it = commits.begin();
  std::queue<int64_t> redo_list;
  while(process_num){
    auto resp = resp_queue_.Pop();
    if(resp == nullptr){
      continue;
    }
    int64_t resp_commit_id = resp->commit_id;
    //LOG(ERROR)<<"recv :"<<resp_commit_id<<" process num:"<<process_num;
    resp_list[resp->commit_id-1]=std::move(resp);
    if(cur_it == commits.end() || resp_commit_id < *cur_it){
      bool ret = controller_->Commit(resp_commit_id);
      std::vector<int64_t> list = controller_->GetRedo();
      for(int64_t next_id : list){
        bool ret = controller_->Commit(next_id);
        if(!ret){
          //redo_list.push(next_id);
          auto context_ptr = GetTaskContext(next_id);
          context_ptr->SetRedo();
          request_queue_.Push(std::make_unique<ExecutionContext*>(context_ptr));
        }
      }
      assert(ret);
      process_num--;
    }
    while(cur_it != commits.end() && resp_list[*cur_it-1] !=nullptr){
      int64_t commit_id = *cur_it;
      bool ret = controller_->Commit(commit_id);
      cur_it++;
      if(!ret){
        resp_list[commit_id-1]=nullptr;
        if(controller_->GetRedo().size()){
          //redo_list.push(commit_id);

          auto context_ptr = GetTaskContext(commit_id);
          context_ptr->SetRedo();
          request_queue_.Push(std::make_unique<ExecutionContext*>(context_ptr));
        }
      }
      else {
        process_num--;
        std::vector<int64_t> list = controller_->GetRedo();
        for(int64_t next_id : list){
            resp_list[next_id-1]=nullptr;
            //redo_list.push(next_id);
            auto context_ptr = GetTaskContext(next_id);
            context_ptr->SetRedo();
            request_queue_.Push(std::make_unique<ExecutionContext*>(context_ptr));
        }
      }
    }
  }

  for(auto& request: requests) {
    RemoveTask(request.commit_id);
  }

  storage_->Flush();

  return resp_list;
}

absl::StatusOr<std::string> SequentialConcurrencyCommitter::ExecContract(
    const Address& caller_address, const Address& contract_address,
    const std::string& func_addr,
    const Params& func_param, EVMState * state) {
  return executor_->ExecContract(caller_address, contract_address, func_addr, func_param, state);  
}

}  // namespace contract
}  // namespace resdb

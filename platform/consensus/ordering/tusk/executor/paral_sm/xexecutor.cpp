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

#include "service/contract/executor/paral_sm/xexecutor.h"

#include <future>
#include <queue>

#include "service/contract/executor/paral_sm/local_state.h"
#include "common/utils/utils.h"

#include "glog/logging.h"
#include "eEVM/processor.h"


namespace resdb {
namespace contract {
namespace paral_sm {

XExecutor:: XExecutor(
    DataStorage * storage,
    GlobalState * global_state, 
    int window_size,
    std::function<void (std::unique_ptr<ExecuteResp>)> call_back,
    int worker_num):storage_(storage), gs_(global_state),
    worker_num_(worker_num),
    window_size_(window_size),
    call_back_(call_back) {

  //LOG(ERROR)<<"init window:"<<window_size<<" worker:"<<worker_num_;
  controller_ = std::make_unique<XController>(storage, window_size*2);
  executor_ = std::make_unique<ContractExecutor>();

  resp_list_.resize(window_size_);
  is_done_.resize(window_size_);
  for(int i = 0; i < window_size_;++i){
    is_done_[i] =false;
    resp_list_[i] = nullptr;
  }

  first_id_ = 0;
  last_id_ = 1;
  is_stop_ = false;
  id_ = 1;


  for (int i = 0; i < worker_num_; ++i) {
    workers_.push_back(std::thread([&]() {
      while (!is_stop_) {
        auto request= request_queue_.Pop();
        if (request== nullptr) {
          continue;
        }

        //std::unique_ptr<ExecutionContext> request = std::move(*request_ptr);
      
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
        //LOG(ERROR)<<"=========   get resp commit id:"<<request->GetContractExecuteInfo()->commit_id<<" param:"<<
         // request->GetContractExecuteInfo()->func_params.DebugString();
        if(ret.ok()){
          resp->ret = 0;
          resp->result = *ret;
          if(request->IsRedo()){
            resp->retry_time=request->RedoTime();
          }
          local_state.Flesh(request->GetContractExecuteInfo()->contract_address, 
                  request->GetContractExecuteInfo()->commit_id);
        }
        else {
          LOG(ERROR)<<"commit :"<<resp->commit_id<<" fail";
          resp->ret = -1;
          assert(resp->ret>=0);
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

void XExecutor::SetExecuteCallBack(std::function<void (std::unique_ptr<ExecuteResp>)> func) {
  call_back_ = std::move(func);
}

XExecutor::~XExecutor(){
  //LOG(ERROR)<<"desp";
  is_stop_ = true;
  for (int i = 0; i < worker_num_; ++i) {
    workers_[i].join();
  }
  if(response_.joinable()){
    response_.join();
  }
}

/*
void XExecutor::CallBack(uint64_t commit_id){
  //LOG(ERROR)<<"call back:"<<commit_id;
    int idx = commit_id%window_size_;
    if(call_back_){
      call_back_(std::move(resp_list_[idx]));
    }
    else {
      resp_list_[idx] = nullptr;
    }
    is_done_[idx] = true;

    //LOG(ERROR)<<"current commit done call back commit id:"<<commit_id<<" idx:"<<idx<<" first id:"<<first_id_;
    bool need_notify = false;
    while(first_id_ < last_id_ && is_done_[(first_id_+1)%window_size_]) {
      is_done_[(first_id_+1)%window_size_] = false;
      first_id_++;
      //LOG(ERROR)<<"commit:"<<first_id_;
      need_notify = true;
    }

    if(need_notify){
      std::lock_guard<std::mutex> lk(mutex_);
      cv_.notify_all();
    }
    //LOG(ERROR)<<"call back done:"<<first_id_<<" last id:"<<last_id_;
}

bool XExecutor::WaitNext(){
  while(!is_stop_) {
    int timeout_ms = 10000;
    std::unique_lock<std::mutex> lk(mutex_);
    cv_.wait_for(lk, std::chrono::microseconds(timeout_ms), [&] {
        return id_ - first_id_<window_size_;
    });
    if(id_ - first_id_<window_size_){
      return true;
    }
  }
  return false;
}

bool XExecutor::WaitAll(){
  while(!is_stop_) {
    int timeout_ms = 10000;
    std::unique_lock<std::mutex> lk(mutex_);
    cv_.wait_for(lk, std::chrono::microseconds(timeout_ms), [&] {
        return first_id_>0&&first_id_%500==0;
        //return id_ - first_id_<window_size_;
    });
    if(id_ - first_id_<window_size_){
      return true;
    }
  }
  return false;
}
*/

void XExecutor::ResponseProcess() {
  auto resp = resp_queue_.Pop();
  if(resp == nullptr){
    return; 
  }
  int64_t resp_commit_id = resp->commit_id;
  int idx = resp->commit_id % window_size_;
  //LOG(ERROR)<<"recv :"<<resp_commit_id<<" idx:"<<idx;
  resp_list_[idx]=std::move(resp);

    std::queue<int64_t> q;
    q.push(resp_commit_id);
    while(!q.empty()){
      int64_t next_id = q.front();
      q.pop();

      bool ret = controller_->Commit(next_id);
      std::vector<int64_t> next_commit = controller_->GetRedo();
      //LOG(ERROR)<<"redo size:"<<next_commit.size();
      for(int64_t new_next : next_commit) {
        if(next_id == new_next){
          auto context_ptr = GetTaskContext(new_next);
          context_ptr->SetRedo();
          //LOG(ERROR)<<"redo :"<<new_next;
          request_queue_.Push(std::make_unique<ExecutionContext*>(context_ptr));
        }
        else {
          q.push(new_next);
        }
      }

      std::vector<int64_t> done_list = controller_->GetDone();
      for(int64_t done_id : done_list) {
        //LOG(ERROR)<<"get doen id:"<<done_id;
        CallBack(done_id);
      }
    }
  }
  //
  //LOG(ERROR)<<"last id:"<<last_id_;
  while(resp_list_[last_id_%window_size_] !=nullptr){
    int idx = last_id_%window_size_;
    int64_t current_commit_id = resp_list_[idx]->commit_id;
    //LOG(ERROR)<<" !!!!! commit new resp:"<<current_commit_id;
    bool ret = controller_->Commit(current_commit_id);
    if(!ret){
      //LOG(ERROR)<<"redo size:"<<controller_->GetRedo().size();
      if(controller_->GetRedo().size()){
        assert(controller_->GetRedo()[0] == current_commit_id);
        //redo_list.push(commit_id);
        //LOG(ERROR)<<"commit redo:"<<current_commit_id;
        auto context_ptr = GetTaskContext(current_commit_id);
        context_ptr->SetRedo();
        //LOG(ERROR)<<"redo :"<<current_commit_id;
        request_queue_.Push(std::make_unique<ExecutionContext*>(context_ptr));
      }
    }
    else {
      std::vector<int64_t> list = controller_->GetRedo();
      assert(list.empty());
    }

    std::vector<int64_t> done_list = controller_->GetDone();
    for(int64_t done_id : done_list) {
      //LOG(ERROR)<<"get doen id:"<<done_id;
      CallBack(done_id);
    }

    last_id_++;
  }
//  LOG(ERROR)<<"last id:"<<last_id_;
}

*/
void XExecutor::AsyncExecContract(std::vector<ContractExecuteInfo>& requests) {
  for(auto& request: requests) {
    if(!WaitNext()){
      return;
    }
    request_queue_.Push(std::make_unique<ExecutionContext>(request));
  }

  return ;
}

absl::StatusOr<std::string> XExecutor::ExecContract(
    const Address& caller_address, const Address& contract_address,
    const std::string& func_addr,
    const Params& func_param, EVMState * state) {
  return executor_->ExecContract(caller_address, contract_address, func_addr, func_param, state);  
}

}
}  // namespace contract
}  // namespace resdb

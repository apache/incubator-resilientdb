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

#include "service/contract/executor/x_manager/streaming_e_committer.h"
#include "service/contract/executor/x_manager/executor_state.h"

#include <future>
#include <queue>

#include "common/utils/utils.h"

#include "glog/logging.h"
#include "eEVM/processor.h"

//#define DEBUG

namespace resdb {
namespace contract {
namespace x_manager {

namespace {
class TimeTrack {
public:
  TimeTrack(std::string name = ""){
    name_ = name;
    start_time_ = GetCurrentTime();
  }

  ~TimeTrack(){
    uint64_t end_time = GetCurrentTime();
    //LOG(ERROR) << name_ <<" run:" << (end_time - start_time_)<<"ms"; 
  }
  
  double GetRunTime(){
    uint64_t end_time = GetCurrentTime();
    return (end_time - start_time_) / 1000000.0;
  }
private:
  std::string name_;
  uint64_t start_time_;
};
}

StreamingECommitter:: StreamingECommitter(
    DataStorage * storage,
    GlobalState * global_state, 
    int window_size,
    std::function<void (std::unique_ptr<ExecuteResp>)> call_back,
    int worker_num):storage_(storage), gs_(global_state),
    worker_num_(worker_num),
    window_size_(window_size),
    call_back_(call_back) {

  //LOG(ERROR)<<"init window:"<<window_size<<" worker:"<<worker_num_;
  controller_ = std::make_unique<StreamingEController>(storage, window_size*2);
  executor_ = std::make_unique<ContractExecutor>();

//  LOG(ERROR)<<"init";
  resp_list_.resize(window_size_);
  is_done_.resize(window_size_);
  for(int i = 0; i < window_size_;++i){
    is_done_[i] =false;
    resp_list_[i] = nullptr;
  }

  num_ = 0;
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
        ExecutionContext * request = *request_ptr;
      
        TimeTrack track;
        ExecutorState executor_state(controller_.get(), request->GetContractExecuteInfo()->commit_id);
        executor_state.Set(gs_->GetAccount(
            request->GetContractExecuteInfo()->contract_address), 
           request->GetContractExecuteInfo()->commit_id,
           request->RedoTime());

        std::unique_ptr<ExecuteResp> resp = std::make_unique<ExecuteResp>();
        auto ret = ExecContract(request->GetContractExecuteInfo()->caller_address, 
            request->GetContractExecuteInfo()->contract_address,
            request->GetContractExecuteInfo()->func_addr,
            request->GetContractExecuteInfo()->func_params, &executor_state);
        resp->state = ret.status();
        resp->contract_address = request->GetContractExecuteInfo()->contract_address;
        resp->commit_id = request->GetContractExecuteInfo()->commit_id;
        resp->user_id = request->GetContractExecuteInfo()->user_id;
        //LOG(ERROR)<<"=========   get resp commit id:"<<request->GetContractExecuteInfo()->commit_id<<" param:"<< request->GetContractExecuteInfo()->func_params.DebugString();
        //LOG(ERROR)<<"=========   get resp commit id:"<<request->GetContractExecuteInfo()->commit_id;
        if(ret.ok()){
          resp->ret = 0;
          resp->result = *ret;
          if(request->IsRedo()){
            resp->retry_time=request->RedoTime();
          }
          //assert(resp->retry_time<=5);
          resp->runtime = track.GetRunTime()*1000;
          //LOG(ERROR)<<"run:"<<resp->runtime;
          //local_state.Flesh(request->GetContractExecuteInfo()->contract_address, 
           //       request->GetContractExecuteInfo()->commit_id);
        }
        else {
          //LOG(ERROR)<<"commit :"<<resp->commit_id<<" fail";
          resp->ret = -1;
          //assert(resp->ret>=0);
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

void StreamingECommitter::SetExecuteCallBack(std::function<void (std::unique_ptr<ExecuteResp>)> func) {
  call_back_ = std::move(func);
}

void StreamingECommitter::Clear(){
  for(int i = 0; i < window_size_;++i){
    is_done_[i] =false;
    resp_list_[i] = nullptr;
  }

  num_ = 0;
  first_id_ = 0;
  last_id_ = 1;
  id_ = 1;
}

StreamingECommitter::~StreamingECommitter(){
  is_stop_ = true;
  for (int i = 0; i < worker_num_; ++i) {
    workers_[i].join();
  }
  if(response_.joinable()){
    response_.join();
  }
}

void StreamingECommitter::SetController(std::unique_ptr<StreamingEController> controller) {
  controller_ = std::move(controller);
}
  
void StreamingECommitter::AddTask(int64_t commit_id, std::unique_ptr<ExecutionContext> context){
    context_list_[commit_id%window_size_] = std::move(context);
}

void StreamingECommitter::RemoveTask(int64_t commit_id){
    context_list_.erase(context_list_.find(commit_id));
}

ExecutionContext* StreamingECommitter::GetTaskContext(int64_t commit_id){
  return context_list_[commit_id%window_size_].get();
}

void StreamingECommitter::CallBack(uint64_t commit_id){
    int idx = commit_id%window_size_;
  //LOG(ERROR)<<"call back:"<<commit_id<<" idx:"<<idx;
    if(call_back_){
      call_back_(std::move(resp_list_[idx]));
    }
    else {
      resp_list_[idx] = nullptr;
    }
    is_done_[idx] = true;

    //LOG(ERROR)<<"call back done:"<<commit_id<<" idx:"<<idx<<" resp:"<<(resp_list_[idx] == nullptr);
    //LOG(ERROR)<<"current commit done call back commit id:"<<commit_id<<" idx:"<<idx<<" first id:"<<first_id_;
    bool need_notify = false;
    /*
    while(first_id_ < last_id_ && is_done_[(first_id_+1)%window_size_]) {
      is_done_[(first_id_+1)%window_size_] = false;
      first_id_++;
      //LOG(ERROR)<<"commit:"<<first_id_;
      need_notify = true;
    }
    */
    need_notify = true;

    if(need_notify){
      std::lock_guard<std::mutex> lk(mutex_);
      cv_.notify_all();
      num_--;
    }
}

bool StreamingECommitter::WaitNext(){
  int c = 64;
  while(!is_stop_) {
    int timeout_ms = 10000;
    std::unique_lock<std::mutex> lk(mutex_);
    cv_.wait_for(lk, std::chrono::microseconds(timeout_ms), [&] {
        return num_<c;
        //return id_ - first_id_<window_size_;
    });
    if(num_<c){
    //if(id_ - first_id_<window_size_){
      return true;
    }
  }
  return false;
}

bool StreamingECommitter::WaitAll(){
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

void StreamingECommitter::ResponseProcess() {
  auto resp = resp_queue_.Pop();
  if(resp == nullptr){
    return; 
  }
  int64_t resp_commit_id = resp->commit_id;
  int idx = resp->commit_id % window_size_;

#ifdef DEBUG
  LOG(ERROR)<<"recv :"<<resp_commit_id<<" idx:"<<idx<<" last id:"<<last_id_<<" ret:"<<resp->ret<<" last id:"<<last_id_;
#endif
  resp_list_[idx]=std::move(resp);

  controller_->Commit(resp_commit_id);
  std::vector<int64_t> next_commit = controller_->GetRedo();
  for(int64_t new_next : next_commit) {
    auto context_ptr = GetTaskContext(new_next);
    context_ptr->SetRedo();
#ifdef DEBUG
    LOG(ERROR)<<"redo :"<<new_next<<" redo time:"<<context_ptr->RedoTime();
#endif
    controller_->Clear(new_next);
    request_queue_.Push(std::make_unique<ExecutionContext*>(context_ptr));
  }

  std::vector<int64_t> done_list = controller_->GetDone();
  for(int64_t done_id : done_list) {
    //LOG(ERROR)<<"get doen id:"<<done_id;
    CallBack(done_id);
  }
}

void StreamingECommitter::AsyncExecContract(std::vector<ContractExecuteInfo>& requests) {
 Clear();
 controller_->Clear();

  for(auto& request: requests) {
    if(!WaitNext()){
      return;
    }
    num_++;
    int cur_idx = id_%window_size_;
    assert(id_>=last_id_);

    request.commit_id = id_++;
    auto context = std::make_unique<ExecutionContext>(request);
    auto context_ptr = context.get();

    //LOG(ERROR)<<"execute:"<<request.commit_id<<" id:"<<id_<<" first id:"<<first_id_<<" last id:"<<last_id_<<" window:"<<window_size_<<" cur idx:"<<cur_idx;
   assert(cur_idx<window_size_);
    assert(resp_list_[cur_idx] == nullptr);

    AddTask(cur_idx, std::move(context));
    controller_->Clear(request.commit_id);
    request_queue_.Push(std::make_unique<ExecutionContext*>(context_ptr));
  }

  return ;
}

absl::StatusOr<std::string> StreamingECommitter::ExecContract(
    const Address& caller_address, const Address& contract_address,
    const std::string& func_addr,
    const Params& func_param, EVMState * state) {
    //LOG(ERROR)<<"start:"<<caller_address;
  return executor_->ExecContract(caller_address, contract_address, func_addr, func_param, state);  
}

}
}  // namespace contract
}  // namespace resdb

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

#include "platform/consensus/execution/transaction_executor.h"

#include <glog/logging.h>
#include "common/utils/utils.h"


namespace resdb {

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
      stop_(false),
      duplicate_manager_(nullptr) {

  memset(blucket_, 0, sizeof(blucket_));
  global_stats_ = Stats::GetGlobalStats();
  ordering_thread_ = std::thread(&TransactionExecutor::OrderMessage, this);
  for (int i = 0; i < execute_thread_num_; ++i) {
    execute_thread_.push_back(
        std::thread(&TransactionExecutor::ExecuteMessage, this));
  }

  for (int i = 0; i < 1; ++i) {
    prepare_thread_.push_back(
        std::thread(&TransactionExecutor::PrepareMessage, this));
  }

  if (transaction_manager_ && transaction_manager_->IsOutOfOrder()) {
    execute_OOO_thread_ =
        std::thread(&TransactionExecutor::ExecuteMessageOutOfOrder, this);
    LOG(ERROR) << " is out of order:" << transaction_manager_->IsOutOfOrder();
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
  if (execute_thread_num_ == 1) return;
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
        // LOG(INFO) << "request seq:" << seq << " has been executed"
        // << " next seq:" << next_execute_seq_;
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
    message->set_commit_time(GetCurrentTime());
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
  std::unique_ptr<BatchUserResponse> response;
   global_stats_->GetTransactionDetails(batch_request);
  if (transaction_manager_) {
    response = transaction_manager_->ExecuteBatch(batch_request);
  }
  if (response != nullptr){
    std::cout<<3<<"testing"<<request->seq()<<std::endl;
    set_OnExecuteSuccess(request->seq());
  }
  // global_stats_->IncTotalRequest(batch_request.user_requests_size());
  // global_stats_->IncExecuteDone();
}

void TransactionExecutor::Execute(std::unique_ptr<Request> request,
                                  bool need_execute) {
  std::cout<<0<<"testing"<<request->seq()<<std::endl;
  RegisterExecute(request->seq());
  std::unique_ptr<BatchUserRequest> batch_request = nullptr;
  std::unique_ptr<std::vector<std::unique_ptr<google::protobuf::Message>>> data;
  std::vector<std::unique_ptr<google::protobuf::Message>> * data_p = nullptr;
  BatchUserRequest* batch_request_p = nullptr;

  // Execute the request, then send the response back to the user.
  if (batch_request_p == nullptr) {
    batch_request = std::make_unique<BatchUserRequest>();
    if (!batch_request->ParseFromString(request->data())) {
      LOG(ERROR) << "parse data fail";
    }
    batch_request->set_hash(request->hash());
    if (request->has_committed_certs()) {
      *batch_request->mutable_committed_certs() = request->committed_certs();
    }
    batch_request->set_seq(request->seq());
    batch_request->set_proxy_id(request->proxy_id());
    batch_request_p = batch_request.get();
    // LOG(ERROR)<<"get data from req:";
  } else {
  assert(batch_request_p);
    batch_request_p->set_seq(request->seq());
    batch_request_p->set_proxy_id(request->proxy_id());
    // LOG(ERROR)<<" get from cache:"<<uid;
  }
  assert(batch_request_p);

  // LOG(INFO) << " get request batch size:"
  // << batch_request.user_requests_size()<<" proxy id:"
  //  <<request->proxy_id()<<" need execute:"<<need_execute;

  std::unique_ptr<BatchUserResponse> response;
  
  global_stats_->GetTransactionDetails(*batch_request_p);
  if (transaction_manager_ && need_execute) {
    if (execute_thread_num_ == 1) {
      response = transaction_manager_->ExecuteBatch(*batch_request_p);
      if (response != nullptr){
        std::cout<<1<<"testing"<<request->seq()<<std::endl;
        set_OnExecuteSuccess(request->seq());
      }
    } else {
      std::vector<std::unique_ptr<std::string>> response_v;

      if(data_p == nullptr) {
        int64_t start_time = GetCurrentTime();
        data = std::move(transaction_manager_->Prepare(*batch_request_p));
        int64_t end_time = GetCurrentTime();
        if (end_time - start_time > 10) {
          // LOG(ERROR)<<"exec data done:"<<uid<<" wait
          // time:"<<(end_time-start_time);
        }
        data_p = data.get();
      }

      WaitForExecute(request->seq());
	    if(data_p->empty() || (*data_p)[0] == nullptr){
		    response = transaction_manager_->ExecuteBatch(*batch_request_p);
	    }
	    else {
		    response_v = transaction_manager_->ExecuteBatchData(*data_p);
	    }
      if (response != nullptr || !response_v.empty()){
        std::cout<<2<<"testing"<<request->seq()<<std::endl;
        set_OnExecuteSuccess(request->seq());
      }
      FinishExecute(request->seq());

      if(response == nullptr){
	      response = std::make_unique<BatchUserResponse>();
	      for (auto& s : response_v) {
		      response->add_response()->swap(*s);
	      }
      }
    }
  }
  // LOG(ERROR)<<" CF = :"<<(cf==1)<<" uid:"<<uid;


  
  if (duplicate_manager_ && batch_request_p) {	      
    duplicate_manager_->AddExecuted(batch_request_p->hash(), batch_request_p->seq());		    
  }

  if (response == nullptr) {
    response = std::make_unique<BatchUserResponse>();
  }
  global_stats_->IncTotalRequest(batch_request_p->user_requests_size());
  response->set_proxy_id(batch_request_p->proxy_id());
  response->set_createtime(batch_request_p->createtime() + request->queuing_time());
  response->set_local_id(batch_request_p->local_id());

  response->set_seq(request->seq());

  if (post_exec_func_) {
    post_exec_func_(std::move(request), std::move(response));
  }

  global_stats_->IncExecuteDone();
}

void TransactionExecutor::set_OnExecuteSuccess(uint64_t seq) {
  // Monotonic update: only move forward.
  uint64_t cur = latest_executed_seq_.load(std::memory_order_relaxed);
  while (seq > cur && !latest_executed_seq_.compare_exchange_weak(
            cur, seq, std::memory_order_release, std::memory_order_relaxed)) {
    /* retry with updated `cur` */
  }
  // latest_executed_seq_ = seq;
}

uint64_t TransactionExecutor::get_latest_executed_seq() const {
  return latest_executed_seq_/*.load(std::memory_order_acquire)*/;
}

void TransactionExecutor::SetDuplicateManager(DuplicateManager* manager) {
  duplicate_manager_ = manager;
}


bool TransactionExecutor::SetFlag(uint64_t uid, int f) {
  std::unique_lock<std::mutex> lk(f_mutex_[uid % mod]);
  auto it = flag_[uid % mod].find(uid);
  if (it == flag_[uid % mod].end()) {
    flag_[uid % mod][uid] |= f;
    // LOG(ERROR)<<"NO FUTURE uid:"<<uid;
    return true;
  }
  assert(it != flag_[uid % mod].end());
  if (f == Start_Prepare) {
    if (flag_[uid % mod][uid] & Start_Execute) {
      return false;
    }
  } else if(f == Start_Execute){
    if (flag_[uid % mod][uid] & End_Prepare) {
    //if (flag_[uid % mod][uid] & Start_Prepare) {
      return false;
    }
  }
  flag_[uid % mod][uid] |= f;
  return true;
}

void TransactionExecutor::ClearPromise(uint64_t uid) {
  std::unique_lock<std::mutex> lk(f_mutex_[uid % mod]);
  auto it = pre_[uid % mod].find(uid);
  if (it == pre_[uid % mod].end()) {
    return;
  }
  // LOG(ERROR)<<"CLEAR UID:"<<uid;
  assert(it != pre_[uid % mod].end());
  assert(flag_[uid % mod].find(uid) != flag_[uid % mod].end());
  //assert(data_[uid%mod].find(uid) != data_[uid%mod].end());
  //assert(req_[uid%mod].find(uid) != req_[uid%mod].end());
  //data_[uid%mod].erase(data_[uid%mod].find(uid));
  //req_[uid%mod].erase(req_[uid%mod].find(uid));
  pre_[uid % mod].erase(it);
  flag_[uid % mod].erase(flag_[uid % mod].find(uid));
}

std::promise<int>* TransactionExecutor::GetPromise(uint64_t uid) {
  std::unique_lock<std::mutex> lk(f_mutex_[uid % mod]);
  auto it = pre_[uid % mod].find(uid);
  if (it == pre_[uid % mod].end()) {
    return nullptr;
  }
  return it->second.get();
}

std::unique_ptr<std::future<int>> TransactionExecutor::GetFuture(uint64_t uid) {
  std::unique_lock<std::mutex> lk(f_mutex_[uid % mod]);
  auto it = pre_[uid % mod].find(uid);
  if (it == pre_[uid % mod].end()) {
    return nullptr;
  }
  //return std::move(it->second);
  // LOG(ERROR)<<"add future:"<<uid;
  return std::make_unique<std::future<int>>(it->second->get_future());
}

bool TransactionExecutor::AddFuture(uint64_t uid) {
  std::unique_lock<std::mutex> lk(f_mutex_[uid % mod]);
  auto it = pre_[uid % mod].find(uid);
  if (it == pre_[uid % mod].end()) {
    // LOG(ERROR)<<"add future:"<<uid;
    std::unique_ptr<std::promise<int>> p =
        std::make_unique<std::promise<int>>();
    //auto f = std::make_unique<std::future<int>>(p->get_future());
    pre_[uid % mod][uid] = std::move(p);
    //pre_f_[uid % mod][uid] = std::move(f);
    flag_[uid % mod][uid] = 0;
    return true;
  }
  return false;
}

void TransactionExecutor::Prepare(std::unique_ptr<Request> request) {
  if (AddFuture(request->uid())) {
    prepare_queue_.Push(std::move(request));
  }
}

void TransactionExecutor::PrepareMessage() {
  while (!IsStop()) {
    std::unique_ptr<Request> request = prepare_queue_.Pop();
    if (request == nullptr) {
      continue;
    }

    uint64_t uid = request->uid();
    int current_f = SetFlag(uid, Start_Prepare);
    if (current_f == 0) {
      // commit has done
      // LOG(ERROR)<<" want prepare, commit started:"<<uid;
//      ClearPromise(uid);
      continue;
    }

    std::promise<int>* p = GetPromise(uid) ;
    assert(p);
    //LOG(ERROR)<<" prepare started:"<<uid;

    // LOG(ERROR)<<" prepare uid:"<<uid;

    // Execute the request, then send the response back to the user.
    std::unique_ptr<BatchUserRequest> batch_request =
        std::make_unique<BatchUserRequest>();
    if (!batch_request->ParseFromString(request->data())) {
      LOG(ERROR) << "parse data fail";
    }
    // batch_request = std::make_unique<BatchUserRequest>();
    batch_request->set_seq(request->seq());
    batch_request->set_hash(request->hash());
    batch_request->set_proxy_id(request->proxy_id());
    if (request->has_committed_certs()) {
      *batch_request->mutable_committed_certs() = request->committed_certs();
    }

    // LOG(ERROR)<<"prepare seq:"<<batch_request->seq()<<" proxy
    // id:"<<request->proxy_id()<<" local id:"<<batch_request->local_id();

    std::unique_ptr<std::vector<std::unique_ptr<google::protobuf::Message>>>
     request_v = transaction_manager_->Prepare(*batch_request);
    {
      std::unique_lock<std::mutex> lk(fd_mutex_[uid % mod]);
   //   assert(request_v);
      // assert(data_[uid%mod].find(uid) == data_[uid%mod].end());
      data_[uid%mod][uid] = std::move(request_v);
      req_[uid % mod][uid] = std::move(batch_request);
    }
    //LOG(ERROR)<<"set promise:"<<uid;
    p->set_value(1);
    {
      int set_ret = SetFlag(uid, End_Prepare);
      if (set_ret == 0) {
        // LOG(ERROR)<<"commit interrupt:"<<uid;
        //ClearPromise(uid);
      } else {
        //LOG(ERROR)<<"prepare done:"<<uid;
      }
    }
  }
}


}  // namespace resdb

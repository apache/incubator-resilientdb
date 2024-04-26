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

#include "executor/kv/quecc_executor.h"

#include <glog/logging.h>

#include <atomic>
#include <barrier>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>
#include <queue>

#include "chain/storage/storage.h"
#include "executor/common/transaction_manager.h"
#include "platform/config/resdb_config_utils.h"
#include "platform/proto/resdb.pb.h"
#include "proto/kv/kv.pb.h"
using namespace std;
using resdb::KVRequest;
// 4 is hardcoded for thread count for now, eventually will use config value
barrier execute_barrier(5);
mutex results_mutex;

namespace resdb {

QueccExecutor::QueccExecutor(std::unique_ptr<Storage> storage)
    : storage_(std::move(storage)) {
  thread_count_ = 5;
  atomic<int> ready_planner_count_(0);
  std::unique_ptr<BatchUserResponse> batch_output =
      std::make_unique<BatchUserResponse>();
  for (int thread_number = 0; thread_number < thread_count_; thread_number++) {
    batch_array_.push_back(std::vector<KVOperation>());
    sorted_transactions_.push_back(
        std::vector<std::vector<KVOperation>>(thread_count_));
    batch_ready_.push_back(false);
    multi_op_batches_.push_back(std::vector<KVRequest>());
    multi_op_number_batches_.push_back(std::vector<int>());
    wait_point_list_.push_back(std::vector<int>());
    multi_op_ready_.store(false);

    std::thread planner(&QueccExecutor::PlannerThread, this, thread_number);

    // thread pinning
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(thread_number, &cpuset);
    int status = pthread_setaffinity_np(planner.native_handle(),
                                        sizeof(cpu_set_t), &cpuset);
    thread_list_.push_back(move(planner));
  }
}

QueccExecutor::~QueccExecutor() {
  is_stop_ = true;
  cv_.notify_all();
  cv2_.notify_all();
  for (auto& th : thread_list_) {
    if (th.joinable()) {
      th.join();
    }
  }
}
std::unique_ptr<std::string> QueccExecutor::ExecuteData(
    const std::string& request) {
  KVRequest kv_request;
  if (!kv_request.ParseFromString(request)) {
    LOG(ERROR) << "parse data fail";
    return nullptr;
  }
  return ExecuteData(kv_request);
}

struct CompareWeight {
    bool operator()(const std::pair<int, int>& left, const std::pair<int, int>& right) const {
        return left.second > right.second;
    }
};

void QueccExecutor::CreateRanges(){
  priority_queue<pair<int, int>, vector<pair<int, int>>, CompareWeight> range_current_weight;
  for(int i=0; i<thread_count_; i++){
    range_current_weight.push({i, 0});
  }
  int weight_threshold= total_weight_/thread_count_;
  vector<pair<string, int>> sort_list;
  bool assigned=false;
  int pos=0;
  for(const auto& pair: key_weight_){
    sort_list.push_back(pair);
  }
  sort(sort_list.begin(), sort_list.end(), [](const auto& a, const auto& b){
    return a.second>b.second;
  });
  pair<int, int> smallest_range;
  for(const auto& pair: sort_list){
    //Find range with lowest weight
    smallest_range= range_current_weight.top();
    range_current_weight.pop();
    rid_to_range_[pair.first]=smallest_range.first;
    smallest_range.second=smallest_range.second+pair.second;
    range_current_weight.push(smallest_range);
    //Add key to range and weight to range
  }
}

// Using threadnumber, can know which position in vector to pull for planner
// thread and which range to pull for executor thread
void QueccExecutor::PlannerThread(const int thread_number) {
  std::vector<std::unique_ptr<std::string>> response_group;
  while (true) {
    response_group.clear();
    while (!batch_ready_[thread_number]) {
      std::unique_lock<std::mutex> lk(mutex_);
      cv_.wait_for(lk, std::chrono::microseconds(100),
                   [&] { return batch_ready_[thread_number]; });
      if (is_stop_) {
        return;
      }
    }

    if(multi_op_ready_.load()){
      //Process multi_op distributions for wait positions
      vector<KVRequest> multi_op_batch = multi_op_batches_[thread_number];
      vector<int> multi_op_id = multi_op_number_batches_[thread_number];
      std::cout<<"Hello 2.5 "<< multi_op_batch.size() << std::endl;
      //Look through each KV_Request, if a KV_Request is ever across 2 ranges
      //it could cause a cascading abort and is therefore recorded as a wait point
      int txn_id=0;
      set<int> range_used_list = set<int>();
      for(size_t i=0; i<multi_op_batch.size(); i++){
        txn_id=multi_op_id[i];
        range_used_list.clear();
        for(const auto& request_operation: multi_op_batch[i].ops()){
          range_used_list.insert(rid_to_range_.at(request_operation.key()));
          if(range_used_list.size()>1){
            std::cout<<"Hello 5"<<std::endl;
            wait_point_list_[thread_number].push_back(txn_id);
            break;
          }
        }
      }

      ready_planner_count_.fetch_sub(1);
      if (ready_planner_count_.load() == 0) {
        cv2_.notify_all();
      }
      batch_ready_[thread_number] = false;
      continue;
    }

    // Planner
    std::vector<KVOperation> batch =
        std::move(batch_array_[thread_number]);

    for (const auto& oper : batch) {
      this->sorted_transactions_[thread_number][rid_to_range_.at(oper.op.key())]
          .push_back(oper);
    }

    batch_array_[thread_number].clear();
    const int& range_being_executed = thread_number;

    int wait_point_position=0;
    int wait_point_vector=0;
    vector<vector<int>> local_wait_points;
    for(const auto& waitPointVector : wait_point_list_){
      local_wait_points.push_back(waitPointVector);
    }
    std::cout<<"Hello 6"<<std::endl;
    execute_barrier.arrive_and_wait();

    // Executor
    for (int priority = 0; priority < thread_count_; priority++) {
      std::vector<KVOperation>& range_ops =
          this->sorted_transactions_[priority][range_being_executed];
      for (size_t op_count=0; op_count<range_ops.size(); op_count++) {
        KVOperation op = range_ops[op_count];
          //If past wait point, check for next wait point
        while(wait_point_vector<wait_point_list_.size() && op.transaction_number>wait_point_list_[wait_point_vector][wait_point_position]){
          wait_point_position++;
          if(wait_point_position==(int)wait_point_list_[wait_point_vector].size()){
            wait_point_position=0;
            wait_point_vector++;
          }
        }
        //std::cout<<"Hello 7"<<std::endl;
        //If at wait point, first time through, check operations to know if transaction is safe
        //If it succeeds, all subsequent operations in txn can skip this step
        //If it fails, then it goes into if, sses operations_checked is negative, and skips the operation, as txn was aborted
        if(wait_point_vector<wait_point_list_.size() && op.transaction_number==wait_point_list_[wait_point_vector][wait_point_position] && operations_checked_[op.transaction_number].load()!=transaction_tracker_[op.transaction_number]){
          if(operations_checked_.find(op.transaction_number)==operations_checked_.end()){
          }
          if(operations_checked_[op.transaction_number].load()<0){
            continue;
          }
          KVOperation current_op=op;
          int current_txn_id=op.transaction_number;
          int new_op_count=op_count;
          bool same_txn=true;
          //Verify all operations in txn
          while(same_txn){ 
            if(operations_checked_[current_txn_id].load()>=0 && current_op.op.cmd() == Operation::SET){   
              if(!VerifyRequest(current_op.op.key(), current_op.op.value())){
                //If verify fails, set to -2*op_count so it never becomes positive
                operations_checked_[current_txn_id].store(-2*transaction_tracker_[current_txn_id]);
              }
            }
            ++operations_checked_[current_txn_id];
            new_op_count++;
            current_op=range_ops[new_op_count];
            //Continues looping through operations until next operation has a different txn id
            if(current_op.transaction_number!=current_txn_id){
              same_txn=false;
              new_op_count--;
            }
          }
          //Wait for all threads to finish checking ops from this txn
          while(operations_checked_[current_txn_id].load()!=transaction_tracker_[current_txn_id] && operations_checked_[current_txn_id].load()>0){}
          //Check again in case another thread had an abort, if so, skip to first operation of next txn id
          if(operations_checked_[current_txn_id].load()<0){
            op_count=new_op_count;
            continue;
          }
        }
        //After all checks are done, can execute operations
        std::unique_ptr<std::string> response = ExecuteData(op);
        if (response == nullptr) {
          response = std::make_unique<std::string>();
        }
        response_group.push_back(std::move(response));
      }
      this->sorted_transactions_[priority][range_being_executed].clear();
    }

    // Lock used to minimize conflicts of adding to batchresponse
    results_mutex.lock();
    for (const auto& response : response_group) {
      batch_response_->add_response()->swap(*response);
    }
    results_mutex.unlock();
    ready_planner_count_.fetch_sub(1);
    if (ready_planner_count_.load() == 0) {
      cv2_.notify_all();
    }
    batch_ready_[thread_number] = false;
  }
}

std::unique_ptr<BatchUserResponse> QueccExecutor::ExecuteBatch(
    const BatchUserRequest& request) {
  std::cout<<"Hello"<<std::endl;
  this->batch_response_ = std::make_unique<BatchUserResponse>();
  rid_to_range_.clear();
  transaction_tracker_.clear();
  operation_list_.clear();
  key_weight_.clear();
  multi_op_transactions_.clear();
  wait_point_list_.clear();
  operations_checked_.clear();
  multi_op_transactions_numbers_.clear();
  total_weight_=0;

  for (int i = 0; i < (int)batch_array_.size(); i++) {
    batch_array_[i].clear();
  }
  int batch_number = 0;
  int txn_id=0;
  // process through transactions
  for (const auto& sub_request : request.user_requests()) {
    KVRequest kv_request;
    if (!kv_request.ParseFromString(sub_request.request().data())) {
      LOG(ERROR) << "parse data fail";
      return std::move(this->batch_response_);
    }

    // printf("txn id: %lu\n", kv_request.txn_id());
    //printf("kv_request size: %d\n", kv_request.ops_size());    
    if (kv_request.ops_size()) {
      if(kv_request.ops_size()>1){
        multi_op_transactions_.push_back(kv_request);
        multi_op_transactions_numbers_.push_back(txn_id);
      }
      transaction_tracker_[txn_id] = kv_request.ops_size();
      for(const auto& op : kv_request.ops()){
        KVOperation newOp;
        newOp.transaction_number=txn_id;
        newOp.op= op;
        operation_list_.push_back(newOp);
        if(!key_weight_.count(op.key())){
          key_weight_[op.key()]=0;
        }
        //Add weight, tune weights to estimated cost ratio between set and get
        if(op.cmd() == Operation::SET){
          key_weight_[op.key()]=key_weight_[op.key()]+5;
          total_weight_=total_weight_+5;
        }
        else{
          key_weight_[op.key()]=key_weight_[op.key()]+1;
          total_weight_=total_weight_+1;
        }
      }
    } else {
      KVOperation newOp;
      newOp.transaction_number=txn_id;
      newOp.op.set_cmd(kv_request.cmd());
      newOp.op.set_key(kv_request.key());
      newOp.op.set_value(kv_request.value());
      operation_list_.push_back(newOp);

      if(kv_request.cmd() == Operation::SET){
        key_weight_[kv_request.key()]=key_weight_[kv_request.key()]+5;
        total_weight_=total_weight_+5;
      }
      else{
        key_weight_[kv_request.key()]=key_weight_[kv_request.key()]+1;
        total_weight_=total_weight_+1;
      }
      transaction_tracker_[txn_id]=1;
    }
    operations_checked_[txn_id].store(0);
    txn_id++;
  }
  std::cout<<"Hello 2 "<<operation_list_.size() << " "<< multi_op_transactions_.size()<<std::endl;
  int planner_vector_size =
      (operation_list_.size() + thread_count_ - 1) / thread_count_;
  for (const auto& op : operation_list_) {
    //Push txn into correct vector
    batch_array_[batch_number].push_back(std::move(op));
    if ((int)batch_array_[batch_number].size() >= planner_vector_size) {
      batch_number++;
    }
  }
  CreateRanges();

  int multi_op_split_size = multi_op_transactions_numbers_.size()/thread_count_+1;
  int count_per_split=0;
  int op_batch_number=0;
  //Send multi operation transactions to thread to gauge if they require waits
  for(size_t i=0; i<multi_op_transactions_numbers_.size(); i++){
    multi_op_ready_.store(true);
    multi_op_batches_[op_batch_number].push_back(multi_op_transactions_[i]);
    multi_op_number_batches_[op_batch_number].push_back(multi_op_transactions_numbers_[i]);
    count_per_split++;
    if(count_per_split>multi_op_split_size){
      count_per_split=0;
      op_batch_number++;
    }
  }
  std::cout<<"Hello 3 "<< multi_op_batches_[0].size()<<std::endl;
  if(multi_op_ready_.load()){
    ready_planner_count_.fetch_add(thread_count_);
    // Allows planner threads to start consuming
    for (int i = 0; i < thread_count_; i++) {
      batch_ready_[i] = true;
    }
    cv_.notify_all();

    // Wait for threads to finish to get batch response
    while (ready_planner_count_.load() != 0) {
      std::unique_lock<std::mutex> lk2(mutex2_);
      if (cv2_.wait_for(lk2, std::chrono::microseconds(100),
                        [this] { return is_stop_; })) {
        return std::move(this->batch_response_);
      }
    }
    multi_op_ready_.store(false);
  }
  std::cout<<"Hello 4 "<<std::endl;
  ready_planner_count_.fetch_add(thread_count_);
  // Allows planner threads to start consuming
  for (int i = 0; i < thread_count_; i++) {
    batch_ready_[i] = true;
  }
  cv_.notify_all();

  // Wait for threads to finish to get batch response
  while (ready_planner_count_.load() != 0) {
    std::unique_lock<std::mutex> lk2(mutex2_);
    if (cv2_.wait_for(lk2, std::chrono::microseconds(100),
                      [this] { return is_stop_; })) {
      return std::move(this->batch_response_);
    }
  }
  return std::move(this->batch_response_);
}

std::unique_ptr<std::string> QueccExecutor::ExecuteData(const KVOperation& oper) {
  KVResponse kv_response;
  if (oper.op.cmd() == Operation::SET) {
    Set(oper.op.key(), oper.op.value());
  } else if (oper.op.cmd() == Operation::GET) {
    kv_response.set_value(Get(oper.op.key()));
  } else if (oper.op.cmd() == Operation::GETALLVALUES) {
    kv_response.set_value(GetAllValues());
  } else if (oper.op.cmd() == Operation::GETRANGE) {
    kv_response.set_value(GetRange(oper.op.key(), oper.op.value()));
  }

  std::unique_ptr<std::string> resp_str = std::make_unique<std::string>();
  if (!kv_response.SerializeToString(resp_str.get())) {
    return nullptr;
  }
  // Add some way to decrement number of KVOperations for the txn in
  // transaction_tracker here, could have map of int->atomic_int
  // If a txn has 0 KVOperations remaining, remove it
  // Any txns left in map by end had some issue/aborted and we know which
  // txns must be undone
  return resp_str;
}

std::unique_ptr<std::string> QueccExecutor::ExecuteData(
    const KVRequest& kv_request) {
  KVResponse kv_response;

  /*if (kv_request.ops_size()) {
    for (const auto& op : kv_request.ops()) {
      auto resp_info = kv_response.add_resp_info();
      resp_info->set_key(op.key());
      if (op.cmd() == Operation::SET) {
        Set(op.key(), op.value());
      } else if (op.cmd() == Operation::GET) {
        resp_info->set_value(Get(op.key()));
      } else if (op.cmd() == Operation::GETALLVALUES) {
        resp_info->set_value(GetAllValues());
      } else if (op.cmd() == Operation::GETRANGE) {
        resp_info->set_value(GetRange(op.key(), op.value()));
      }
    }
  } else {*/
    if (kv_request.cmd() == Operation::SET) {
      Set(kv_request.key(), kv_request.value());
    } else if (kv_request.cmd() == Operation::GET) {
      kv_response.set_value(Get(kv_request.key()));
    } else if (kv_request.cmd() == Operation::GETALLVALUES) {
      kv_response.set_value(GetAllValues());
    } else if (kv_request.cmd() == Operation::GETRANGE) {
      kv_response.set_value(GetRange(kv_request.key(), kv_request.value()));
    }
  //}

  std::unique_ptr<std::string> resp_str = std::make_unique<std::string>();
  if (!kv_response.SerializeToString(resp_str.get())) {
    return nullptr;
  }

  return resp_str;
}

void QueccExecutor::Set(const std::string& key, const std::string& value) {
   storage_->SetValue(key, value);
}

std::string QueccExecutor::Get(const std::string& key) {
  return storage_->GetValue(key);
}

std::string QueccExecutor::GetAllValues() { return storage_->GetAllValues(); }

// Get values on a range of keys
std::string QueccExecutor::GetRange(const std::string& min_key,
                                    const std::string& max_key) {
  return storage_->GetRange(min_key, max_key);
}

}  // namespace resdb

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

#include "executor/kv/kv_executor.h"

#include <glog/logging.h>

namespace resdb {

StrictExecutor::StrictExecutor(std::unique_ptr<Storage> storage)
    : storage_(std::move(storage)) {
      int thread_count_=4;
      for(int i=0; i<thread_count_;i++){
        std::thread planner(&StrictExecutor::WorkerThread, this, NULL);

        // thread pinning
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(thread_number, &cpuset);
        int status = pthread_setaffinity_np(planner.native_handle(),
                                            sizeof(cpu_set_t), &cpuset);
        thread_list_.push_back(move(planner));
      }

    }

StrictExecutor::~StrictExecutor() {
  is_stop_ = true;
  empty_queue_.notify_all();
  not_empty_queue_.notify_all();
  lock_taken_.notify_all();
  for (auto& th : thread_list_) {
    if (th.joinable()) {
      th.join();
    }
  }
}
void StrictExecutor::WorkerThread(){
  KVRequest kv_request;
  std::set<std::string> held_locks;
  bool lock_conflict;
  std::string key;
  while(true){
    //Wait until request for thread to process is added and grab it
    queue_lock_.lock();
    while(request_queue.size()==0){
      if (not_empty_queue_.wait_for(queue_lock_, std::chrono::microseconds(100),
        [this] { return is_stop_; })) {
        queue_lock_.unlock();
        return;
      }
    }
    kv_request=request_queue_.front();
    request_queue.pop();
    queue_lock_.unlock();

    //Grab locks needed for request
    map_lock_.lock();
    //If multiple operations in kv_request
    if(kv_request.ops_size()){
      lock_conflict=true;
      while(lock_conflict){
        lock_conflict=false;
        for(const auto& op : kv_request.ops()){
          if(held_locks.contains(op.key())){
            continue;
          }
          if(lock_map_.find(op.key()) == lock_map_.end() || lock_map_[op.key()]==0){
            lock_map_[op.key()]=1;
            held_locks.insert(op.key());
          }
          else{
            lock_conflict=true;
            break;
          }
        }
        if(lock_conflict){
          for(auto& key : held_locks_){
            lock_map_[key]=0;
          }
          lock_taken_.notify_all();
          //Put thread to sleep until another thread releases locks or time expires
          if (lock_taken_.wait_for(map_lock_, std::chrono::microseconds(100),[this] { return is_stop_; })) {
            map_lock_.unlock();
            return;
          }
        }
      }
    }
    //If only one operation in kv_request
    else{
      if(lock_map_.find(kv_request.key()) == lock_map_.end()){
        lock_map_[kv_request.key()]=1;
        held_locks.insert(kv_request.key());
      }
      else{
        while(lock_map[kv_request.key()]==1){
          if (lock_taken_.wait_for(map_lock_, std::chrono::microseconds(100),[this] { return is_stop_; })) {
            map_lock_.unlock();
            return;
          }
        }
      }
    }
    map_lock_.unlock();

    //Once all locks are grabbed, execute transaction
    std::unique_ptr<std::string> response =
        ExecuteData(kv_request);
    if (response == nullptr) {
      response = std::make_unique<std::string>();
    }

    //Release locks that were grabbed
    map_lock_.lock();
    for(auto& key : held_locks_){
      lock_map_[key]=0;
    }
    map_lock_.unlock();
    lock_taken_.notify_all();

    //Add response
    response_lock_.lock();
    this->batch_response->add_response()->swap(*response);
    //Need to check all threads are done processing, and if all threads are done + queue is empty, wake main thread
    //Maybe check number of responses in batch response, if equals number of requests, all requests are processed    
    if(this->batch_response_.get()->response_size()==request_count_){
      empty_queue_.notify_all();
    }
    response_lock_.unlock();
  }
}

std::unique_ptr<BatchUserResponse> StrictExecutor::ExecuteBatch(
    const BatchUserRequest& request) {
  this->batch_response_ = std::make_unique<BatchUserResponse>();
  request_count_=0;
  for (const auto& sub_request : request.user_requests()) {
    KVRequest kv_request;
    if (!kv_request.ParseFromString(sub_request.request().data())) {
      LOG(ERROR) << "parse data fail";
      return std::move(this->batch_response_);
    }
    queue_lock_.lock();
    request_queue_.push(std::move(kv_request));
    request_count_++;
    queue_lock_.unlock();
    //Might want to move this/add new condition to avoid unecessary broadcasts
    not_empty_queue_.notify_all():
  }
  response_lock_.lock();
  while(this->batch_response_.get()->response_size()!=request_count_){
    if (empty_queue_.wait_for(queue_lock_, std::chrono::microseconds(100),
        [this] { return is_stop_; })) {
        response_lock_.unlock();
      return std::move(this->batch_response_);
    }
  }
  response_lock_.unlock();
  return std::move(this->batch_response_);
}

std::unique_ptr<std::string> StrictExecutor::ExecuteData(
    const KVRequest& kv_request) {
  KVResponse kv_response;

  if (kv_request.ops_size()) {
    for(const auto& op : kv_request.ops()){
      if (op.cmd() == Operation::SET) {
        Set(op.key(), op.value());
      } else if (op.cmd() == Operation::GET) {
        kv_response.set_value(Get(op.key()));
      } else if (op.cmd() == Operation::GETALLVALUES) {
        kv_response.set_value(GetAllValues());
      } else if (op.cmd() == Operation::GETRANGE) {
        kv_response.set_value(GetRange(op.key(), op.value()));
      } else if (op.cmd() == Operation::SET_WITH_VERSION) {
        SetWithVersion(op.key(), op.value(), kv_request.version());
      } else if (op.cmd() == Operation::GET_WITH_VERSION) {
        GetWithVersion(op.key(), kv_request.version(),
                      kv_response.mutable_value_info());
      } else if (op.cmd() == Operation::GET_ALL_ITEMS) {
        GetAllItems(kv_response.mutable_items());
      } else if (op.cmd() == Operation::GET_KEY_RANGE) {
        GetKeyRange(kv_request.min_key(), kv_request.max_key(),
                    kv_response.mutable_items());
      } else if (op.cmd() == Operation::GET_HISTORY) {
        GetHistory(op.key(), kv_request.min_version(),
                  kv_request.max_version(), kv_response.mutable_items());
      } else if (op.cmd() == Operation::GET_TOP) {
        GetTopHistory(op.key(), kv_request.top_number(),
                      kv_response.mutable_items());
      }
    }
  }
  else{
    if (kv_request.cmd() == Operation::SET) {
      Set(kv_request.key(), kv_request.value());
    } else if (kv_request.cmd() == Operation::GET) {
      kv_response.set_value(Get(kv_request.key()));
    } else if (kv_request.cmd() == Operation::GETALLVALUES) {
      kv_response.set_value(GetAllValues());
    } else if (kv_request.cmd() == Operation::GETRANGE) {
      kv_response.set_value(GetRange(kv_request.key(), kv_request.value()));
    } else if (kv_request.cmd() == Operation::SET_WITH_VERSION) {
      SetWithVersion(kv_request.key(), kv_request.value(), kv_request.version());
    } else if (kv_request.cmd() == Operation::GET_WITH_VERSION) {
      GetWithVersion(kv_request.key(), kv_request.version(),
                    kv_response.mutable_value_info());
    } else if (kv_request.cmd() == Operation::GET_ALL_ITEMS) {
      GetAllItems(kv_response.mutable_items());
    } else if (kv_request.cmd() == Operation::GET_KEY_RANGE) {
      GetKeyRange(kv_request.min_key(), kv_request.max_key(),
                  kv_response.mutable_items());
    } else if (kv_request.cmd() == Operation::GET_HISTORY) {
      GetHistory(kv_request.key(), kv_request.min_version(),
                kv_request.max_version(), kv_response.mutable_items());
    } else if (kv_request.cmd() == Operation::GET_TOP) {
      GetTopHistory(kv_request.key(), kv_request.top_number(),
                    kv_response.mutable_items());
    }
  }

  std::unique_ptr<std::string> resp_str = std::make_unique<std::string>();
  if (!kv_response.SerializeToString(resp_str.get())) {
    return nullptr;
  }
  return resp_str;
}

void StrictExecutor::Set(const std::string& key, const std::string& value) {
  storage_->SetValue(key, value);
}

std::string StrictExecutor::Get(const std::string& key) {
  return storage_->GetValue(key);
}

std::string StrictExecutor::GetAllValues() { return storage_->GetAllValues(); }

// Get values on a range of keys
std::string StrictExecutor::GetRange(const std::string& min_key,
                                 const std::string& max_key) {
  return storage_->GetRange(min_key, max_key);
}

void StrictExecutor::SetWithVersion(const std::string& key,
                                const std::string& value, int version) {
  storage_->SetValueWithVersion(key, value, version);
}

void StrictExecutor::GetWithVersion(const std::string& key, int version,
                                ValueInfo* info) {
  std::pair<std::string, int> ret = storage_->GetValueWithVersion(key, version);
  info->set_value(ret.first);
  info->set_version(ret.second);
}

void StrictExecutor::GetAllItems(Items* items) {
  const std::map<std::string, std::pair<std::string, int>>& ret =
      storage_->GetAllItems();
  for (auto it : ret) {
    Item* item = items->add_item();
    item->set_key(it.first);
    item->mutable_value_info()->set_value(it.second.first);
    item->mutable_value_info()->set_version(it.second.second);
  }
}

void StrictExecutor::GetKeyRange(const std::string& min_key,
                             const std::string& max_key, Items* items) {
  const std::map<std::string, std::pair<std::string, int>>& ret =
      storage_->GetKeyRange(min_key, max_key);
  for (auto it : ret) {
    Item* item = items->add_item();
    item->set_key(it.first);
    item->mutable_value_info()->set_value(it.second.first);
    item->mutable_value_info()->set_version(it.second.second);
  }
}

void StrictExecutor::GetHistory(const std::string& key, int min_version,
                            int max_version, Items* items) {
  const std::vector<std::pair<std::string, int>>& ret =
      storage_->GetHistory(key, min_version, max_version);
  for (auto it : ret) {
    Item* item = items->add_item();
    item->set_key(key);
    item->mutable_value_info()->set_value(it.first);
    item->mutable_value_info()->set_version(it.second);
  }
}

void StrictExecutor::GetTopHistory(const std::string& key, int top_number,
                               Items* items) {
  const std::vector<std::pair<std::string, int>>& ret =
      storage_->GetTopHistory(key, top_number);
  for (auto it : ret) {
    Item* item = items->add_item();
    item->set_key(key);
    item->mutable_value_info()->set_value(it.first);
    item->mutable_value_info()->set_version(it.second);
  }
}

}  // namespace resdb

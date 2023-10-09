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

#include "chain/state/chain_state.h"
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

QueccExecutor::QueccExecutor(std::unique_ptr<ChainState> state)
    : state_(std::move(state)) {
  thread_count_ = 5;
  atomic<int> ready_planner_count_(0);
  std::unique_ptr<BatchUserResponse> batch_output =
      std::make_unique<BatchUserResponse>();
  for (int thread_number = 0; thread_number < thread_count_; thread_number++) {
    batch_array_.push_back(std::vector<unique_ptr<KVRequest>>());
    sorted_transactions_.push_back(
        std::vector<std::vector<unique_ptr<KVOperation>>>(thread_count_));
    batch_ready_.push_back(false);

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

    // Planner
    std::vector<unique_ptr<KVRequest>> batch =
        std::move(batch_array_[thread_number]);

    for (const auto& txn : batch) {
      for (const auto& op : txn->ops()) {
        this->sorted_transactions_[thread_number][rid_to_range_.at(op.key())]
            .push_back(make_unique<KVOperation>(op));
      }
    }

    batch_array_[thread_number].clear();
    const int& range_being_executed = thread_number;

    execute_barrier.arrive_and_wait();

    // Executor
    for (int priority = 0; priority < thread_count_; priority++) {
      std::vector<unique_ptr<KVOperation>>& range_ops =
          this->sorted_transactions_[priority][range_being_executed];
      for (unique_ptr<KVOperation>& op : range_ops) {
        std::unique_ptr<std::string> response = ExecuteData(*op);
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
  this->batch_response_ = std::make_unique<BatchUserResponse>();
  rid_to_range_.clear();
  transaction_tracker_.clear();

  // for (int i = 0; i < (int)batch_array_.size(); i++) {
  //   batch_array_[i].clear();
  // }
  int batch_number = 0;

  std::vector<unique_ptr<KVRequest>> txn_list;

  // process through transactions
  for (const auto& sub_request : request.user_requests()) {
    KVRequest kv_request;
    if (!kv_request.ParseFromString(sub_request.request().data())) {
      LOG(ERROR) << "parse data fail";
      return std::move(this->batch_response_);
    }

    // printf("txn id: %lu\n", kv_request.txn_id());
    // printf("kv_request size: %d\n", kv_request.ops_size());
    transaction_tracker_[kv_request.txn_id()] = 0;

    if (kv_request.ops_size()) {
      transaction_tracker_[kv_request.txn_id()] = kv_request.ops_size();
    } else {
      transaction_tracker_[kv_request.txn_id()]++;
    }

    txn_list.push_back(make_unique<KVRequest>(kv_request));
  }

  int planner_vector_size =
      (txn_list.size() + thread_count_ - 1) / thread_count_;
  for (unique_ptr<KVRequest>& txn : txn_list) {
    // Set rid into hash map and push txn into correct vector
    for (const auto& op : txn->ops()) {
      unordered_map<string, int>::iterator it =
          this->rid_to_range_.find(op.key());
      if (it == this->rid_to_range_.end()) {
        this->rid_to_range_[op.key()] = 1;
      }
    }
    batch_array_[batch_number].push_back(std::move(txn));
    if ((int)batch_array_[batch_number].size() >= planner_vector_size) {
      batch_number++;
    }
  }

  // RIDs in hash map are now equal to which range they go into
  int range_count = 0;
  int range_size = ((rid_to_range_.size() - 1) / thread_count_) + 1;
  for (const auto& key : rid_to_range_) {
    rid_to_range_[key.first] = range_count / range_size;
    range_count++;
  }

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

std::unique_ptr<std::string> QueccExecutor::ExecuteData(const KVOperation& op) {
  KVResponse kv_response;
  if (op.cmd() == KVOperation::SET) {
    Set(op.key(), op.value());
  } else if (op.cmd() == KVOperation::GET) {
    kv_response.set_value(Get(op.key()));
  } else if (op.cmd() == KVOperation::GETVALUES) {
    kv_response.set_value(GetValues());
  } else if (op.cmd() == KVOperation::GETRANGE) {
    kv_response.set_value(GetRange(op.key(), op.value()));
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

  if (kv_request.ops_size()) {
    for (const auto& op : kv_request.ops()) {
      auto resp_info = kv_response.add_resp_info();
      resp_info->set_key(op.key());
      if (op.cmd() == KVOperation::SET) {
        Set(op.key(), op.value());
      } else if (op.cmd() == KVOperation::GET) {
        resp_info->set_value(Get(op.key()));
      } else if (op.cmd() == KVOperation::GETVALUES) {
        resp_info->set_value(GetValues());
      } else if (op.cmd() == KVOperation::GETRANGE) {
        resp_info->set_value(GetRange(op.key(), op.value()));
      }
    }
  } else {
    if (kv_request.cmd() == KVRequest::SET) {
      Set(kv_request.key(), kv_request.value());
    } else if (kv_request.cmd() == KVRequest::GET) {
      kv_response.set_value(Get(kv_request.key()));
    } else if (kv_request.cmd() == KVRequest::GETVALUES) {
      kv_response.set_value(GetValues());
    } else if (kv_request.cmd() == KVRequest::GETRANGE) {
      kv_response.set_value(GetRange(kv_request.key(), kv_request.value()));
    }
  }

  std::unique_ptr<std::string> resp_str = std::make_unique<std::string>();
  if (!kv_response.SerializeToString(resp_str.get())) {
    return nullptr;
  }

  return resp_str;
}

void QueccExecutor::Set(const std::string& key, const std::string& value) {
  if (!VerifyRequest(key, value)) {
    return;
  }
  state_->SetValue(key, value);
}

std::string QueccExecutor::Get(const std::string& key) {
  return state_->GetValue(key);
}

std::string QueccExecutor::GetValues() { return state_->GetAllValues(); }

// Get values on a range of keys
std::string QueccExecutor::GetRange(const std::string& min_key,
                                    const std::string& max_key) {
  return state_->GetRange(min_key, max_key);
}

Storage* QueccExecutor::GetStorage() { return state_->GetStorage(); }

}  // namespace resdb

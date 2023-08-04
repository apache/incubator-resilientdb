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

#include "platform/consensus/ordering/common/performance_manager.h"

#include <glog/logging.h>

#include "common/utils/utils.h"

namespace resdb {
namespace common {

PerformanceManager::PerformanceManager(
    const ResDBConfig& config, ReplicaCommunicator* replica_communicator,
    SystemInfo* system_info, std::unique_ptr<RequestHandler> request_handler)
    : config_(config),
      system_info_(system_info),
      replica_communicator_(replica_communicator),
      collector_pool_(std::make_unique<DataCollectorPool>(
          "response", config_.GetMaxProcessTxn() * 4, nullptr)),
      batch_queue_("user request"),
      request_handler_(std::move(request_handler)) {
  stop_ = false;
  eval_started_ = false;
  eval_ready_future_ = eval_ready_promise_.get_future();
  if (config_.GetPublicKeyCertificateInfo()
          .public_key()
          .public_key_info()
          .type() == CertificateKeyInfo::CLIENT) {
    for (int i = 0; i < 2; ++i) {
      user_req_thread_[i] =
          std::thread(&PerformanceManager::BatchProposeMsg, this);
    }
  }
  global_stats_ = Stats::GetGlobalStats();
  send_num_ = 0;
  total_num_ = 0;
}

PerformanceManager::~PerformanceManager() {
  stop_ = true;
  for (int i = 0; i < 16; ++i) {
    if (user_req_thread_[i].joinable()) {
      user_req_thread_[i].join();
    }
  }
}

// use system info
int PerformanceManager::GetPrimary() { return system_info_->GetPrimaryId(); }

std::unique_ptr<Request> PerformanceManager::GenerateUserRequest() {
  std::unique_ptr<Request> request = std::make_unique<Request>();
  request->set_data(data_func_());
  return request;
}

void PerformanceManager::SetDataFunc(std::function<std::string()> func) {
  data_func_ = std::move(func);
}

int PerformanceManager::StartEval() {
  if (eval_started_) {
    return 0;
  }
  eval_started_ = true;
  for (int i = 0; i < 60000000000; ++i) {
    std::unique_ptr<QueueItem> queue_item = std::make_unique<QueueItem>();
    queue_item->user_request = GenerateUserRequest();
    batch_queue_.Push(std::move(queue_item));
    if (i == 20000000) {
      eval_ready_promise_.set_value(true);
    }
  }
  LOG(WARNING) << "start eval done";
  return 0;
}

// =================== response ========================
// handle the response message. If receive f+1 commit messages, send back to the
// caller.
int PerformanceManager::ProcessResponseMsg(std::unique_ptr<Context> context,
                                           std::unique_ptr<Request> request) {
  // Add the response message, and use the call back to collect the received
  // messages.
  // The callback will be triggered if it received f+1 messages.
  if (request->ret() == -2) {
    // LOG(INFO) << "get response fail:" << request->ret();
    send_num_--;
    return 0;
  }

  std::string response;
  DataCollector::CollectorResultCode ret =
      AddResponseMsg(std::move(request), [&](const std::string& data) {
        response = data;
        return;
      });

  if (ret == DataCollector::CollectorResultCode::STATE_CHANGED) {
    BatchUserResponse batch_response;
    if (batch_response.ParseFromString(response)) {
      SendResponseToClient(batch_response);
    } else {
      LOG(ERROR) << "parse response fail:";
    }
  }
  return ret == DataCollector::CollectorResultCode::INVALID ? -2 : 0;
}

bool PerformanceManager::MayConsensusChangeStatus(
    int type, int received_count,
    std::atomic<DataCollector::TransactionStatue>* status) {
  switch (type) {
    case Request::TYPE_RESPONSE:
      // if receive f+1 response results, ack to the caller.
      if (*status == DataCollector::TransactionStatue::None &&
          config_.GetMinClientReceiveNum() <= received_count) {
        DataCollector::TransactionStatue old_status =
            DataCollector::TransactionStatue::None;
        return status->compare_exchange_strong(
            old_status, DataCollector::TransactionStatue::EXECUTED,
            std::memory_order_acq_rel, std::memory_order_acq_rel);
      }
      break;
  }
  return false;
}

DataCollector::CollectorResultCode PerformanceManager::AddResponseMsg(
    std::unique_ptr<Request> request,
    std::function<void(const std::string&)> response_call_back) {
  if (request == nullptr) {
    return DataCollector::CollectorResultCode::INVALID;
  }

  int type = request->type();
  uint64_t seq = request->seq();
  int resp_received_count = 0;
  int ret = collector_pool_->GetCollector(seq)->AddRequest(
      std::move(request), false,
      [&](const Request& request, int received_count,
          std::atomic<DataCollector::TransactionStatue>* status) {
        if (MayConsensusChangeStatus(type, received_count, status)) {
          resp_received_count = 1;
          response_call_back(request.data());
        }
      });
  if (ret != 0) {
    return DataCollector::CollectorResultCode::INVALID;
  }
  if (resp_received_count > 0) {
    collector_pool_->Update(seq);
    return DataCollector::CollectorResultCode::STATE_CHANGED;
  }
  return DataCollector::CollectorResultCode::OK;
}

void PerformanceManager::SendResponseToClient(
    const BatchUserResponse& batch_response) {
  uint64_t create_time = batch_response.createtime();
  uint64_t local_id = batch_response.local_id();
  if (create_time > 0) {
    uint64_t run_time = GetSysClock() - create_time;
    global_stats_->AddLatency(run_time);
  } else {
    LOG(ERROR) << "seq:" << local_id << " no resp";
  }
  send_num_--;

  if (config_.IsPerformanceRunning()) {
    return;
  }
}

// =================== request ========================
int PerformanceManager::BatchProposeMsg() {
  LOG(WARNING) << "batch wait time:" << config_.ClientBatchWaitTimeMS()
               << " batch num:" << config_.ClientBatchNum()
               << " max txn:" << config_.GetMaxProcessTxn();
  std::vector<std::unique_ptr<QueueItem>> batch_req;
  eval_ready_future_.get();
  while (!stop_) {
    if (send_num_ > config_.GetMaxProcessTxn()) {
      usleep(100000);
      continue;
    }
    if (batch_req.size() < config_.ClientBatchNum()) {
      std::unique_ptr<QueueItem> item =
          batch_queue_.Pop(config_.ClientBatchWaitTimeMS());
      if (item == nullptr) {
        continue;
      }
      batch_req.push_back(std::move(item));
      if (batch_req.size() < config_.ClientBatchNum()) {
        continue;
      }
    }
    DoBatch(batch_req);
    batch_req.clear();
  }
  return 0;
}

int PerformanceManager::DoBatch(
    const std::vector<std::unique_ptr<QueueItem>>& batch_req) {
  BatchUserRequest batch_request;
  for (size_t i = 0; i < batch_req.size(); ++i) {
    BatchUserRequest::UserRequest* req = batch_request.add_user_requests();
    *req->mutable_request() = *batch_req[i]->user_request.get();
    req->set_id(i);
  }

  batch_request.set_createtime(GetSysClock());

  std::unique_ptr<Request> new_request =
      request_handler_->ConvertToUserRequest(batch_request);

  SendMessage(*new_request);

  PostSend();
  return 0;
}

void PerformanceManager::SendMessage(const Request& new_request) {
  replica_communicator_->SendMessage(new_request, GetPrimary());
  global_stats_->BroadCastMsg();
  send_num_++;
  if (total_num_++ == 1000000) {
    stop_ = true;
    LOG(WARNING) << "total num is done:" << total_num_;
  }
  if (total_num_ % 10000 == 0) {
    LOG(WARNING) << "total num is :" << total_num_;
  }
  global_stats_->IncClientCall();
  return;
}

}  // namespace common
}  // namespace resdb

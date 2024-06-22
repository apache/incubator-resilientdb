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
    SystemInfo* system_info, SignatureVerifier* verifier)
    : config_(config),
      system_info_(system_info),
      replica_communicator_(replica_communicator),
      batch_queue_("user request"),
      verifier_(verifier) {
  stop_ = false;
  local_id_ = 0;
  send_num_ = 0;
  total_num_ = 0;

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
// user.
int PerformanceManager::ProcessResponseMsg(std::unique_ptr<Context> context,
                                           std::unique_ptr<Request> request) {
  std::unique_ptr<Request> response;
  // Add the response message, and use the call back to collect the received
  // messages.
  // The callback will be triggered if it received f+1 messages.
  if (request->ret() == -2) {
    // LOG(INFO) << "get response fail:" << request->ret();
    send_num_--;
    return 0;
  }
  int ret = AddResponseMsg(std::move(request), [&](const Request& request) {
    response = std::make_unique<Request>(request);
    return;
  });

  if (ret == 1) {
    BatchUserResponse batch_response;
    if (batch_response.ParseFromString(response->data())) {
      SendResponseToClient(batch_response);
    } else {
      LOG(ERROR) << "parse response fail:";
    }
  }
  return ret;
}

int PerformanceManager::AddResponseMsg(
    std::unique_ptr<Request> request,
    std::function<void(const Request&)> response_call_back) {
  if (request == nullptr) {
    return -2;
  }

  uint64_t seq = request->seq();
  // LOG(ERROR)<<"get response:"<<seq;
  bool done = false;
  {
    int idx = seq % response_set_size_;
    std::unique_lock<std::mutex> lk(response_lock_[idx]);
    if (response_[idx][seq] == -1) {
      return 0;
    }
    response_[idx][seq]++;
    if (response_[idx][seq] >= config_.GetMinClientReceiveNum()) {
      response_[idx][seq] = -1;
      done = true;
    }
  }
  if (done) {
    response_call_back(*request);
    return 1;
  }
  return 0;
}

void PerformanceManager::SendResponseToClient(
    const BatchUserResponse& batch_response) {
  uint64_t create_time = batch_response.createtime();
  uint64_t local_id = batch_response.local_id();
  if (create_time > 0) {
    uint64_t run_time = GetCurrentTime() - create_time;
    global_stats_->AddLatency(run_time);
  } else {
    LOG(ERROR) << "seq:" << local_id << " no resp";
  }
  send_num_--;
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
  auto new_request =
      NewRequest(Request::TYPE_NEW_TXNS, Request(), config_.GetSelfInfo().id());
  if (new_request == nullptr) {
    return -2;
  }

  BatchUserRequest batch_request;
  for (size_t i = 0; i < batch_req.size(); ++i) {
    BatchUserRequest::UserRequest* req = batch_request.add_user_requests();
    *req->mutable_request() = *batch_req[i]->user_request.get();
    req->set_id(i);
  }

  batch_request.set_createtime(GetCurrentTime());
  batch_request.set_local_id(local_id_++);
  batch_request.SerializeToString(new_request->mutable_data());
  if (verifier_) {
    auto signature_or = verifier_->SignMessage(new_request->data());
    if (!signature_or.ok()) {
      LOG(ERROR) << "Sign message fail";
      return -2;
    }
    *new_request->mutable_data_signature() = *signature_or;
  }

  new_request->set_hash(SignatureVerifier::CalculateHash(new_request->data()));
  new_request->set_proxy_id(config_.GetSelfInfo().id());

  replica_communicator_->SendMessage(*new_request, GetPrimary());
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
  return 0;
}

}  // namespace common
}  // namespace resdb

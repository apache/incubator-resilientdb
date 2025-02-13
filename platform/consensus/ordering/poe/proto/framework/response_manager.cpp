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

#include "platform/consensus/ordering/common/framework/response_manager.h"

#include <glog/logging.h>

#include "common/utils/utils.h"

namespace resdb {
namespace common {

using namespace resdb::comm;

ResponseManager::ResponseManager(const ResDBConfig& config,
                                 ReplicaCommunicator* replica_communicator,
                                 SignatureVerifier* verifier)
    : config_(config),
      replica_communicator_(replica_communicator),
      batch_queue_("user request"),
      verifier_(verifier) {
  stop_ = false;
  local_id_ = 1;

  if (config_.GetPublicKeyCertificateInfo()
              .public_key()
              .public_key_info()
              .type() == CertificateKeyInfo::CLIENT ||
      config_.IsTestMode()) {
    user_req_thread_ = std::thread(&ResponseManager::BatchProposeMsg, this);
  }
  global_stats_ = Stats::GetGlobalStats();
  send_num_ = 0;
}

ResponseManager::~ResponseManager() {
  stop_ = true;
  if (user_req_thread_.joinable()) {
    user_req_thread_.join();
  }
}

// use system info
int ResponseManager::GetPrimary() { return 1; }

int ResponseManager::NewUserRequest(std::unique_ptr<Context> context,
                                    std::unique_ptr<Request> user_request) {
  context->client = nullptr;

  std::unique_ptr<QueueItem> queue_item = std::make_unique<QueueItem>();
  queue_item->context = std::move(context);
  queue_item->user_request = std::move(user_request);

  batch_queue_.Push(std::move(queue_item));
  return 0;
}

// =================== response ========================
// handle the response message. If receive f+1 commit messages, send back to the
// caller.
int ResponseManager::ProcessResponseMsg(std::unique_ptr<Context> context,
                                        std::unique_ptr<Request> request) {
  std::unique_ptr<Request> response;
  // Add the response message, and use the call back to collect the received
  // messages.
  // The callback will be triggered if it received f+1 messages.
  if (request->ret() == -2) {
    LOG(ERROR) << "get response fail:" << request->ret();
    send_num_--;
    return 0;
  }
  CollectorResultCode ret =
      AddResponseMsg(std::move(request), [&](const Request& request) {
        response = std::make_unique<Request>(request);
        return;
      });

  if (ret == CollectorResultCode::STATE_CHANGED) {
    BatchUserResponse batch_response;
    if (batch_response.ParseFromString(response->data())) {
      SendResponseToClient(batch_response);
    } else {
      LOG(ERROR) << "parse response fail:";
    }
  }
  return ret == CollectorResultCode::INVALID ? -2 : 0;
}

CollectorResultCode ResponseManager::AddResponseMsg(
    std::unique_ptr<Request> request,
    std::function<void(const Request&)> response_call_back) {
  if (request == nullptr) {
    return CollectorResultCode::INVALID;
  }

  int type = request->type();
  uint64_t seq = request->seq();
  bool done = false;
  {
    int idx = seq % response_set_size_;
    std::unique_lock<std::mutex> lk(response_lock_[idx]);
    if (response_[idx][seq] == -1) {
      return CollectorResultCode::OK;
    }
    response_[idx][seq]++;
    if (response_[idx][seq] >= config_.GetMinClientReceiveNum()) {
      response_[idx][seq] = -1;
      done = true;
    }
  }
  if (done) {
    response_call_back(*request);
    return CollectorResultCode::STATE_CHANGED;
  }
  return CollectorResultCode::OK;
}

void ResponseManager::SendResponseToClient(
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
int ResponseManager::BatchProposeMsg() {
  LOG(INFO) << "batch wait time:" << config_.ClientBatchWaitTimeMS()
            << " batch num:" << config_.ClientBatchNum();
  std::vector<std::unique_ptr<QueueItem>> batch_req;
  while (!stop_) {
    if (send_num_ > config_.GetMaxProcessTxn()) {
      LOG(ERROR) << "send num too high, wait:" << send_num_;
      usleep(100);
      continue;
    }
    if (batch_req.size() < config_.ClientBatchNum()) {
      std::unique_ptr<QueueItem> item =
          batch_queue_.Pop(config_.ClientBatchWaitTimeMS());
      if (item != nullptr) {
        batch_req.push_back(std::move(item));
        if (batch_req.size() < config_.ClientBatchNum()) {
          continue;
        }
      }
    }
    if (batch_req.empty()) {
      continue;
    }
    int ret = DoBatch(batch_req);
    batch_req.clear();
    if (ret != 0) {
      Response response;
      response.set_result(Response::ERROR);
      for (size_t i = 0; i < batch_req.size(); ++i) {
        if (batch_req[i]->context && batch_req[i]->context->client) {
          int ret = batch_req[i]->context->client->SendRawMessage(response);
          if (ret) {
            LOG(ERROR) << "send resp" << response.DebugString()
                       << " fail ret:" << ret;
          }
        }
      }
    }
  }
  return 0;
}

int ResponseManager::DoBatch(
    const std::vector<std::unique_ptr<QueueItem>>& batch_req) {
  auto new_request =
      NewRequest(Request::TYPE_NEW_TXNS, Request(), config_.GetSelfInfo().id());
  if (new_request == nullptr) {
    return -2;
  }
  std::vector<std::unique_ptr<Context>> context_list;

  BatchUserRequest batch_request;
  for (size_t i = 0; i < batch_req.size(); ++i) {
    BatchUserRequest::UserRequest* req = batch_request.add_user_requests();
    *req->mutable_request() = *batch_req[i]->user_request.get();
    *req->mutable_signature() = batch_req[i]->context->signature;
    req->set_id(i);
    context_list.push_back(std::move(batch_req[i]->context));
  }

  if (!config_.IsPerformanceRunning()) {
    LOG(ERROR) << "add context list:" << new_request->seq()
               << " list size:" << context_list.size();
    batch_request.set_local_id(local_id_);
  }
  batch_request.set_createtime(GetCurrentTime());
  std::string data;
  batch_request.SerializeToString(&data);
  if (verifier_) {
    auto signature_or = verifier_->SignMessage(data);
    if (!signature_or.ok()) {
      LOG(ERROR) << "Sign message fail";
      return -2;
    }
    *new_request->mutable_data_signature() = *signature_or;
  }

  batch_request.SerializeToString(new_request->mutable_data());
  new_request->set_hash(SignatureVerifier::CalculateHash(new_request->data()));
  new_request->set_proxy_id(config_.GetSelfInfo().id());
  replica_communicator_->SendMessage(*new_request, GetPrimary());
  send_num_++;
  LOG(INFO) << "send msg to primary:" << GetPrimary()
            << " batch size:" << batch_req.size();
  return 0;
}

}  // namespace common
}  // namespace resdb

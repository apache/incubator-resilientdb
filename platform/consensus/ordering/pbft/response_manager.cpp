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

#include "platform/consensus/ordering/pbft/response_manager.h"

#include <glog/logging.h>

#include "common/utils/utils.h"

namespace resdb {

ResponseClientTimeout::ResponseClientTimeout(std::string hash_,
                                             uint64_t time_) {
  this->hash = hash_;
  this->timeout_time = time_;
}

ResponseClientTimeout::ResponseClientTimeout(
    const ResponseClientTimeout& other) {
  this->hash = other.hash;
  this->timeout_time = other.timeout_time;
}

bool ResponseClientTimeout::operator<(
    const ResponseClientTimeout& other) const {
  return timeout_time > other.timeout_time;
}

ResponseManager::ResponseManager(const ResDBConfig& config,
                                 ReplicaCommunicator* replica_communicator,
                                 SystemInfo* system_info,
                                 SignatureVerifier* verifier)
    : config_(config),
      replica_communicator_(replica_communicator),
      collector_pool_(std::make_unique<LockFreeCollectorPool>(
          "response", config_.GetMaxProcessTxn(), nullptr)),
      context_pool_(std::make_unique<LockFreeCollectorPool>(
          "context", config_.GetMaxProcessTxn(), nullptr)),
      batch_queue_("user request"),
      system_info_(system_info),
      verifier_(verifier) {
  stop_ = false;
  local_id_ = 1;
  timeout_length_ = 5000000;

  if (config_.GetPublicKeyCertificateInfo()
              .public_key()
              .public_key_info()
              .type() == CertificateKeyInfo::CLIENT ||
      config_.IsTestMode()) {
    user_req_thread_ = std::thread(&ResponseManager::BatchProposeMsg, this);
  }
  if (config_.GetConfigData().enable_viewchange()) {
    checking_timeout_thread_ =
        std::thread(&ResponseManager::MonitoringClientTimeOut, this);
  }
  global_stats_ = Stats::GetGlobalStats();
  send_num_ = 0;
}

ResponseManager::~ResponseManager() {
  stop_ = true;
  if (user_req_thread_.joinable()) {
    user_req_thread_.join();
  }
  if (checking_timeout_thread_.joinable()) {
    checking_timeout_thread_.join();
  }
}

// use system info
int ResponseManager::GetPrimary() { return system_info_->GetPrimaryId(); }

int ResponseManager::AddContextList(
    std::vector<std::unique_ptr<Context>> context_list, uint64_t id) {
  return context_pool_->GetCollector(id)->SetContextList(
      id, std::move(context_list));
}

std::vector<std::unique_ptr<Context>> ResponseManager::FetchContextList(
    uint64_t id) {
  auto context = context_pool_->GetCollector(id)->FetchContextList(id);
  context_pool_->Update(id);
  return context;
}

int ResponseManager::NewUserRequest(std::unique_ptr<Context> context,
                                    std::unique_ptr<Request> user_request) {
  if (!user_request->need_response()) {
    context->client = nullptr;
  }

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
      AddResponseMsg(context->signature, std::move(request),
                     [&](const Request& request,
                         const TransactionCollector::CollectorDataType*) {
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

bool ResponseManager::MayConsensusChangeStatus(
    int type, int received_count, std::atomic<TransactionStatue>* status) {
  switch (type) {
    case Request::TYPE_RESPONSE:
      // if receive f+1 response results, ack to the caller.
      if (*status == TransactionStatue::None &&
          config_.GetMinClientReceiveNum() <= received_count) {
        TransactionStatue old_status = TransactionStatue::None;
        return status->compare_exchange_strong(
            old_status, TransactionStatue::EXECUTED, std::memory_order_acq_rel,
            std::memory_order_acq_rel);
      }
      break;
  }
  return false;
}

CollectorResultCode ResponseManager::AddResponseMsg(
    const SignatureInfo& signature, std::unique_ptr<Request> request,
    std::function<void(const Request&,
                       const TransactionCollector::CollectorDataType*)>
        response_call_back) {
  if (request == nullptr) {
    return CollectorResultCode::INVALID;
  }

  std::string hash = request->hash();

  std::unique_ptr<BatchUserResponse> batch_response =
      std::make_unique<BatchUserResponse>();
  if (!batch_response->ParseFromString(request->data())) {
    LOG(ERROR) << "parse response fail:" << request->data().size()
               << " seq:" << request->seq();
    RemoveWaitingResponseRequest(hash);
    return CollectorResultCode::INVALID;
  }

  uint64_t seq = batch_response->local_id();
  request->set_seq(seq);

  int type = request->type();
  int resp_received_count = 0;
  int ret = collector_pool_->GetCollector(seq)->AddRequest(
      std::move(request), signature, false,
      [&](const Request& request, int received_count,
          TransactionCollector::CollectorDataType* data,
          std::atomic<TransactionStatue>* status, bool force) {
        if (MayConsensusChangeStatus(type, received_count, status)) {
          resp_received_count = 1;
          response_call_back(request, data);
        }
      });
  if (ret != 0) {
    return CollectorResultCode::INVALID;
  }
  if (resp_received_count > 0) {
    collector_pool_->Update(seq);
    RemoveWaitingResponseRequest(hash);
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

  if (config_.IsPerformanceRunning()) {
    return;
  }

  std::vector<std::unique_ptr<Context>> context_list =
      FetchContextList(batch_response.local_id());
  if (context_list.empty()) {
    LOG(ERROR) << "context list is empty. local id:"
               << batch_response.local_id();
    return;
  }

  for (size_t i = 0; i < context_list.size(); ++i) {
    auto& context = context_list[i];
    if (context->client == nullptr) {
      LOG(ERROR) << " no channel:";
      continue;
    }
    int ret = context->client->SendRawMessageData(batch_response.response(i));
    if (ret) {
      LOG(ERROR) << "send resp fail ret:" << ret;
    }
  }
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
               << " list size:" << context_list.size()
               << " local_id:" << local_id_;
    batch_request.set_local_id(local_id_);
    int ret = AddContextList(std::move(context_list), local_id_++);
    if (ret != 0) {
      LOG(ERROR) << "add context list fail:";
      return ret;
    }
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
  // LOG(INFO) << "send msg to primary:" << GetPrimary()
  //          << " batch size:" << batch_req.size();
  AddWaitingResponseRequest(std::move(new_request));
  return 0;
}

void ResponseManager::AddWaitingResponseRequest(
    std::unique_ptr<Request> request) {
  if (!config_.GetConfigData().enable_viewchange()) {
    return;
  }
  pm_lock_.lock();
  assert(timeout_length_ > 0);
  uint64_t time = GetCurrentTime() + timeout_length_;
  client_timeout_min_heap_.push(ResponseClientTimeout(request->hash(), time));
  waiting_response_batches_.insert(
      make_pair(request->hash(), std::move(request)));
  pm_lock_.unlock();
  sem_post(&request_sent_signal_);
}

void ResponseManager::RemoveWaitingResponseRequest(const std::string& hash) {
  if (!config_.GetConfigData().enable_viewchange()) {
    return;
  }
  pm_lock_.lock();
  if (waiting_response_batches_.find(hash) != waiting_response_batches_.end()) {
    waiting_response_batches_.erase(waiting_response_batches_.find(hash));
  }
  pm_lock_.unlock();
}

bool ResponseManager::CheckTimeOut(std::string hash) {
  pm_lock_.lock();
  bool value =
      (waiting_response_batches_.find(hash) != waiting_response_batches_.end());
  pm_lock_.unlock();
  return value;
}

std::unique_ptr<Request> ResponseManager::GetTimeOutRequest(std::string hash) {
  pm_lock_.lock();
  auto value = std::move(waiting_response_batches_.find(hash)->second);
  pm_lock_.unlock();
  return value;
}

void ResponseManager::MonitoringClientTimeOut() {
  while (!stop_) {
    sem_wait(&request_sent_signal_);
    pm_lock_.lock();
    if (client_timeout_min_heap_.empty()) {
      pm_lock_.unlock();
      continue;
    }
    auto client_timeout = client_timeout_min_heap_.top();
    client_timeout_min_heap_.pop();
    pm_lock_.unlock();

    if (client_timeout.timeout_time > GetCurrentTime()) {
      usleep(client_timeout.timeout_time - GetCurrentTime());
    }

    if (CheckTimeOut(client_timeout.hash)) {
      auto request = GetTimeOutRequest(client_timeout.hash);
      if (request) {
        replica_communicator_->BroadCast(*request);
      }
    }
  }
}
}  // namespace resdb

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

#include "platform/consensus/ordering/pbft/performance_manager.h"

#include <glog/logging.h>

#include "common/utils/utils.h"

namespace resdb {

PerformanceClientTimeout::PerformanceClientTimeout(std::string hash_,
                                                   uint64_t time_) {
  this->hash = hash_;
  this->timeout_time = time_;
}

PerformanceClientTimeout::PerformanceClientTimeout(
    const PerformanceClientTimeout& other) {
  this->hash = other.hash;
  this->timeout_time = other.timeout_time;
}

bool PerformanceClientTimeout::operator<(
    const PerformanceClientTimeout& other) const {
  return timeout_time > other.timeout_time;
}

PerformanceManager::PerformanceManager(
    const ResDBConfig& config, ReplicaCommunicator* replica_communicator,
    SystemInfo* system_info, SignatureVerifier* verifier)
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

  checking_timeout_thread_ =
      std::thread(&PerformanceManager::MonitoringClientTimeOut, this);
  global_stats_ = Stats::GetGlobalStats();
  for (size_t i = 0; i <= config_.GetReplicaNum(); i++) {
    send_num_.push_back(0);
  }
  total_num_ = 0;
  timeout_length_ = 100000000;  // 10s
}

PerformanceManager::~PerformanceManager() {
  stop_ = true;
  for (int i = 0; i < 16; ++i) {
    if (user_req_thread_[i].joinable()) {
      user_req_thread_[i].join();
    }
  }
  if (checking_timeout_thread_.joinable()) {
    checking_timeout_thread_.join();
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
  for (int i = 0; i < 60000000; ++i) {
    std::unique_ptr<QueueItem> queue_item = std::make_unique<QueueItem>();
    queue_item->context = nullptr;
    queue_item->user_request = GenerateUserRequest();
    batch_queue_.Push(std::move(queue_item));
    if (i == 2000000) {
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
  std::string hash = request->hash();
  int32_t primary_id = request->primary_id();
  uint64_t seq = request->seq();
  // Add the response message, and use the call back to collect the received
  // messages.
  // The callback will be triggered if it received f+1 messages.
  if (request->ret() == -2) {
    // LOG(INFO) << "get response fail:" << request->ret();
    // send_num_--;
    RemoveWaitingResponseRequest(hash);
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
      if (seq > highest_seq_) {
        highest_seq_ = seq;
        if (highest_seq_primary_id_ != primary_id) {
          system_info_->SetPrimary(primary_id);
          highest_seq_primary_id_ = primary_id;
        }
      }
      SendResponseToClient(batch_response);
      RemoveWaitingResponseRequest(hash);
    } else {
      LOG(ERROR) << "parse response fail:";
    }
  }
  return ret == CollectorResultCode::INVALID ? -2 : 0;
}

bool PerformanceManager::MayConsensusChangeStatus(
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

CollectorResultCode PerformanceManager::AddResponseMsg(
    const SignatureInfo& signature, std::unique_ptr<Request> request,
    std::function<void(const Request&,
                       const TransactionCollector::CollectorDataType*)>
        response_call_back) {
  if (request == nullptr) {
    return CollectorResultCode::INVALID;
  }

  std::unique_ptr<BatchUserResponse> batch_response =
      std::make_unique<BatchUserResponse>();
  if (!batch_response->ParseFromString(request->data())) {
    LOG(ERROR) << "parse response fail:" << request->data().size()
               << " seq:" << request->seq();
    return CollectorResultCode::INVALID;
  }

  uint64_t seq = batch_response->local_id();

  int type = request->type();
  seq = request->seq();
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
    return CollectorResultCode::STATE_CHANGED;
  }
  return CollectorResultCode::OK;
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
  {
    // std::lock_guard<std::mutex> lk(mutex_);
    if (send_num_[batch_response.primary_id()] > 0) {
      send_num_[batch_response.primary_id()]--;
    }
  }

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
    // std::lock_guard<std::mutex> lk(mutex_);
    if (send_num_[GetPrimary()] >= config_.GetMaxProcessTxn()) {
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

int PerformanceManager::DoBatch(
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
    if (batch_req[i]->context) {
      *req->mutable_signature() = batch_req[i]->context->signature;
    }
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
  send_num_[GetPrimary()]++;
  if (total_num_++ == 1000000) {
    stop_ = true;
    LOG(WARNING) << "total num is done:" << total_num_;
  }
  if (total_num_ % 10000 == 0) {
    LOG(WARNING) << "total num is :" << total_num_;
  }
  global_stats_->IncClientCall();
  AddWaitingResponseRequest(std::move(new_request));
  return 0;
}

void PerformanceManager::AddWaitingResponseRequest(
    std::unique_ptr<Request> request) {
  if (!config_.GetConfigData().enable_viewchange()) {
    return;
  }
  pm_lock_.lock();
  uint64_t time = GetCurrentTime() + this->timeout_length_;
  client_timeout_min_heap_.push(
      PerformanceClientTimeout(request->hash(), time));
  waiting_response_batches_.insert(
      make_pair(request->hash(), std::move(request)));
  pm_lock_.unlock();
  sem_post(&request_sent_signal_);
}

void PerformanceManager::RemoveWaitingResponseRequest(std::string hash) {
  if (!config_.GetConfigData().enable_viewchange()) {
    return;
  }
  pm_lock_.lock();
  if (waiting_response_batches_.find(hash) != waiting_response_batches_.end()) {
    waiting_response_batches_.erase(waiting_response_batches_.find(hash));
  }
  pm_lock_.unlock();
}

bool PerformanceManager::CheckTimeOut(std::string hash) {
  pm_lock_.lock();
  bool value =
      (waiting_response_batches_.find(hash) != waiting_response_batches_.end());
  pm_lock_.unlock();
  return value;
}

std::unique_ptr<Request> PerformanceManager::GetTimeOutRequest(
    std::string hash) {
  pm_lock_.lock();
  auto value = std::move(waiting_response_batches_.find(hash)->second);
  pm_lock_.unlock();
  return value;
}

void PerformanceManager::MonitoringClientTimeOut() {
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

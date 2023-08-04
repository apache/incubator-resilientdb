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

#include "platform/consensus/ordering/tendermint/performance_manager.h"

#include <glog/logging.h>

#include "common/utils/utils.h"

namespace resdb {
namespace tendermint {

std::unique_ptr<Request> PerformanceManager::NewRequest(
    const TendermintRequest& request) {
  TendermintRequest new_request(request);
  new_request.set_type(TendermintRequest::TYPE_NEWREQUEST);
  new_request.set_sender_id(config_.GetSelfInfo().id());
  // LOG(ERROR) << "send type:" <<
  // TendermintRequest_Type_Name(new_request.type());
  auto ret = std::make_unique<Request>();
  new_request.SerializeToString(ret->mutable_data());
  return ret;
}

PerformanceManager::PerformanceManager(
    const ResDBConfig& config, ReplicaCommunicator* replica_communicator)
    : config_(config),
      replica_communicator_(replica_communicator),
      batch_queue_("user request") {
  stop_ = false;
  eval_started_ = false;
  eval_ready_future_ = eval_ready_promise_.get_future();
  if (config_.GetPublicKeyCertificateInfo()
          .public_key()
          .public_key_info()
          .type() == CertificateKeyInfo::CLIENT) {
    for (int i = 0; i < 1; ++i) {
      user_req_thread_[i] =
          std::thread(&PerformanceManager::BatchProposeMsg, this);
    }
  }
  global_stats_ = Stats::GetGlobalStats();
  send_num_ = 0;
  total_num_ = 0;
  primary_id_ = 0;
  LOG(ERROR) << "performance manager";
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
int PerformanceManager::GetPrimary() {
  return (primary_id_ % config_.GetReplicaInfos().size()) + 1;
}

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
// client.
int PerformanceManager::ProcessResponseMsg(std::unique_ptr<Context> context,
                                           std::unique_ptr<Request> request) {
  std::unique_lock<std::mutex> lk(response_mutex_);
  LOG(ERROR) << "get response from:" << request->sender_id();
  BatchUserResponse batch_response;
  if (batch_response.ParseFromString(request->data())) {
    response_nodes_list_[batch_response.local_id()].insert(
        request->sender_id());
    LOG(ERROR) << "get id:" << batch_response.local_id() << " size:"
               << response_nodes_list_[batch_response.local_id()].size();
    if (response_nodes_list_[batch_response.local_id()].size() !=
        config_.GetMinClientReceiveNum()) {
      return 0;
    }
    SendResponseToClient(batch_response);
  } else {
    LOG(ERROR) << "parse response fail:";
  }
  return 0;
}

void PerformanceManager::SendResponseToClient(
    const BatchUserResponse& batch_response) {
  uint64_t create_time = batch_response.createtime();
  uint64_t local_id = batch_response.local_id();
  if (create_time > 0) {
    uint64_t run_time = GetSysClock() - create_time;
    global_stats_->AddLatency(run_time);
    LOG(ERROR) << "latency:" << run_time << " send num:" << send_num_
               << " id:" << local_id;
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
      LOG(INFO) << "send num too high, wait:" << send_num_;
      usleep(100);
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
  BatchUserRequest batch_request;
  for (size_t i = 0; i < batch_req.size(); ++i) {
    BatchUserRequest::UserRequest* req = batch_request.add_user_requests();
    *req->mutable_request() = *batch_req[i]->user_request.get();
    if (batch_req[i]->context) {
      *req->mutable_signature() = batch_req[i]->context->signature;
    }
    req->set_id(i);
  }

  std::string data;
  batch_request.SerializeToString(&data);
  batch_request.set_createtime(GetSysClock());
  batch_request.set_local_id(local_id_++);

  TendermintRequest tendermint_request;
  batch_request.SerializeToString(tendermint_request.mutable_data());
  tendermint_request.set_proxy_id(config_.GetSelfInfo().id());
  LOG(INFO) << "send msg to primary:" << GetPrimary()
            << " batch size:" << batch_req.size() << " id:" << local_id_ - 1;
  return SendMessage(tendermint_request);
}

int PerformanceManager::SendMessage(
    const TendermintRequest& tendermint_request) {
  std::unique_ptr<Request> request = NewRequest(tendermint_request);
  replica_communicator_->SendMessage(*request, GetPrimary());
  primary_id_++;
  send_num_++;
  global_stats_->IncClientCall();
  if (total_num_++ == 1000000) {
    stop_ = true;
    LOG(WARNING) << "total num is done:" << total_num_;
  }
  if (total_num_ % 10000 == 0) {
    LOG(WARNING) << "total num is :" << total_num_;
  }

  return 0;
}

}  // namespace tendermint
}  // namespace resdb

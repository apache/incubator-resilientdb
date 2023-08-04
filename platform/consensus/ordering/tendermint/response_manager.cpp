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

#include "platform/consensus/ordering/tendermint/response_manager.h"

#include <glog/logging.h>

#include "common/utils/utils.h"

namespace resdb {
namespace tendermint {

ResponseManager::ResponseManager(const ResDBConfig& config,
                                 ReplicaCommunicator* replica_communicator)
    : config_(config),
      replica_communicator_(replica_communicator),
      batch_queue_("user request") {
  stop_ = false;
  user_req_thread_ = std::thread(&ResponseManager::BatchProposeMsg, this);
  send_num_ = 0;
  primary_id_ = 0;
}

ResponseManager::~ResponseManager() {
  stop_ = true;
  if (user_req_thread_.joinable()) {
    user_req_thread_.join();
  }
}

std::unique_ptr<Request> ResponseManager::NewRequest(
    const TendermintRequest& request) {
  TendermintRequest new_request(request);
  new_request.set_type(TendermintRequest::TYPE_NEWREQUEST);
  new_request.set_sender_id(config_.GetSelfInfo().id());
  LOG(ERROR) << "send type:" << TendermintRequest_Type_Name(new_request.type());
  auto ret = std::make_unique<Request>();
  new_request.SerializeToString(ret->mutable_data());
  return ret;
}

// use system info
int ResponseManager::GetPrimary() {
  return (primary_id_ % config_.GetReplicaInfos().size()) + 1;
}

int ResponseManager::AddContextList(
    std::vector<std::unique_ptr<Context>> context_list, uint64_t id) {
  std::unique_lock<std::mutex> lk(client_list_mutex_);
  client_list_[id] = std::move(context_list);
  return 0;
}

std::vector<std::unique_ptr<Context>> ResponseManager::FetchContextList(
    uint64_t id) {
  std::unique_lock<std::mutex> lk(client_list_mutex_);
  return std::move(client_list_[id]);
}

void ResponseManager::ClearContextList(uint64_t id) {
  std::unique_lock<std::mutex> lk(client_list_mutex_);
  if (client_list_.find(id) == client_list_.end()) return;
  client_list_.erase(client_list_.find(id));
}

bool ResponseManager::IsExistContextList(uint64_t id) {
  std::unique_lock<std::mutex> lk(client_list_mutex_);
  return client_list_.find(id) != client_list_.end();
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

// handle the response message. If receive f+1 commit messages, send back to the
// client.
int ResponseManager::ProcessResponseMsg(std::unique_ptr<Context> context,
                                        std::unique_ptr<Request> request) {
  std::unique_lock<std::mutex> lk(response_mutex_);
  LOG(ERROR) << "get response from:" << request->sender_id();
  BatchUserResponse batch_response;
  if (batch_response.ParseFromString(request->data())) {
    if (!IsExistContextList(batch_response.local_id())) {
      LOG(ERROR) << "client not exist:" << batch_response.local_id();
      return 0;
    }
    response_nodes_list_[batch_response.local_id()].insert(
        request->sender_id());
    LOG(ERROR) << "get size:"
               << response_nodes_list_[batch_response.local_id()].size();
    if (response_nodes_list_[batch_response.local_id()].size() <
        config_.GetMinClientReceiveNum()) {
      return 0;
    }
    SendResponseToClient(batch_response);
    ClearContextList(batch_response.local_id());
    response_nodes_list_[batch_response.local_id()].clear();
  } else {
    LOG(ERROR) << "parse response fail:";
  }
  return 0;
}

void ResponseManager::SendResponseToClient(
    const BatchUserResponse& batch_response) {
  uint64_t create_time = batch_response.createtime();
  uint64_t local_id = batch_response.local_id();
  if (create_time > 0) {
    uint64_t run_time = GetSysClock() - create_time;
    // global_stats_->AddLatency(run_time);
  } else {
    LOG(ERROR) << "seq:" << local_id << " no resp";
  }
  send_num_--;

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
      LOG(ERROR) << " no client:";
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
  std::vector<std::unique_ptr<Context>> context_list;

  BatchUserRequest batch_request;
  for (size_t i = 0; i < batch_req.size(); ++i) {
    BatchUserRequest::UserRequest* req = batch_request.add_user_requests();
    *req->mutable_request() = *batch_req[i]->user_request.get();
    *req->mutable_signature() = batch_req[i]->context->signature;
    req->set_id(i);
    context_list.push_back(std::move(batch_req[i]->context));
  }

  batch_request.set_local_id(local_id_);
  int ret = AddContextList(std::move(context_list), local_id_++);
  if (ret != 0) {
    LOG(ERROR) << "add context list fail:";
    return ret;
  }

  std::string data;
  batch_request.SerializeToString(&data);
  batch_request.set_createtime(GetSysClock());

  TendermintRequest tendermint_request;
  batch_request.SerializeToString(tendermint_request.mutable_data());
  tendermint_request.set_proxy_id(config_.GetSelfInfo().id());
  LOG(INFO) << "send msg to primary:" << GetPrimary()
            << " batch size:" << batch_req.size();
  return SendMessage(tendermint_request);
}

int ResponseManager::SendMessage(const TendermintRequest& tendermint_request) {
  std::unique_ptr<Request> request = NewRequest(tendermint_request);
  replica_communicator_->SendMessage(*request, GetPrimary());
  primary_id_++;
  send_num_++;
  return 0;
}

}  // namespace tendermint
}  // namespace resdb

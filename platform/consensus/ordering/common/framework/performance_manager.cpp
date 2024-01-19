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

#include "platform/consensus/ordering/common/framework/performance_manager.h"

#include <glog/logging.h>

#include "common/utils/utils.h"

namespace resdb {
namespace common {

using comm::CollectorResultCode;

PerformanceManager::PerformanceManager(
    const ResDBConfig& config, ReplicaCommunicator* replica_communicator,
    SignatureVerifier* verifier)
     : config_(config),
      replica_communicator_(replica_communicator),
      batch_queue_("user request"),
      verifier_(verifier) {
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
  replica_num_ = config_.GetReplicaNum();
  id_ = config_.GetSelfInfo().id();
  primary_ = id_ % replica_num_;
  if (primary_ == 0) primary_ = replica_num_;
  local_id_ = 1;
  sum_ = 0;
}

PerformanceManager::~PerformanceManager() {
  stop_ = true;
  for (int i = 0; i < 16; ++i) {
    if (user_req_thread_[i].joinable()) {
      user_req_thread_[i].join();
    }
  }
}

int PerformanceManager::GetPrimary() { return primary_; }

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
  for (int i = 0; i < 600000000; ++i) {
    // for (int i = 0; i < 60000000000; ++i) {
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
  // Add the response message, and use the call back to collect the received
  // messages.
  // The callback will be triggered if it received f+1 messages.
  if (request->ret() == -2) {
    // LOG(INFO) << "get response fail:" << request->ret();
    send_num_--;
    return 0;
  }

  //LOG(INFO) << "get response:" << request->seq() << " sender:"<<request->sender_id();
  std::unique_ptr<BatchUserResponse> batch_response = nullptr;
  CollectorResultCode ret =
      AddResponseMsg(std::move(request), [&](std::unique_ptr<BatchUserResponse> request) {
        batch_response = std::move(request);
        return;
      });

  if (ret == CollectorResultCode::STATE_CHANGED) {
    assert(batch_response);
      SendResponseToClient(*batch_response);
  }
  return ret == CollectorResultCode::INVALID ? -2 : 0;
}

CollectorResultCode PerformanceManager::AddResponseMsg(
    std::unique_ptr<Request> request,
    std::function<void(std::unique_ptr<BatchUserResponse>)> response_call_back) {
  if (request == nullptr) {
    return CollectorResultCode::INVALID;
  }

  //uint64_t seq = request->seq();

  std::unique_ptr<BatchUserResponse> batch_response = std::make_unique<BatchUserResponse>();
  if (!batch_response->ParseFromString(request->data())) {
    LOG(ERROR) << "parse response fail:"<<request->data().size()
    <<" seq:"<<request->seq(); return CollectorResultCode::INVALID;
  }

  uint64_t seq = batch_response->local_id();


  bool done = false;
  {
    int idx = seq % response_set_size_;
    std::unique_lock<std::mutex> lk(response_lock_[idx]);
    if (response_[idx].find(seq) == response_[idx].end()) {
      //LOG(ERROR)<<"has done local seq:"<<seq<<" global seq:"<<request->seq();
      return CollectorResultCode::OK;
    }
    response_[idx][seq]++;
    //LOG(ERROR)<<"get seq :"<<request->seq()<<" local id:"<<seq<<" num:"<<response_[idx][seq]<<" send:"<<send_num_;
    if (response_[idx][seq] >= config_.GetMinClientReceiveNum()) {
      //LOG(ERROR)<<"get seq :"<<request->seq()<<" local id:"<<seq<<" num:"<<response_[idx][seq]<<" done:"<<send_num_;
      response_[idx].erase(response_[idx].find(seq));
      done = true;
    }
  }
  if (done) {
    response_call_back(std::move(batch_response));
    return CollectorResultCode::STATE_CHANGED;
  }
  return CollectorResultCode::OK;
}

void PerformanceManager::SendResponseToClient(
    const BatchUserResponse& batch_response) {
  uint64_t create_time = batch_response.createtime();
  if (create_time > 0) {
    uint64_t run_time = GetCurrentTime() - create_time;
    global_stats_->AddLatency(run_time);
  } else {
  }
  //send_num_-=10;
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
      // LOG(ERROR)<<"wait send num:"<<send_num_;
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
  auto new_request = comm::NewRequest(Request::TYPE_NEW_TXNS, Request(),
                                      config_.GetSelfInfo().id());
  if (new_request == nullptr) {
    return -2;
  }

  BatchUserRequest batch_request;
  for (size_t i = 0; i < batch_req.size(); ++i) {
    BatchUserRequest::UserRequest* req = batch_request.add_user_requests();
    *req->mutable_request() = *batch_req[i]->user_request.get();
    if (batch_req[i]->context) {
      *req->mutable_signature() = batch_req[i]->context->signature;
    }
    req->set_id(i);
  }

  batch_request.set_local_id(local_id_++);

  {
    int idx = batch_request.local_id() % response_set_size_;
    std::unique_lock<std::mutex> lk(response_lock_[idx]);
    response_[idx][batch_request.local_id()]++;
  }

  batch_request.set_createtime(GetCurrentTime());
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
  new_request->set_user_seq(batch_request.local_id());

  SendMessage(*new_request);

  global_stats_->BroadCastMsg();
  send_num_++;
  sum_ += batch_req.size();
  //LOG(ERROR)<<"send num:"<<send_num_<<" total num:"<<total_num_<<" sum:"<<sum_<<" to:"<<GetPrimary();
  if (total_num_++ == 1000000) {
    stop_ = true;
    LOG(WARNING) << "total num is done:" << total_num_;
  }
  if (total_num_ % 1000 == 0) {
    LOG(WARNING) << "total num is :" << total_num_;
  }
  global_stats_->IncClientCall();
  return 0;
}

void PerformanceManager::SendMessage(const Request& request){
  replica_communicator_->SendMessage(request, GetPrimary());
}

}  // namespace common
}  // namespace resdb

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

#include "platform/consensus/ordering/pbft/message_manager.h"

#include <glog/logging.h>

#include "common/utils/utils.h"

namespace resdb {

MessageManager::MessageManager(
    const ResDBConfig& config,
    std::unique_ptr<TransactionManager> transaction_manager,
    CheckPointManager* checkpoint_manager, SystemInfo* system_info)
    : config_(config),
      queue_("executed"),
      txn_db_(checkpoint_manager->GetTxnDB()),
      system_info_(system_info),
      checkpoint_manager_(checkpoint_manager),
      transaction_executor_(std::make_unique<TransactionExecutor>(
          config,
          [&](std::unique_ptr<Request> request,
              std::unique_ptr<BatchUserResponse> resp_msg) {
            if (request->is_recovery()) {
              if (checkpoint_manager_) {
                checkpoint_manager_->AddCommitData(std::move(request));
              }
              return;
            }
            resp_msg->set_proxy_id(request->proxy_id());
            resp_msg->set_seq(request->seq());
            resp_msg->set_current_view(request->current_view());
            resp_msg->set_primary_id(GetCurrentPrimary());
            if (transaction_executor_->NeedResponse() &&
                resp_msg->proxy_id() != 0) {
              queue_.Push(std::move(resp_msg));
            }
            if (checkpoint_manager_) {
              checkpoint_manager_->AddCommitData(std::move(request));
            }
          },
          system_info_, std::move(transaction_manager))),
      collector_pool_(std::make_unique<LockFreeCollectorPool>(
          "txn", config_.GetMaxProcessTxn(), transaction_executor_.get(),
          config_.GetConfigData().enable_viewchange())) {
  global_stats_ = Stats::GetGlobalStats();
  transaction_executor_->SetSeqUpdateNotifyFunc(
      [&](uint64_t seq) { collector_pool_->Update(seq - 1); });
  checkpoint_manager_->SetExecutor(transaction_executor_.get());
}

MessageManager::~MessageManager() {
  if (transaction_executor_) {
    transaction_executor_->Stop();
  }
}

std::unique_ptr<BatchUserResponse> MessageManager::GetResponseMsg() {
  return queue_.Pop();
}

int64_t MessageManager::GetCurrentPrimary() const {
  return system_info_->GetPrimaryId();
}

uint64_t MessageManager ::GetCurrentView() const {
  return system_info_->GetCurrentView();
}

void MessageManager::SetNextSeq(uint64_t seq) {
  next_seq_ = seq;
  LOG(ERROR) << "set next seq:" << next_seq_;
}

int64_t MessageManager::GetNextSeq() { return next_seq_; }

absl::StatusOr<uint64_t> MessageManager::AssignNextSeq() {
  std::unique_lock<std::mutex> lk(seq_mutex_);
  uint32_t max_executed_seq = transaction_executor_->GetMaxPendingExecutedSeq();
  global_stats_->SeqGap(next_seq_ - max_executed_seq);
  if (next_seq_ - max_executed_seq >
      static_cast<uint64_t>(config_.GetMaxProcessTxn())) {
    // LOG(ERROR) << "next_seq_: " << next_seq_ << " max_executed_seq: " <<
    // max_executed_seq;
    return absl::InvalidArgumentError("Seq has been used up.");
  }
  return next_seq_++;
}

std::vector<ReplicaInfo> MessageManager::GetReplicas() {
  return system_info_->GetReplicas();
}

// Check if the request is valid.
// 1. view is the same as the current view
// 2. seq is larger or equal than the next execute seq.
// 3. inside the water mark.
bool MessageManager::IsValidMsg(const Request& request) {
  if (request.type() == Request::TYPE_RESPONSE) {
    return true;
  }
  // view should be the same as the current one.
  if (static_cast<uint64_t>(request.current_view()) != GetCurrentView()) {
    LOG(ERROR) << "message view :[" << request.current_view()
               << "] is older than the cur view :[" << GetCurrentView() << "]";
    return false;
  }

  if (static_cast<uint64_t>(request.seq()) <
      transaction_executor_->GetMaxPendingExecutedSeq()) {
    return false;
  }

  return true;
}

bool MessageManager::MayConsensusChangeStatus(
    int type, int received_count, std::atomic<TransactionStatue>* status,
    bool ret) {
  switch (type) {
    case Request::TYPE_PRE_PREPARE:
      if (*status == TransactionStatue::None) {
        TransactionStatue old_status = TransactionStatue::None;
        return status->compare_exchange_strong(
            old_status, TransactionStatue::READY_PREPARE,
            std::memory_order_acq_rel, std::memory_order_acq_rel);
      }
      break;
    case Request::TYPE_PREPARE:
      if (*status == TransactionStatue::READY_PREPARE &&
          config_.GetMinDataReceiveNum() <= received_count) {
        TransactionStatue old_status = TransactionStatue::READY_PREPARE;
        return status->compare_exchange_strong(
            old_status, TransactionStatue::READY_COMMIT,
            std::memory_order_acq_rel, std::memory_order_acq_rel);
      }
      break;
    case Request::TYPE_COMMIT:
      if (*status == TransactionStatue::READY_COMMIT &&
          config_.GetMinDataReceiveNum() <= received_count) {
        TransactionStatue old_status = TransactionStatue::READY_COMMIT;
        return status->compare_exchange_strong(
            old_status, TransactionStatue::READY_EXECUTE,
            std::memory_order_acq_rel, std::memory_order_acq_rel);
        return true;
      }
      break;
  }
  return ret;
}

// Add commit messages and return the number of messages have been received.
// The commit messages only include post(pre-prepare), prepare and commit
// messages. Messages are handled by state (PREPARE,COMMIT,READY_EXECUTE).

// If there are enough messages and the state is changed after adding the
// message, return 1, otherwise return 0. Return -2 if the request is not valid.
CollectorResultCode MessageManager::AddConsensusMsg(
    const SignatureInfo& signature, std::unique_ptr<Request> request) {
  if (request == nullptr || !IsValidMsg(*request)) {
    return CollectorResultCode::INVALID;
  }
  int type = request->type();
  uint64_t seq = request->seq();
  int resp_received_count = 0;
  int proxy_id = request->proxy_id();

  int ret = collector_pool_->GetCollector(seq)->AddRequest(
      std::move(request), signature, type == Request::TYPE_PRE_PREPARE,
      [&](const Request& request, int received_count,
          TransactionCollector::CollectorDataType* data,
          std::atomic<TransactionStatue>* status, bool force) {
        if (MayConsensusChangeStatus(type, received_count, status, force)) {
          resp_received_count = 1;
        }
      });
  if (ret == 1) {
    SetLastCommittedTime(proxy_id);
  } else if (ret != 0) {
    return CollectorResultCode::INVALID;
  }
  if (resp_received_count > 0) {
    return CollectorResultCode::STATE_CHANGED;
  }
  return CollectorResultCode::OK;
}

RequestSet MessageManager::GetRequestSet(uint64_t min_seq, uint64_t max_seq) {
  RequestSet ret;
  std::unique_lock<std::mutex> lk(data_mutex_);
  for (uint64_t i = min_seq; i <= max_seq; ++i) {
    if (committed_data_.find(i) == committed_data_.end()) {
      LOG(ERROR) << "seq :" << i << " doesn't exist";
      continue;
    }
    RequestWithProof* request = ret.add_requests();
    *request->mutable_request() = committed_data_[i];
    request->set_seq(i);
    for (const auto& request_info : committed_proof_[i]) {
      RequestWithProof::RequestData* data = request->add_proofs();
      *data->mutable_request() = *request_info->request;
      *data->mutable_signature() = request_info->signature;
    }
  }
  return ret;
}

// Get the transactions that have been execuited.
Request* MessageManager::GetRequest(uint64_t seq) { return txn_db_->Get(seq); }

std::vector<RequestInfo> MessageManager::GetPreparedProof(uint64_t seq) {
  return collector_pool_->GetCollector(seq)->GetPreparedProof();
}

TransactionStatue MessageManager::GetTransactionState(uint64_t seq) {
  return collector_pool_->GetCollector(seq)->GetStatus();
}

int MessageManager::GetReplicaState(ReplicaState* state) {
  *state->mutable_replica_config() = config_.GetConfigData();
  return 0;
}

Storage* MessageManager::GetStorage() {
  return transaction_executor_->GetStorage();
}

void MessageManager::SetLastCommittedTime(uint64_t proxy_id) {
  lct_lock_.lock();
  last_committed_time_[proxy_id] = GetCurrentTime();
  lct_lock_.unlock();
}

uint64_t MessageManager::GetLastCommittedTime(uint64_t proxy_id) {
  lct_lock_.lock();
  auto value = last_committed_time_[proxy_id];
  lct_lock_.unlock();
  return value;
}

bool MessageManager::IsPreapared(uint64_t seq) {
  return collector_pool_->GetCollector(seq)->IsPrepared();
}

uint64_t MessageManager::GetHighestPreparedSeq() {
  return checkpoint_manager_->GetHighestPreparedSeq();
}

void MessageManager::SetHighestPreparedSeq(uint64_t seq) {
  return checkpoint_manager_->SetHighestPreparedSeq(seq);
}

void MessageManager::SetDuplicateManager(DuplicateManager* manager) {
  transaction_executor_->SetDuplicateManager(manager);
}

void MessageManager::SendResponse(std::unique_ptr<Request> request) {
  std::unique_ptr<BatchUserResponse> response =
      std::make_unique<BatchUserResponse>();
  response->set_createtime(GetCurrentTime());
  // response->set_local_id(batch_request.local_id());
  response->set_hash(request->hash());
  response->set_proxy_id(request->proxy_id());
  response->set_seq(request->seq());
  response->set_current_view(GetCurrentView());
  response->set_primary_id(GetCurrentPrimary());
  if (transaction_executor_->NeedResponse() && response->proxy_id() != 0) {
    queue_.Push(std::move(response));
  }
}

LockFreeCollectorPool* MessageManager::GetCollectorPool() {
  return collector_pool_.get();
}

}  // namespace resdb

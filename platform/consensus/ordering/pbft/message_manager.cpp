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

#include <utility>

#include "common/utils/utils.h"

namespace resdb {

MessageManager::MessageManager(
    const ResDBConfig& config,
    std::unique_ptr<TransactionManager> transaction_manager,
    CheckPointManager* checkpoint_manager, SystemInfo* system_info)
    : config_(config),
      queue_("executed"),
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
            bool response_held = false;
            if (response_hold_predicate_ &&
                response_hold_predicate_(*request)) {
              HeldResponseKey key(request->seq(), request->hash());
              {
                std::lock_guard<std::mutex> lk(held_response_mutex_);
                auto released_it = released_before_hold_.find(key);
                if (released_it != released_before_hold_.end()) {
                  // A POE cert arrived before this replica finished execution.
                  // Consume that early release and let the response continue.
                  released_before_hold_.erase(released_it);
                } else {
                  // Normal POE path: hold the optimistic response until a
                  // valid local POE certificate releases it.
                  rollback_dropped_held_responses_.erase(key);
                  held_responses_[key] = HeldResponse{*request, *resp_msg};
                  response_held = true;
                }
              }
            }
            if (post_execute_hook_) {
              post_execute_hook_(*request, *resp_msg);
            }
            // Flat PBFT/3PC leave response_filter_ unset, so responses are
            // queued normally. Sharded modes install a filter so only replicas
            // in the coordinator shard send client/proxy responses.
            if (!response_held) {
              QueueResponseIfAllowed(*request, std::move(resp_msg));
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
  checkpoint_manager_->SetResetExecute(
      [&](uint64_t seq) { SetNextCommitSeq(seq); });
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
  LOG(ERROR) << "set next old seq:" << next_seq_;
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
    LOG(ERROR) << " msg not invalid";
    return CollectorResultCode::INVALID;
  }

  int type = request->type();
  uint64_t seq = request->seq();
  int resp_received_count = 0;
  int proxy_id = request->proxy_id();
  if (checkpoint_manager_->IsCommitted(seq)) {
    LOG(ERROR) << " seq:" << seq << " type:" << type << " has been committed";
    return CollectorResultCode::STATE_CHANGED;
  }

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
    LOG(ERROR) << " add request fail";
    return CollectorResultCode::INVALID;
  }
  if (resp_received_count > 0) {
    if (type == Request::TYPE_COMMIT) {
      if (checkpoint_manager_) {
        checkpoint_manager_->AddCommitState(seq);
      }
    }
    return CollectorResultCode::STATE_CHANGED;
  }
  return CollectorResultCode::OK;
}

std::vector<RequestInfo> MessageManager::GetPreparedProof(uint64_t seq) {
  return collector_pool_->GetCollector(seq)->GetPreparedProof();
}


int MessageManager::ExecuteOrderedRequest(std::unique_ptr<Request> request) {
  if (request == nullptr) {
    LOG(ERROR) << "ExecuteOrderedRequest(): null request";
    return -2;
  }

  // Check if the request is valid before executing (sanity check as request should already be validated). 
  if (!IsValidMsg(*request)) {
    LOG(ERROR) << "ExecuteOrderedRequest(): invalid request, seq:"
               << request->seq() << " view:" << request->current_view();
    return -2;
  }

  const uint64_t seq = request->seq();
  const uint64_t proxy_id = request->proxy_id();

  // If this sequence is already committed, do not execute again.
  if (checkpoint_manager_ && checkpoint_manager_->IsCommitted(seq)) {
    LOG(ERROR) << "ExecuteOrderedRequest(): seq already committed: " << seq;
    return 0;
  }

  // Mirror the PBFT path: mark the sequence committed before handing it to
  // the executor. In PBFT this happens when TYPE_COMMIT changes state.
  if (checkpoint_manager_) {
    checkpoint_manager_->AddCommitState(seq);
  }

  SetLastCommittedTime(proxy_id);

  // Send directly to the executor. The executor callback already pushes
  // BatchUserResponse objects into queue_ and checkpoint_manager_->AddCommitData
  // when execution completes.
  transaction_executor_->Commit(std::move(request));
  return 0;
}

int MessageManager::GetReplicaState(ReplicaState* state) {
  *state->mutable_replica_config() = config_.GetConfigData();
  return 0;
}

Storage* MessageManager::GetStorage() {
  return transaction_executor_->GetStorage();
}

void MessageManager::SetNextCommitSeq(int seq) {
  LOG(ERROR) << " set next commit seq:" << seq;
  SetNextSeq(seq);
  SetHighestPreparedSeq(seq);
  collector_pool_->Reset(seq);
  checkpoint_manager_->SetLastCommit(seq - 1);
  return transaction_executor_->SetPendingExecutedSeq(seq);
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

void MessageManager::SetResponseFilter(
    std::function<bool(const Request&)> filter) {
  // Optional response gate used by sharded 3PC. The filter is evaluated after
  // execution, before the response is placed on the send queue.
  response_filter_ = std::move(filter);
}

void MessageManager::SetPostExecuteHook(PostExecuteHook hook) {
  // Optional hook used by sharded 3PC/POE to create an execution proof after
  // the executor has produced the response. Unset by default for all existing
  // PBFT, flat 3PC, and sharded 3PC/PBFT paths.
  post_execute_hook_ = std::move(hook);
}

void MessageManager::SetResponseHoldPredicate(
    ResponseHoldPredicate predicate) {
  // POE installs this predicate so optimistic execution does not immediately
  // reach the client. Existing PBFT/3PC modes leave it unset.
  response_hold_predicate_ = std::move(predicate);
}

int MessageManager::ReleaseHeldResponse(uint64_t seq,
                                        const std::string& hash) {
  HeldResponse held_response;
  bool has_response = false;
  HeldResponseKey key(seq, hash);
  {
    std::lock_guard<std::mutex> lk(held_response_mutex_);
    auto it = held_responses_.find(key);
    if (it == held_responses_.end()) {
      if (rollback_dropped_held_responses_.find(key) !=
          rollback_dropped_held_responses_.end()) {
        // Rollback already discarded this response, so a delayed cert should
        // not recreate the release-before-hold race.
        return 0;
      }
      // Cert-before-execution race: remember the release so the later hold
      // point can queue the response immediately.
      released_before_hold_.insert(key);
      return 0;
    }
    held_response = it->second;
    held_responses_.erase(it);
    released_before_hold_.erase(key);
    has_response = true;
  }

  if (has_response) {
    QueueResponseIfAllowed(
        held_response.request,
        std::make_unique<BatchUserResponse>(held_response.response));
  }
  return 0;
}

int MessageManager::DropHeldResponse(uint64_t seq, const std::string& hash) {
  HeldResponseKey key(seq, hash);
  std::lock_guard<std::mutex> lk(held_response_mutex_);
  held_responses_.erase(key);
  released_before_hold_.erase(key);
  rollback_dropped_held_responses_.insert(key);
  return 0;
}

int MessageManager::DropHeldResponsesAfter(uint64_t checkpoint_seq) {
  // Rollback invalidates optimistic POE work after checkpoint_seq. Drop both
  // responses already held and early releases for responses not yet held.
  int dropped = 0;
  std::lock_guard<std::mutex> lk(held_response_mutex_);
  auto held_it = held_responses_.begin();
  while (held_it != held_responses_.end()) {
    if (held_it->first.first > checkpoint_seq) {
      rollback_dropped_held_responses_.insert(held_it->first);
      held_it = held_responses_.erase(held_it);
      dropped++;
    } else {
      ++held_it;
    }
  }
  auto released_it = released_before_hold_.begin();
  while (released_it != released_before_hold_.end()) {
    if (released_it->first > checkpoint_seq) {
      rollback_dropped_held_responses_.insert(*released_it);
      released_it = released_before_hold_.erase(released_it);
      dropped++;
    } else {
      ++released_it;
    }
  }
  return dropped;
}

uint64_t MessageManager::GetStableCheckpoint() const {
  return checkpoint_manager_ == nullptr
             ? 0
             : checkpoint_manager_->GetStableCheckpoint();
}

int MessageManager::RollbackToCheckpoint(uint64_t checkpoint_seq) {
  LOG(ERROR) << "message manager rollback to checkpoint:" << checkpoint_seq;
  DropHeldResponsesAfter(checkpoint_seq);

  // Restore application/executor state first, then align ordering metadata so
  // the next accepted transaction starts at checkpoint_seq + 1.
  int ret = transaction_executor_ == nullptr
                ? -2
                : transaction_executor_->RollbackToCheckpoint(checkpoint_seq);

  const uint64_t next_seq = checkpoint_seq + 1;
  SetNextSeq(next_seq);
  if (checkpoint_manager_ != nullptr) {
    SetHighestPreparedSeq(next_seq);
    checkpoint_manager_->SetLastCommit(checkpoint_seq);
  }
  if (collector_pool_ != nullptr) {
    collector_pool_->Reset(next_seq);
  }
  return ret;
}

void MessageManager::QueueResponseIfAllowed(
    const Request& request, std::unique_ptr<BatchUserResponse> response) {
  if (response == nullptr) {
    return;
  }
  if (transaction_executor_->NeedResponse() &&
      (!response_filter_ || response_filter_(request)) &&
      response->proxy_id() != 0) {
    queue_.Push(std::move(response));
  }
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
  // Apply the same filter to direct response generation, such as duplicate
  // request handling, so non-coordinator shards cannot reply through this path.
  QueueResponseIfAllowed(*request, std::move(response));
}

LockFreeCollectorPool* MessageManager::GetCollectorPool() {
  return collector_pool_.get();
}

}  // namespace resdb

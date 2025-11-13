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

#include "platform/consensus/ordering/raft/consensus_manager_raft.h"

#include <chrono>

#include <glog/logging.h>

#include "platform/proto/resdb.pb.h"

namespace resdb {

ConsensusManagerRaft::ConsensusManagerRaft(
    const ResDBConfig& config, std::unique_ptr<TransactionManager> executor,
    std::unique_ptr<CustomQuery> query_executor)
    : ConsensusManager(config),
      transaction_manager_(std::move(executor)),
      query_executor_(std::move(query_executor)) {
  // RAFT owns its heartbeat cadence via AppendEntries, so disable the default
  // heartbeat thread provided by ConsensusManager.
  config_.SetHeartBeatEnabled(false);

  Storage* storage =
      transaction_manager_ ? transaction_manager_->GetStorage() : nullptr;
  if (storage != nullptr) {
    raft_log_ = std::make_unique<RaftLog>(storage);
    auto status = raft_log_->LoadFromStorage();
    if (!status.ok()) {
      LOG(WARNING) << "Failed to load persisted RAFT log: " << status;
    }
    persistent_state_ = std::make_unique<RaftPersistentState>(storage);
    status = persistent_state_->Load();
    if (!status.ok()) {
      LOG(WARNING) << "Failed to load persisted RAFT state: " << status;
    }
    snapshot_manager_ = std::make_unique<RaftSnapshotManager>(storage);
  } else {
    LOG(WARNING) << "Transaction manager storage unavailable; RAFT state will "
                    "not be persisted.";
  }
  raft_rpc_ = std::make_unique<RaftRpc>(config_, GetBroadCastClient());
}

ConsensusManagerRaft::~ConsensusManagerRaft() { StopHeartbeatThread(); }

int ConsensusManagerRaft::ConsensusCommit(std::unique_ptr<Context> context,
                                          std::unique_ptr<Request> request) {
  switch (request->type()) {
    case Request::TYPE_CLIENT_REQUEST:
    case Request::TYPE_NEW_TXNS:
      return HandleClientRequest(std::move(context), std::move(request));
    case Request::TYPE_CUSTOM_QUERY:
      return HandleCustomQuery(std::move(context), std::move(request));
    case Request::TYPE_CUSTOM_CONSENSUS:
      return HandleCustomConsensus(std::move(context), std::move(request));
    default:
      LOG(WARNING) << "ConsensusManagerRaft received unsupported request type: "
                   << request->type();
      return -1;
  }
}

std::vector<ReplicaInfo> ConsensusManagerRaft::GetReplicas() {
  return config_.GetReplicaInfos();
}

uint32_t ConsensusManagerRaft::GetPrimary() { return leader_id_.load(); }

uint32_t ConsensusManagerRaft::GetVersion() {
  return static_cast<uint32_t>(current_term_.load());
}

void ConsensusManagerRaft::Start() {
  ConsensusManager::Start();
  StartHeartbeatThread();
}

void ConsensusManagerRaft::SetClientRequestHandler(RequestHandler handler) {
  client_request_handler_ = std::move(handler);
}

void ConsensusManagerRaft::SetCustomConsensusHandler(RequestHandler handler) {
  consensus_message_handler_ = std::move(handler);
}

void ConsensusManagerRaft::SetHeartbeatTask(HeartbeatTask task) {
  heartbeat_task_ = std::move(task);
}

void ConsensusManagerRaft::UpdateLeadership(uint32_t leader_id,
                                            uint64_t term) {
  leader_id_.store(leader_id);
  current_term_.store(term);
}

int ConsensusManagerRaft::HandleClientRequest(
    std::unique_ptr<Context> context, std::unique_ptr<Request> request) {
  if (client_request_handler_) {
    return client_request_handler_(std::move(context), std::move(request));
  }
  LOG(WARNING) << "Client request arrived but RAFT core handler is not set yet.";
  return -1;
}

int ConsensusManagerRaft::HandleCustomQuery(std::unique_ptr<Context> context,
                                            std::unique_ptr<Request> request) {
  if (query_executor_ == nullptr) {
    LOG(ERROR) << "Custom query executor not configured for RAFT.";
    return -1;
  }

  std::unique_ptr<std::string> resp_str =
      query_executor_->Query(request->data());

  CustomQueryResponse response;
  if (resp_str != nullptr) {
    response.set_resp_str(*resp_str);
  }

  if (context != nullptr && context->client != nullptr) {
    int ret = context->client->SendRawMessage(response);
    if (ret) {
      LOG(ERROR) << "Failed to send custom query response, ret=" << ret;
      return ret;
    }
  }
  return 0;
}

int ConsensusManagerRaft::HandleCustomConsensus(
    std::unique_ptr<Context> context, std::unique_ptr<Request> request) {
  if (!raft::IsRaftUserType(request->user_type())) {
    LOG(ERROR) << "Unknown custom consensus user_type="
               << request->user_type();
    return -1;
  }
  if (consensus_message_handler_) {
    return consensus_message_handler_(std::move(context), std::move(request));
  }
  LOG(WARNING) << "RAFt RPC received but handler is not registered.";
  return -1;
}

void ConsensusManagerRaft::StartHeartbeatThread() {
  bool expected = false;
  if (!heartbeat_running_.compare_exchange_strong(expected, true)) {
    return;
  }
  raft_heartbeat_thread_ =
      std::thread(&ConsensusManagerRaft::HeartbeatLoop, this);
}

void ConsensusManagerRaft::StopHeartbeatThread() {
  if (!heartbeat_running_.exchange(false)) {
    return;
  }
  heartbeat_cv_.notify_all();
  if (raft_heartbeat_thread_.joinable()) {
    raft_heartbeat_thread_.join();
  }
}

void ConsensusManagerRaft::HeartbeatLoop() {
  std::unique_lock<std::mutex> lk(heartbeat_mutex_);
  while (heartbeat_running_) {
    lk.unlock();
    if (heartbeat_task_) {
      heartbeat_task_();
    }
    lk.lock();
    heartbeat_cv_.wait_for(lk, std::chrono::milliseconds(500), [this]() {
      return !heartbeat_running_;
    });
  }
}

}  // namespace resdb

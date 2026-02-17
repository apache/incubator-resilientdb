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

#include "platform/consensus/ordering/common/framework/consensus.h"

#include <glog/logging.h>
#include <unistd.h>

#include "common/utils/utils.h"

namespace resdb {
namespace common {

Consensus::Consensus(const ResDBConfig& config,
                     std::unique_ptr<TransactionManager> executor)
    : ConsensusManager(config),
      replica_communicator_(GetBroadCastClient()),
      transaction_executor_(std::make_unique<TransactionExecutor>(
          config,
          [&](std::unique_ptr<Request> request,
              std::unique_ptr<BatchUserResponse> resp_msg) {
            ResponseMsg(*resp_msg);
          },
          nullptr, std::move(executor))) {
  LOG(INFO) << "is running is performance mode:"
            << config_.IsPerformanceRunning();
  is_stop_ = false;
  global_stats_ = Stats::GetGlobalStats();
}

void Consensus::Init() {
  if (performance_manager_ == nullptr) {
    performance_manager_ =
        config_.IsPerformanceRunning()
            ? std::make_unique<PerformanceManager>(
                  config_, GetBroadCastClient(), GetSignatureVerifier())
            : nullptr;
  }

  if (response_manager_ == nullptr) {
    response_manager_ =
        !config_.IsPerformanceRunning()
            ? std::make_unique<ResponseManager>(config_, GetBroadCastClient(),
                                                GetSignatureVerifier())
            : nullptr;
  }
}

void Consensus::InitProtocol(ProtocolBase* protocol) {
  protocol->SetSingleCallFunc(
      [&](int type, const google::protobuf::Message& msg, int node_id) {
        return SendMsg(type, msg, node_id);
      });

  protocol->SetBroadcastCallFunc(
      [&](int type, const google::protobuf::Message& msg) {
        return Broadcast(type, msg);
      });

  protocol->SetCommitFunc(
      [&](const google::protobuf::Message& msg) { return CommitMsg(msg); });
}

Consensus::~Consensus() { is_stop_ = true; }

void Consensus::SetPerformanceManager(
    std::unique_ptr<PerformanceManager> performance_manager) {
  performance_manager_ = std::move(performance_manager);
}

bool Consensus::IsStop() { return is_stop_; }

void Consensus::SetupPerformanceDataFunc(std::function<std::string()> func) {
  performance_manager_->SetDataFunc(func);
}

void Consensus::SetCommunicator(ReplicaCommunicator* replica_communicator) {
  replica_communicator_ = replica_communicator;
}

int Consensus::Broadcast(int type, const google::protobuf::Message& msg) {
  Request request;
  msg.SerializeToString(request.mutable_data());
  request.set_type(Request::TYPE_CUSTOM_CONSENSUS);
  request.set_user_type(type);
  request.set_sender_id(config_.GetSelfInfo().id());

  replica_communicator_->BroadCast(request);
  return 0;
}

int Consensus::SendMsg(int type, const google::protobuf::Message& msg,
                       int node_id) {
  Request request;
  msg.SerializeToString(request.mutable_data());
  request.set_type(Request::TYPE_CUSTOM_CONSENSUS);
  request.set_user_type(type);
  request.set_sender_id(config_.GetSelfInfo().id());
  replica_communicator_->SendMessage(request, node_id);
  return 0;
}

std::vector<ReplicaInfo> Consensus::GetReplicas() {
  return config_.GetReplicaInfos();
}

int Consensus::CommitMsg(const google::protobuf::Message& txn) { return 0; }

// The implementation of PBFT.
int Consensus::ConsensusCommit(std::unique_ptr<Context> context,
                               std::unique_ptr<Request> request) {
  switch (request->type()) {
    case Request::TYPE_CLIENT_REQUEST:
      if (config_.IsPerformanceRunning()) {
        return performance_manager_->StartEval();
      }
    case Request::TYPE_RESPONSE:
      if (config_.IsPerformanceRunning()) {
        return performance_manager_->ProcessResponseMsg(std::move(context),
                                                        std::move(request));
      }
    case Request::TYPE_NEW_TXNS: {
      return ProcessNewTransaction(std::move(request));
    }
    case Request::TYPE_CUSTOM_CONSENSUS: {
      return ProcessCustomConsensus(std::move(request));
    }
  }
  return 0;
}

int Consensus::ProcessCustomConsensus(std::unique_ptr<Request> request) {
  return 0;
}

int Consensus::ProcessNewTransaction(std::unique_ptr<Request> request) {
  return 0;
}

int Consensus::ResponseMsg(const BatchUserResponse& batch_resp) {
  Request request;
  request.set_seq(batch_resp.seq());
  request.set_type(Request::TYPE_RESPONSE);
  request.set_sender_id(config_.GetSelfInfo().id());
  request.set_proxy_id(batch_resp.proxy_id());
  batch_resp.SerializeToString(request.mutable_data());
  replica_communicator_->SendMessage(request, request.proxy_id());
  return 0;
}

}  // namespace common
}  // namespace resdb

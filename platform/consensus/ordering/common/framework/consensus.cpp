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

#include "platform/consensus/ordering/common/framework/consensus.h"

#include <glog/logging.h>
#include <unistd.h>

#include "common/utils/utils.h"

namespace resdb {
namespace common {

Consensus::Consensus(const ResDBConfig& config,
                     std::unique_ptr<TransactionManager> executor,
                     std::unique_ptr<PerformanceManager> performance_manager,
                     std::unique_ptr<ResponseManager> response_manager)
    : ConsensusManager(config),
      replica_communicator_(GetBroadCastClient()),
      performance_manager_(std::move(performance_manager)),
      response_manager_(std::move(response_manager)),
      transaction_executor_(std::make_unique<TransactionExecutor>(
          config,
          [&](std::unique_ptr<Request> request,
              std::unique_ptr<BatchUserResponse> resp_msg) {
            ResponseMsg(*resp_msg);
          },
          nullptr, std::move(executor))) {
  LOG(INFO) << "is running is performance mode:"
            << config_.IsPerformanceRunning();
  global_stats_ = Stats::GetGlobalStats();

  if(performance_manager_ == nullptr) {
    performance_manager_ = 
        config_.IsPerformanceRunning()
        ? std::make_unique<PerformanceManager>(
          config_, GetBroadCastClient(), GetSignatureVerifier())
        : nullptr;
  }
  if(response_manager_ == nullptr ) {
    response_manager_ = 
          !config_.IsPerformanceRunning()
              ? std::make_unique<ResponseManager>(config_, GetBroadCastClient(),
                                                  GetSignatureVerifier())
              : nullptr;
  }
}

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

int Consensus::CommitMsg(const google::protobuf::Message &txn) {
  return 0;
}

// The implementation of PBFT.
int Consensus::ConsensusCommit(std::unique_ptr<Context> context,
                               std::unique_ptr<Request> request) {
   //LOG(ERROR)<<"receive commit:"<<request->type()<<" "<<Request_Type_Name(request->type());
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
  //LOG(ERROR)<<"send response:"<<batch_resp.proxy_id();
  if (batch_resp.proxy_id() == 0) {
    return 0;
  }
  //LOG(ERROR)<<"send response:"<<batch_resp.proxy_id();
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

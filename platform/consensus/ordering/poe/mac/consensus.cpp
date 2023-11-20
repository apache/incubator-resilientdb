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

#include "platform/consensus/ordering/poe/mac/consensus.h"

#include <glog/logging.h>
#include <unistd.h>

#include "common/crypto/signature_verifier.h"

namespace resdb {
namespace poe {

namespace {

std::unique_ptr<POERequest> Decode(const Request& request) {
  auto hot_stuff_request = std::make_unique<POERequest>();
  hot_stuff_request->ParseFromString(request.data());
  return hot_stuff_request;
}

template <typename Type>
std::unique_ptr<Type> DecodeMsg(const Request& request) {
  auto dec_request = std::make_unique<Type>();
  dec_request->ParseFromString(request.data());
  return dec_request;
}

}  // namespace

Consensus::Consensus(const ResDBConfig& config,
                     std::unique_ptr<TransactionManager> executor)
    : ConsensusManager(config),
      system_info_(config),
      message_manager_(std::make_unique<MessageManager>(
          config, std::move(executor), &system_info_)),
      commitment_(std::make_unique<Commitment>(
          config_, message_manager_.get(), GetBroadCastClient(),
          GetSignatureVerifier(),
          [&](int user_type, const ::google::protobuf::Message& msg,
              int proxy_id) { return SendMessage(user_type, msg, proxy_id); })),
      response_manager_(config_.IsPerformanceRunning()
                            ? nullptr
                            : std::make_unique<common::ResponseManager>(
                                  config_, GetBroadCastClient(), &system_info_,
                                  GetSignatureVerifier())),
      performance_manager_(config_.IsPerformanceRunning()
                               ? std::make_unique<common::PerformanceManager>(
                                     config_, GetBroadCastClient(),
                                     &system_info_, GetSignatureVerifier())
                               : nullptr),
       communicator_(GetBroadCastClient()){
  global_stats_ = Stats::GetGlobalStats();
}

void Consensus::Start() { ConsensusManager::Start(); }

std::vector<ReplicaInfo> Consensus::GetReplicas() {
  return config_.GetReplicaInfos();
}

int Consensus::ConsensusCommit(std::unique_ptr<Context> context,
                               std::unique_ptr<Request> request) {
   LOG(ERROR) << "get request:" << Request::Type_Name(request->type())
           << " from:" << request->sender_id()<<" seq:"<<request->seq();
  switch (request->type()) {
    case Request::TYPE_CLIENT_REQUEST:
      if (config_.IsPerformanceRunning()) {
        return performance_manager_->StartEval();
      }
      return response_manager_->NewUserRequest(std::move(context),
                                               std::move(request));
    case Request::TYPE_RESPONSE:
      if (config_.IsPerformanceRunning()) {
        return performance_manager_->ProcessResponseMsg(std::move(context),
                                                        std::move(request));
      }
      return response_manager_->ProcessResponseMsg(std::move(context),
                                                   std::move(request));
    case Request::TYPE_CUSTOM_TYPE:
      return commitment_->Process(request->user_type(), Decode(*request));

    case Request::TYPE_NEW_TXNS:
      return commitment_->ProcessNewTransaction(std::move(request));
    default:
      return commitment_->Process(request->user_type(), Decode(*request));
  }
  return 0;
}

void Consensus::SetupPerformanceDataFunc(std::function<std::string()> func) {
  performance_manager_->SetDataFunc(func);
}

int Consensus::SendMessage(int user_type,
                           const ::google::protobuf::Message& msg,
                           int proxy_id) {
  if (user_type == 0) {
    communicator_->SendMessage(msg, proxy_id);
    return 0;
  } else {
    Request request;
    request.set_user_type(user_type);
    request.set_type(Request::TYPE_CUSTOM_TYPE);
    request.set_sender_id(config_.GetSelfInfo().id());
    msg.SerializeToString(request.mutable_data());
    if (proxy_id == -1) {
      LOG(ERROR)<<"send msg type:"<<request.type()<<" communicator:"<<(communicator_==nullptr);
      communicator_->BroadCast(request);
    } else {
      LOG(ERROR)<<"send msg type:"<<request.type()<<" proxy id:"<<proxy_id;
      communicator_->SendMessage(request, proxy_id);
    }
  }
  return 0;
}

}  // namespace poe
}  // namespace resdb

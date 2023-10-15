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

#include "platform/consensus/ordering/poe/consensus_manager_poe.h"

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

ConsensusManagerPOE::ConsensusManagerPOE(
    const ResDBConfig& config, std::unique_ptr<TransactionManager> executor)
    : ConsensusManager(config),
      system_info_(config),
      message_manager_(std::make_unique<MessageManager>(
          config, std::move(executor), &system_info_)),
      commitment_(std::make_unique<Commitment>(config_, message_manager_.get(),
                                               GetBroadCastClient(),
                                               GetSignatureVerifier())),
      response_manager_(
          config_.IsPerformanceRunning()
              ? nullptr
              : std::make_unique<common::ResponseManager>(
                    config_, GetBroadCastClient(),
                    std::make_unique<common::RequestSerializer<POERequest>>(
                        GetSignatureVerifier(), config_.GetSelfInfo().id(),
                        POERequest::TYPE_NEWREQUEST))),
      performance_manager_(
          config_.IsPerformanceRunning()
              ? std::make_unique<common::PerformanceManager>(
                    config_, GetBroadCastClient(), &system_info_,
                    std::make_unique<common::RequestSerializer<POERequest>>(
                        GetSignatureVerifier(), config_.GetSelfInfo().id(),
                        POERequest::TYPE_NEWREQUEST))
              : nullptr),
      viewchange_manager_(std::make_unique<ViewChange>(
          config_, message_manager_.get(), commitment_.get(), &system_info_,
          GetBroadCastClient(), GetSignatureVerifier())) {}

void ConsensusManagerPOE::Start() {
  ConsensusManager::Start();
  commitment_->Init();
}

std::vector<ReplicaInfo> ConsensusManagerPOE::GetReplicas() {
  return config_.GetReplicaInfos();
}

int ConsensusManagerPOE::ConsensusCommit(std::unique_ptr<Context> context,
                                         std::unique_ptr<Request> request) {
  // LOG(ERROR) << "get request:" << Request::Type_Name(request->type())
  //           << " from:" << request->sender_id();
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
    case Request::TYPE_VIEWCHANGE:
      return viewchange_manager_->ProcessViewChange(
          DecodeMsg<ViewChangeMsg>(*request));
    case Request::TYPE_NEWVIEW:
      return viewchange_manager_->ProcessNewView(
          DecodeMsg<NewViewChangeMsg>(*request));
    default:
      return commitment_->Process(Decode(*request));
  }
  return 0;
}

void ConsensusManagerPOE::SetupPerformanceDataFunc(
    std::function<std::string()> func) {
  performance_manager_->SetDataFunc(func);
}

}  // namespace poe
}  // namespace resdb

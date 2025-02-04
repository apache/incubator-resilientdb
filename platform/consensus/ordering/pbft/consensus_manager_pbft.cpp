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

#include "platform/consensus/ordering/pbft/consensus_manager_pbft.h"

#include <glog/logging.h>
#include <unistd.h>

#include "common/crypto/signature_verifier.h"

namespace resdb {

ConsensusManagerPBFT::ConsensusManagerPBFT(
    const ResDBConfig& config, std::unique_ptr<TransactionManager> executor,
    std::unique_ptr<CustomQuery> query_executor)
    : ConsensusManager(config),
      system_info_(std::make_unique<SystemInfo>(config)),
      checkpoint_manager_(std::make_unique<CheckPointManager>(
          config, GetBroadCastClient(), GetSignatureVerifier())),
      message_manager_(std::make_unique<MessageManager>(
          config, std::move(executor), checkpoint_manager_.get(),
          system_info_.get())),
      commitment_(std::make_unique<Commitment>(config_, message_manager_.get(),
                                               GetBroadCastClient(),
                                               GetSignatureVerifier())),
      query_(std::make_unique<Query>(config_, message_manager_.get(),
                                     std::move(query_executor))),
      response_manager_(config_.IsPerformanceRunning()
                            ? nullptr
                            : std::make_unique<ResponseManager>(
                                  config_, GetBroadCastClient(),
                                  system_info_.get(), GetSignatureVerifier())),
      performance_manager_(config_.IsPerformanceRunning()
                               ? std::make_unique<PerformanceManager>(
                                     config_, GetBroadCastClient(),
                                     system_info_.get(), GetSignatureVerifier())
                               : nullptr),
      view_change_manager_(std::make_unique<ViewChangeManager>(
          config_, checkpoint_manager_.get(), message_manager_.get(),
          system_info_.get(), GetBroadCastClient(), GetSignatureVerifier())),
      recovery_(std::make_unique<Recovery>(config_, checkpoint_manager_.get(),
                                           system_info_.get(),
                                           message_manager_->GetStorage())) {
  LOG(INFO) << "is running is performance mode:"
            << config_.IsPerformanceRunning();
  global_stats_ = Stats::GetGlobalStats();

  recovery_->ReadLogs(
      [&](const SystemInfoData& data) {
        system_info_->SetCurrentView(data.view());
        system_info_->SetPrimary(data.primary_id());
      },
      [&](std::unique_ptr<Context> context, std::unique_ptr<Request> request) {
        return InternalConsensusCommit(std::move(context), std::move(request));
      });
}

void ConsensusManagerPBFT::SetNeedCommitQC(bool need_qc) {
  commitment_->SetNeedCommitQC(need_qc);
}

void ConsensusManagerPBFT::Start() { ConsensusManager::Start(); }

std::vector<ReplicaInfo> ConsensusManagerPBFT::GetReplicas() {
  return message_manager_->GetReplicas();
}

uint32_t ConsensusManagerPBFT::GetPrimary() {
  return system_info_->GetPrimaryId();
}

uint32_t ConsensusManagerPBFT::GetVersion() {
  return system_info_->GetCurrentView();
}

void ConsensusManagerPBFT::SetPrimary(uint32_t primary, uint64_t version) {
  if (version > system_info_->GetCurrentView()) {
    system_info_->SetCurrentView(version);
    system_info_->SetPrimary(primary);
  }
}

void ConsensusManagerPBFT::AddPendingRequest(std::unique_ptr<Context> context,
                                             std::unique_ptr<Request> request) {
  std::lock_guard<std::mutex> lk(mutex_);
  request_pending_.push(std::make_pair(std::move(context), std::move(request)));
}

absl::StatusOr<std::pair<std::unique_ptr<Context>, std::unique_ptr<Request>>>
ConsensusManagerPBFT::PopPendingRequest() {
  std::lock_guard<std::mutex> lk(mutex_);
  if (request_pending_.empty()) {
    LOG(ERROR) << "empty:";
    return absl::InternalError("No Data.");
  }
  auto new_request = std::move(request_pending_.front());
  request_pending_.pop();
  return new_request;
}

// The implementation of PBFT.
int ConsensusManagerPBFT::ConsensusCommit(std::unique_ptr<Context> context,
                                          std::unique_ptr<Request> request) {
  // LOG(INFO) << "recv impl type:" << request->type() << " "
  //           << "sender id:" << request->sender_id();
  // If it is in viewchange, push the request to the queue
  // for the requests from the new view which come before
  // the local new view done.
  recovery_->AddRequest(context.get(), request.get());
  if (config_.GetConfigData().enable_viewchange()) {
    view_change_manager_->MayStart();
    if (view_change_manager_->IsInViewChange()) {
      switch (request->type()) {
        case Request::TYPE_NEW_TXNS:
        case Request::TYPE_PRE_PREPARE:
        case Request::TYPE_PREPARE:
        case Request::TYPE_COMMIT:
          AddPendingRequest(std::move(context), std::move(request));
          return 0;
      }
    } else {
      while (true) {
        auto new_request = PopPendingRequest();
        if (!new_request.ok()) {
          break;
        }
        InternalConsensusCommit(std::move((*new_request).first),
                                std::move((*new_request).second));
      }
    }
  }
  return InternalConsensusCommit(std::move(context), std::move(request));
}

int ConsensusManagerPBFT::InternalConsensusCommit(
    std::unique_ptr<Context> context, std::unique_ptr<Request> request) {
  // LOG(INFO) << "recv impl type:" << request->type() << " "
  //          << "sender id:" << request->sender_id();

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
    case Request::TYPE_NEW_TXNS:
      return commitment_->ProcessNewRequest(std::move(context),
                                            std::move(request));
    case Request::TYPE_PRE_PREPARE:
      return commitment_->ProcessProposeMsg(std::move(context),
                                            std::move(request));
    case Request::TYPE_PREPARE:
      return commitment_->ProcessPrepareMsg(std::move(context),
                                            std::move(request));
    case Request::TYPE_COMMIT:
      return commitment_->ProcessCommitMsg(std::move(context),
                                           std::move(request));
    case Request::TYPE_CHECKPOINT:
      return checkpoint_manager_->ProcessCheckPoint(std::move(context),
                                                    std::move(request));
    case Request::TYPE_VIEWCHANGE:
      return view_change_manager_->ProcessViewChange(std::move(context),
                                                     std::move(request));
    case Request::TYPE_NEWVIEW:
      return view_change_manager_->ProcessNewView(std::move(context),
                                                  std::move(request));
    case Request::TYPE_QUERY:
      return query_->ProcessQuery(std::move(context), std::move(request));
    case Request::TYPE_REPLICA_STATE:
      return query_->ProcessGetReplicaState(std::move(context),
                                            std::move(request));
    case Request::TYPE_CUSTOM_QUERY:
      return query_->ProcessCustomQuery(std::move(context), std::move(request));
  }
  return 0;
}

void ConsensusManagerPBFT::SetupPerformanceDataFunc(
    std::function<std::string()> func) {
  performance_manager_->SetDataFunc(func);
}

void ConsensusManagerPBFT::SetPreVerifyFunc(
    std::function<bool(const Request&)> func) {
  commitment_->SetPreVerifyFunc(func);
}

}  // namespace resdb

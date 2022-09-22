#include "ordering/pbft/consensus_service_pbft.h"

#include <unistd.h>

#include "crypto/signature_verifier.h"
#include "glog/logging.h"

namespace resdb {

ConsensusServicePBFT::ConsensusServicePBFT(
    const ResDBConfig& config,
    std::unique_ptr<TransactionExecutorImpl> executor)
    : ConsensusService(config),
      system_info_(std::make_unique<SystemInfo>(config)),
      checkpoint_manager_(std::make_unique<CheckPointManager>(
          config, GetBroadCastClient(), GetSignatureVerifier())),
      transaction_manager_(std::make_unique<TransactionManager>(
          config, std::move(executor), checkpoint_manager_.get(),
          system_info_.get())),
      commitment_(std::make_unique<Commitment>(
          config_, transaction_manager_.get(), GetBroadCastClient(),
          GetSignatureVerifier())),
      query_(std::make_unique<Query>(config_, transaction_manager_.get())),
      response_manager_(config_.IsPerformanceRunning()
                            ? nullptr
                            : std::make_unique<ResponseManager>(
                                  config_, GetBroadCastClient(),
                                  system_info_.get(), GetSignatureVerifier())),
      performance_manager_(
          config_.IsPerformanceRunning()
              ? std::make_unique<PerformanceManager>(
                    config_, GetBroadCastClient(), GetSignatureVerifier())
              : nullptr),
      view_change_manager_(std::make_unique<ViewChangeManager>(
          config_, checkpoint_manager_.get(), transaction_manager_.get(),
          system_info_.get(), GetBroadCastClient(), GetSignatureVerifier())) {
  LOG(INFO) << "is running is performance mode:"
            << config_.IsPerformanceRunning();
  global_stats_ = Stats::GetGlobalStats();
}

void ConsensusServicePBFT::Start() { ConsensusService::Start(); }

std::vector<ReplicaInfo> ConsensusServicePBFT::GetReplicas() {
  return transaction_manager_->GetReplicas();
}

uint32_t ConsensusServicePBFT::GetPrimary() {
  return system_info_->GetPrimaryId();
}

uint32_t ConsensusServicePBFT::GetVersion() {
  return system_info_->GetCurrentView();
}

void ConsensusServicePBFT::SetPrimary(uint32_t primary, uint64_t version) {
  if (version > system_info_->GetCurrentView()) {
    system_info_->SetCurrentView(version);
    system_info_->SetPrimary(primary);
  }
}

void ConsensusServicePBFT::AddPendingRequest(std::unique_ptr<Context> context,
                                             std::unique_ptr<Request> request) {
  std::lock_guard<std::mutex> lk(mutex_);
  request_pending_.push(std::make_pair(std::move(context), std::move(request)));
}

absl::StatusOr<std::pair<std::unique_ptr<Context>, std::unique_ptr<Request>>>
ConsensusServicePBFT::PopPendingRequest() {
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
int ConsensusServicePBFT::ConsensusCommit(std::unique_ptr<Context> context,
                                          std::unique_ptr<Request> request) {
  // LOG(INFO) << "recv impl type:" << request->type() << " "
  //           << "sender id:" << request->sender_id();
  // If it is in viewchange, push the request to the queue
  // for the requests from the new view which come before
  // the local new view done.
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

int ConsensusServicePBFT::InternalConsensusCommit(
    std::unique_ptr<Context> context, std::unique_ptr<Request> request) {
  // LOG(INFO) << "recv impl type:" << request->type() << " "
  //          << "sender id:" << request->sender_id();
  switch (request->type()) {
    case Request::TYPE_CLIENT_REQUEST:
      if (config_.IsPerformanceRunning()) {
        return performance_manager_->StartEval();
      }
      return response_manager_->NewClientRequest(std::move(context),
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
  }
  return 0;
}

void ConsensusServicePBFT::SetupPerformanceDataFunc(
    std::function<std::string()> func) {
  performance_manager_->SetDataFunc(func);
}

}  // namespace resdb

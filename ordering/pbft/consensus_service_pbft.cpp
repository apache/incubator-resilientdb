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
      checkpoint_(std::make_unique<CheckPoint>(config, GetBroadCastClient())),
      transaction_manager_(std::make_unique<TransactionManager>(
          config, std::move(executor), checkpoint_->GetCheckPointInfo(),
          system_info_.get())),
      commitment_(std::make_unique<Commitment>(
          config_, transaction_manager_.get(), GetBroadCastClient(),
          GetSignatureVerifier())),
      recovery_(std::make_unique<Recovery>(config_, transaction_manager_.get(),
                                           GetBroadCastClient(),
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
              : nullptr) {
  LOG(INFO) << "is running is performance mode:"
            << config_.IsPerformanceRunning();
  global_stats_ = Stats::GetGlobalStats();
}

void ConsensusServicePBFT::Start() { ConsensusService::Start(); }

std::vector<ReplicaInfo> ConsensusServicePBFT::GetReplicas() {
  return transaction_manager_->GetReplicas();
}

// The implementation of PBFT.
int ConsensusServicePBFT::ConsensusCommit(std::unique_ptr<Context> context,
                                          std::unique_ptr<Request> request) {
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
    case Request::TYPE_RECOVERY_DATA:
      return recovery_->ProcessRecoveryData(std::move(context),
                                            std::move(request));
    case Request::TYPE_RECOVERY_DATA_RESP:
      return recovery_->ProcessRecoveryDataResp(std::move(context),
                                                std::move(request));
    case Request::TYPE_CHECKPOINT:
      return checkpoint_->ProcessCheckPoint(std::move(context),
                                            std::move(request));
    case Request::TYPE_QUERY:
      return query_->ProcessQuery(std::move(context), std::move(request));
    case Request::TYPE_REPLICA_STATE:
      return query_->ProcessGetReplicaState(std::move(context),
                                            std::move(request));
  }
  return 0;
}

void ConsensusServicePBFT::AddNewReplica(const ReplicaInfo& info) {
  recovery_->AddNewReplica(info);
}

void ConsensusServicePBFT::SetupPerformanceDataFunc(
    std::function<std::string()> func) {
  performance_manager_->SetDataFunc(func);
}

}  // namespace resdb

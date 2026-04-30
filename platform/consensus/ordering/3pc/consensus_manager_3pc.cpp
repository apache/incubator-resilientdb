#include "platform/consensus/ordering/3pc/consensus_manager_3pc.h"

#include <utility>

namespace resdb {

ConsensusManager3PC::ConsensusManager3PC(
    const ResDBConfig& config, std::unique_ptr<TransactionManager> executor,
    std::unique_ptr<CustomQuery> query_executor)
    : ConsensusManagerPBFT(config, std::move(executor),
                           /*defer_recovery_init=*/true,
                           /*defer_commitment_init=*/true,
                           std::move(query_executor)) {
  // Replace the PBFT commitment module with the 3PC one, but keep the rest of
  // the scaffolding from ConsensusManagerPBFT intact.
  commitment_ = std::make_unique<Commitment3PC>(
      config_, message_manager_.get(), GetBroadCastClient(),
      GetSignatureVerifier());

  // Register the 3PC duplicate manager with the reused view-change scaffolding.
  if (view_change_manager_ != nullptr) {
    view_change_manager_->SetDuplicateManager(commitment_->GetDuplicateManager());
  }

  // IMPORTANT:
  // The stock PBFT PerformanceManager generates PBFT-specific traffic.
  // For 3PC implementation, this will not work and a new PerformanceManager3PC
  // would have to be created. To avoid this, performance can be measured in
  // normal kv mode through running a custom script that generates requests and
  // calculates throughput and latency.
  if (config_.IsPerformanceRunning()) {
    LOG(WARNING) << "3PC running with PBFT performance mode enabled. "
                 << "PBFT PerformanceManager is not protocol-compatible with 3PC. "
                 << "Use normal request path or implement PerformanceManager3PC.";
  }

  InitRecovery3PC();
}

void ConsensusManager3PC::InitRecovery3PC() {
  // Mirror PBFT recovery process, but with 3PC-specific commit path.
  recovery_->ReadLogs(
      [&](const SystemInfoData& data) {
        LOG(ERROR) << " read data info:" << data.view()
                   << " primary/coordinator:" << data.primary_id();
        system_info_->SetCurrentView(data.view());
        system_info_->SetPrimary(data.primary_id());
      },
      [&](std::unique_ptr<Context> context, std::unique_ptr<Request> request) {
        return InternalConsensusCommit3PC(std::move(context),
                                          std::move(request));
      },
      [&](int seq) { message_manager_->SetNextCommitSeq(seq + 1); });
  LOG(ERROR) << "3PC recovery is done";
}

int ConsensusManager3PC::ConsensusCommit(std::unique_ptr<Context> context,
                                         std::unique_ptr<Request> request) {
  LOG(INFO) << "recv 3PC type:" << request->type()
            << " sender id:" << request->sender_id()
            << " primary/coordinator:" << system_info_->GetPrimaryId();

  // Keep the PBFT scaffolding for pending requests during view-change-like
  // periods, but queue 3PC message types instead of PBFT ordering messages.
  if (view_change_manager_ && view_change_manager_->IsInViewChange()) {
    switch (request->type()) {
      case Request::TYPE_NEW_TXNS:
      case Request::TYPE_3PC_PREPARE:
      case Request::TYPE_3PC_VOTE_COMMIT:
      case Request::TYPE_3PC_VOTE_ABORT:
      case Request::TYPE_3PC_PRECOMMIT:
      case Request::TYPE_3PC_PRECOMMIT_ACK:
      case Request::TYPE_3PC_GLOBAL_COMMIT:
      case Request::TYPE_3PC_GLOBAL_ABORT:
        AddPendingRequest(std::move(context), std::move(request));
        return 0;
      default:
        break;
    }
  } else {
    while (true) {
      auto new_request = PopPendingRequest();
      if (!new_request.ok()) {
        break;
      }
      InternalConsensusCommit3PC(std::move((*new_request).first),
                                 std::move((*new_request).second));
    }
  }

  return InternalConsensusCommit3PC(std::move(context), std::move(request));
}

int ConsensusManager3PC::InternalConsensusCommit3PC(
    std::unique_ptr<Context> context, std::unique_ptr<Request> request) {
  LOG(ERROR) << "recv 3PC impl type:" << request->type()
             << " sender id:" << request->sender_id()
             << " seq:" << request->seq()
             << " coordinator:" << system_info_->GetPrimaryId()
             << " is recovery:" << request->is_recovery();

  switch (request->type()) {
    // Mirror the PBFT client ingress path.
    case Request::TYPE_CLIENT_REQUEST:
      if (config_.IsPerformanceRunning()) {
        // Perfromance path is unaivailable for 3PC unless we implement a
        // 3PC-specific performance manager.
        if (performance_manager_ != nullptr) { return performance_manager_->StartEval(); } if (response_manager_ != nullptr) {
          return response_manager_->NewUserRequest(std::move(context),
                                                  std::move(request));
        }
        LOG(ERROR) << "3PC performance mode requested, but no 3PC performance "
                      "manager is installed.";
        return -2;
      }
      return response_manager_->NewUserRequest(std::move(context),
                                              std::move(request));

    // Keep the existing reply path.
    case Request::TYPE_RESPONSE:
      if (config_.IsPerformanceRunning() && performance_manager_ != nullptr) { return performance_manager_->ProcessResponseMsg(std::move(context), std::move(request)); } if (false) {
        // If we later add PerformanceManager3PC, this branch can be reenabled
        // with the new performance manager. 
        return performance_manager_->ProcessResponseMsg(std::move(context),
                                                        std::move(request));
      }
      return response_manager_->ProcessResponseMsg(std::move(context),
                                                   std::move(request));

    // Entry point for 3PC -> proxy/client batches become a 3PC PREPARE.
    case Request::TYPE_NEW_TXNS: {
      uint64_t proxy_id = request->proxy_id();
      std::string hash = request->hash();

      int ret = commitment_->ProcessNewRequest(std::move(context),
                                               std::move(request));

      // Mirror the PBFT complaint-queue behavior for forwarded/non-coordinator
      // requests, since the proxy/coordinator flow is unchanged in 3PC.
      if (ret == -3) {
        std::pair<std::unique_ptr<Context>, std::unique_ptr<Request>>
            request_complained;
        {
          std::lock_guard<std::mutex> lk(commitment_->rc_mutex_);
          request_complained =
              std::move(commitment_->request_complained_.front());
          commitment_->request_complained_.pop();
        }
        AddComplainedRequest(std::move(request_complained.first),
                             std::move(request_complained.second));

        if (view_change_manager_ != nullptr) {
          view_change_manager_->AddComplaintTimer(proxy_id, hash);
        }
      }
      return ret;
    }

    // -------- 3PC protocol messages --------

    // Participant receives PREPARE from coordinator.
    case Request::TYPE_3PC_PREPARE:
      return GetCommitment3PC()->ProcessPrepareMsg(std::move(context),
                                                   std::move(request));

    // Coordinator receives a participant's VOTE_COMMIT.
    case Request::TYPE_3PC_VOTE_COMMIT:
      return GetCommitment3PC()->ProcessVoteCommitMsg(std::move(context),
                                                      std::move(request));

    // Coordinator receives a participant's VOTE_ABORT.
    case Request::TYPE_3PC_VOTE_ABORT:
      return GetCommitment3PC()->ProcessVoteAbortMsg(std::move(context),
                                                     std::move(request));

    // Participant receives PRECOMMIT from coordinator.
    case Request::TYPE_3PC_PRECOMMIT:
      return GetCommitment3PC()->ProcessPreCommitMsg(std::move(context),
                                                     std::move(request));

    // Coordinator receives PRECOMMIT_ACK from participant.
    case Request::TYPE_3PC_PRECOMMIT_ACK:
      return GetCommitment3PC()->ProcessPreCommitAckMsg(std::move(context),
                                                        std::move(request));

    // Participant receives final commit decision.
    case Request::TYPE_3PC_GLOBAL_COMMIT:
      return GetCommitment3PC()->ProcessGlobalCommitMsg(std::move(context),
                                                        std::move(request));

    // Participant receives final abort decision.
    case Request::TYPE_3PC_GLOBAL_ABORT:
      return GetCommitment3PC()->ProcessGlobalAbortMsg(std::move(context),
                                                       std::move(request));

    // -------- Keep non-consensus functionality intact --------

    case Request::TYPE_QUERY:
      return query_->ProcessQuery(std::move(context), std::move(request));

    case Request::TYPE_REPLICA_STATE:
      return query_->ProcessGetReplicaState(std::move(context),
                                            std::move(request));

    case Request::TYPE_CUSTOM_QUERY:
      return query_->ProcessCustomQuery(std::move(context),
                                        std::move(request));

    case Request::TYPE_RECOVERY_DATA:
      return ProcessRecoveryData(std::move(context), std::move(request));

    case Request::TYPE_RECOVERY_DATA_RESP:
      return ProcessRecoveryDataResponse(std::move(context),
                                         std::move(request));

    // PBFT-only message types are ingored for the 3PC protocol.
    case Request::TYPE_PRE_PREPARE:
    case Request::TYPE_PREPARE:
    case Request::TYPE_COMMIT:
    case Request::TYPE_CHECKPOINT:
    case Request::TYPE_STATUS_SYNC:
    case Request::TYPE_VIEWCHANGE:
    case Request::TYPE_NEWVIEW:
      LOG(WARNING) << "Ignoring PBFT-specific request type under 3PC: "
                   << request->type();
      return 0;

    default:
      LOG(WARNING) << "Unknown request type under 3PC: " << request->type();
      return 0;
  }
}

}  // namespace resdb

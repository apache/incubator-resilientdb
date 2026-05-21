#include "platform/consensus/ordering/sharded_3pc/consensus_manager_sharded_3pc_poe.h"

#include <glog/logging.h>

#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "platform/consensus/ordering/sharded_3pc/shard_3pc_commitment.h"
#include "platform/consensus/ordering/sharded_3pc/sharded_response_manager.h"

namespace resdb {
namespace {

// Global 3PC is unchanged from the sharded 3PC/PBFT path. These are the
// message types owned by the leader-to-leader global ordering layer.
bool NeedsGlobal3PCCommitment(int request_type) {
  switch (request_type) {
    case Request::TYPE_NEW_TXNS:
    case Request::TYPE_3PC_PREPARE:
    case Request::TYPE_3PC_VOTE_COMMIT:
    case Request::TYPE_3PC_VOTE_ABORT:
    case Request::TYPE_3PC_PRECOMMIT:
    case Request::TYPE_3PC_PRECOMMIT_ACK:
    case Request::TYPE_3PC_GLOBAL_COMMIT:
    case Request::TYPE_3PC_GLOBAL_ABORT:
      return true;
    default:
      return false;
  }
}

// Local POE is intentionally separate from the local PBFT message types. The
// POE manager should not dispatch TYPE_PRE_PREPARE/PREPARE/COMMIT.
bool NeedsLocalPOECommitment(int request_type) {
  switch (request_type) {
    case Request::TYPE_POE_EXECUTE:
    case Request::TYPE_POE_PROOF:
    case Request::TYPE_POE_CERT:
    case Request::TYPE_POE_ROLLBACK:
      return true;
    default:
      return false;
  }
}

// Small trace names keep the sharded POE logs readable when debugging the path
// from global 3PC into local POE.
const char* TraceRequestTypeName(int request_type) {
  switch (request_type) {
    case Request::TYPE_NEW_TXNS:
      return "TYPE_NEW_TXNS";
    case Request::TYPE_3PC_PREPARE:
      return "TYPE_3PC_PREPARE";
    case Request::TYPE_3PC_VOTE_COMMIT:
      return "TYPE_3PC_VOTE_COMMIT";
    case Request::TYPE_3PC_VOTE_ABORT:
      return "TYPE_3PC_VOTE_ABORT";
    case Request::TYPE_3PC_PRECOMMIT:
      return "TYPE_3PC_PRECOMMIT";
    case Request::TYPE_3PC_PRECOMMIT_ACK:
      return "TYPE_3PC_PRECOMMIT_ACK";
    case Request::TYPE_3PC_GLOBAL_COMMIT:
      return "TYPE_3PC_GLOBAL_COMMIT";
    case Request::TYPE_3PC_GLOBAL_ABORT:
      return "TYPE_3PC_GLOBAL_ABORT";
    case Request::TYPE_POE_EXECUTE:
      return "TYPE_POE_EXECUTE";
    case Request::TYPE_POE_PROOF:
      return "TYPE_POE_PROOF";
    case Request::TYPE_POE_CERT:
      return "TYPE_POE_CERT";
    case Request::TYPE_POE_ROLLBACK:
      return "TYPE_POE_ROLLBACK";
    default:
      return "UNKNOWN";
  }
}

// One compact log line for each sharded consensus message. The same fields are
// logged for global 3PC and local POE so a single grep can reconstruct the
// transaction path across shards.
void TraceShardedPOERequest(const char* prefix,
                            const ShardMetadata& shard_metadata,
                            const Request& request) {
  LOG(ERROR) << prefix
             << " node:" << shard_metadata.SelfNodeId()
             << " shard:" << shard_metadata.SelfShardId()
             << " is_leader:" << shard_metadata.IsSelfShardLeader()
             << " type_name:" << TraceRequestTypeName(request.type())
             << " type_id:" << request.type()
             << " sender_id:" << request.sender_id()
             << " seq:" << request.seq()
             << " global_txn_id:"
             << (request.has_global_txn_id() ? request.global_txn_id() : 0)
             << " coordinator_shard_id:"
             << (request.has_coordinator_shard_id()
                     ? request.coordinator_shard_id()
                     : 0)
             << " global_coordinator_id:"
             << (request.has_global_coordinator_id()
                     ? request.global_coordinator_id()
                     : 0)
             << " local_shard_id:"
             << (request.has_local_shard_id() ? request.local_shard_id() : 0)
             << " proxy_id:" << request.proxy_id();
}

ResDBConfig BuildConsensusConfigFromMetadata(const ResDBConfig& config,
                                             const ShardMetadata& metadata) {
  // Client/proxy nodes do not belong to a server shard. They still need the
  // full network config because ShardedResponseManager routes batches to shard
  // leaders across the deployment.
  if (!metadata.HasLocalShard()) {
    return config;
  }

  // The shard config stores node ids only. Convert those ids into the full
  // ReplicaInfo entries from server.config so local quorum sizing and network
  // identity follow the local shard rather than the whole deployment.
  std::map<int64_t, ReplicaInfo> replicas_by_node;
  for (const auto& replica : config.GetReplicaInfos()) {
    replicas_by_node.emplace(replica.id(), replica);
  }

  std::vector<ReplicaInfo> local_replicas;
  for (int64_t node_id : metadata.LocalShardReplicas()) {
    auto it = replicas_by_node.find(node_id);
    if (it == replicas_by_node.end()) {
      throw std::invalid_argument("shard config references unknown server node " +
                                  std::to_string(node_id));
    }
    local_replicas.push_back(it->second);
  }

  return ResDBConfig(local_replicas, config.GetSelfInfo(),
                     config.GetConfigData(), config.GetPrivateKey(),
                     config.GetPublicKeyCertificateInfo());
}

ResDBConfig BuildShardedConsensusConfig(
    const ResDBConfig& config, const std::string& shard_config_path) {
  // The PBFT base constructor needs the consensus config before derived
  // members are initialized, so parse metadata here and again in the
  // constructor member for runtime use.
  ShardMetadata metadata(shard_config_path, config.GetSelfInfo().id());
  return BuildConsensusConfigFromMetadata(config, metadata);
}

}  // namespace

ConsensusManagerSharded3PCPOE::ConsensusManagerSharded3PCPOE(
    const ResDBConfig& config, const std::string& shard_config_path,
    std::unique_ptr<TransactionManager> executor)
    : ConsensusManagerPBFT(config,
                           BuildShardedConsensusConfig(config,
      shard_config_path),
                           std::move(executor),
                           // This derived manager installs its own global 3PC
                           // and local POE commitments after shard metadata is
                           // available.
                           /*defer_recovery_init=*/true,
                           /*defer_commitment_init=*/true,
                           /*query_executor=*/nullptr,
                           // Proxy nodes use ShardedResponseManager. Server
                           // nodes do not need PBFT's client batching thread.
                           /*defer_response_manager_init=*/true) {
  shard_metadata_.emplace(shard_config_path, config.GetSelfInfo().id());

  if (shard_metadata_->IsSelfClient()) {
    // Client/proxy nodes only batch client requests and route them to shard
    // leaders. They do not own global 3PC or local POE commitments.
    response_manager_ = std::make_unique<ShardedResponseManager>(
        config_, GetBroadCastClient(), system_info_.get(),
        GetSignatureVerifier(), &(*shard_metadata_));
    LOG(ERROR) << "Sharded 3PC/POE proxy initialized. node:"
               << shard_metadata_->SelfNodeId();
    return;
  }

  local_consensus_config_.emplace(
      BuildConsensusConfigFromMetadata(config, *shard_metadata_));
  // Local POE treats the shard leader as the local primary/leader. Global 3PC
  // coordinator identity remains request metadata, not SystemInfo primary.
  system_info_->SetPrimary(static_cast<uint32_t>(
      shard_metadata_->LeaderForShard(shard_metadata_->SelfShardId())));

  // ShardCommunicator reuses the underlying ReplicaCommunicator but limits
  // outgoing consensus messages to local shard replicas or shard leaders.
  shard_communicator_ = std::make_unique<ShardCommunicator>(
      GetBroadCastClient(), &(*shard_metadata_), config.GetReplicaInfos());
  // Local consensus module for this POE variant. In Phase 2 it only validates
  // local POE execute messages; later phases will add execution/proofs.
  commitment_ = std::make_unique<ShardPOECommitment>(
      *local_consensus_config_, message_manager_.get(), GetBroadCastClient(),
      shard_communicator_.get(), &(*shard_metadata_), GetSignatureVerifier());
  // The global 3PC object is the same sharded wrapper used by the PBFT variant,
  // but its global commit callback starts local POE instead of local PBFT.
  commitment_3pc_ = std::make_unique<Shard3PCCommitment>(
      config_, message_manager_.get(), GetBroadCastClient(),
      shard_communicator_.get(), &(*shard_metadata_), GetSignatureVerifier(),
      [this](const Request& request) {
        return StartLocalPOEFromGlobalCommit(request);
      });

  // All shards will eventually execute the transaction, but only replicas in
  // the coordinator shard should return client responses.
  message_manager_->SetResponseFilter([this](const Request& request) {
    return ShouldEnqueueClientResponse(request);
  });
  // The executor duplicate manager should belong to the local consensus module,
  // not the global 3PC module.
  message_manager_->SetDuplicateManager(commitment_->GetDuplicateManager());

  if (view_change_manager_ != nullptr) {
    view_change_manager_->SetDuplicateManager(
        commitment_->GetDuplicateManager());
  }

  LOG(ERROR) << "Sharded 3PC/POE initialized. node:"
             << shard_metadata_->SelfNodeId()
             << " shard:" << shard_metadata_->SelfShardId()
             << " shard leader:" << shard_metadata_->IsSelfShardLeader();

  InitRecovery3PC();
}

void ConsensusManagerSharded3PCPOE::InitRecovery3PC() {
  // Recovery log replay is inherited from PBFT, but replayed through the POE
  // dispatcher so recovered 3PC and POE messages reach the right module.
  recovery_->ReadLogs(
      [&](const SystemInfoData& data) {
        LOG(ERROR) << " read data info:" << data.view()
                   << " primary/coordinator:" << data.primary_id();
        system_info_->SetCurrentView(data.view());
        system_info_->SetPrimary(data.primary_id());
      },
      [&](std::unique_ptr<Context> context, std::unique_ptr<Request> request) {
        return InternalConsensusCommitPOE(std::move(context),
                                          std::move(request));
      },
      [&](int seq) { message_manager_->SetNextCommitSeq(seq + 1); });
  LOG(ERROR) << "sharded 3PC/POE recovery is done";
}

ShardPOECommitment* ConsensusManagerSharded3PCPOE::GetShardPOECommitment() {
  return static_cast<ShardPOECommitment*>(commitment_.get());
}

bool ConsensusManagerSharded3PCPOE::ShouldEnqueueClientResponse(
    const Request& request) const {
  // Requests without coordinator_shard_id are not sharded client operations in
  // this path. Suppress them rather than risk a non-coordinator shard replying.
  if (!shard_metadata_.has_value() || !shard_metadata_->HasLocalShard() ||
      !request.has_coordinator_shard_id()) {
    return false;
  }
  return request.coordinator_shard_id() == shard_metadata_->SelfShardId();
}

int ConsensusManagerSharded3PCPOE::StartLocalPOEFromGlobalCommit(
    const Request& committed_request) {
  // Global 3PC commit is delivered to shard leaders. Only a shard leader should
  // convert that global decision into local POE traffic for its shard.
  if (!shard_metadata_.has_value() || !shard_metadata_->IsSelfShardLeader()) {
    return 0;
  }
  if (commitment_ == nullptr) {
    LOG(ERROR) << "local shard POE commitment is not initialized";
    return -2;
  }

  Request local_request(committed_request);
  // TYPE_POE_EXECUTE is the boundary between global commit and local POE. The
  // POE commitment fills the leader/view fields before broadcasting.
  local_request.set_type(Request::TYPE_POE_EXECUTE);
  local_request.set_local_shard_id(shard_metadata_->SelfShardId());
  local_request.set_is_local_shard_poe(true);
  // These fallback assignments keep recovery and manually generated tests from
  // failing before all sharded metadata producers are upgraded.
  if (!local_request.has_global_coordinator_id()) {
    local_request.set_global_coordinator_id(committed_request.primary_id() != 0
                                                ? committed_request.primary_id()
                                                : committed_request.sender_id());
  }
  if (!local_request.has_coordinator_shard_id()) {
    local_request.set_coordinator_shard_id(shard_metadata_->SelfShardId());
  }
  if (!local_request.has_global_txn_id()) {
    local_request.set_global_txn_id(local_request.seq());
  }

  LOG(ERROR) << "[sharded_poe_handoff]"
             << " global 3PC commit -> local POE"
             << " node:" << shard_metadata_->SelfNodeId()
             << " shard:" << shard_metadata_->SelfShardId()
             << " seq:" << local_request.seq()
             << " global_txn_id:" << local_request.global_txn_id()
             << " coordinator_shard_id:"
             << local_request.coordinator_shard_id();

  return GetShardPOECommitment()->ProcessGlobalCommittedRequest(local_request);
}

int ConsensusManagerSharded3PCPOE::ConsensusCommit(
    std::unique_ptr<Context> context, std::unique_ptr<Request> request) {
  if (request == nullptr) {
    return -2;
  }
  LOG(INFO) << "recv sharded 3PC/POE type:" << request->type()
            << " sender id:" << request->sender_id()
            << " primary/coordinator:" << system_info_->GetPrimaryId();

  if (view_change_manager_ && view_change_manager_->IsInViewChange()) {
    // Hold only global 3PC and local POE messages during view change. PBFT
    // local phase messages are intentionally absent from this POE manager.
    switch (request->type()) {
      case Request::TYPE_NEW_TXNS:
      case Request::TYPE_3PC_PREPARE:
      case Request::TYPE_3PC_VOTE_COMMIT:
      case Request::TYPE_3PC_VOTE_ABORT:
      case Request::TYPE_3PC_PRECOMMIT:
      case Request::TYPE_3PC_PRECOMMIT_ACK:
      case Request::TYPE_3PC_GLOBAL_COMMIT:
      case Request::TYPE_3PC_GLOBAL_ABORT:
      case Request::TYPE_POE_EXECUTE:
      case Request::TYPE_POE_PROOF:
      case Request::TYPE_POE_CERT:
      case Request::TYPE_POE_ROLLBACK:
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
      InternalConsensusCommitPOE(std::move((*new_request).first),
                                 std::move((*new_request).second));
    }
  }

  return InternalConsensusCommitPOE(std::move(context), std::move(request));
}

int ConsensusManagerSharded3PCPOE::InternalConsensusCommitPOE(
    std::unique_ptr<Context> context, std::unique_ptr<Request> request) {
  if (request == nullptr) {
    return -2;
  }
  const int request_type = request->type();
  // Client/proxy nodes have no commitment modules. These checks catch topology
  // mistakes if server-side consensus traffic is delivered to a proxy process.
  if (NeedsGlobal3PCCommitment(request_type) && commitment_3pc_ == nullptr) {
    LOG(ERROR) << "global sharded 3PC commitment is not initialized";
    return -2;
  }
  if (NeedsLocalPOECommitment(request_type) && commitment_ == nullptr) {
    LOG(ERROR) << "local shard POE commitment is not initialized";
    return -2;
  }

  if (shard_metadata_.has_value() && shard_metadata_->HasLocalShard()) {
    // Use separate prefixes so logs can distinguish global 3PC from local POE
    // without changing protocol behavior.
    if (NeedsGlobal3PCCommitment(request_type)) {
      TraceShardedPOERequest("[sharded_3pc_trace]", *shard_metadata_,
                             *request);
    } else if (NeedsLocalPOECommitment(request_type)) {
      TraceShardedPOERequest("[shard_poe_trace]", *shard_metadata_, *request);
    }
  }

  switch (request_type) {
    case Request::TYPE_CLIENT_REQUEST:
      if (config_.IsPerformanceRunning()) {
        if (performance_manager_ != nullptr) {
          return performance_manager_->StartEval();
        }
      }
      return response_manager_->NewUserRequest(std::move(context),
                                               std::move(request));

    case Request::TYPE_RESPONSE:
      if (config_.IsPerformanceRunning() && performance_manager_ != nullptr) {
        // Performance manager is still not updated for 3pc so this is here for formality.
        return performance_manager_->ProcessResponseMsg(std::move(context),
                                                        std::move(request));
      }
      return response_manager_->ProcessResponseMsg(std::move(context),
                                                   std::move(request));

    case Request::TYPE_NEW_TXNS: {
      // Non leader replicas ignore accidental deliveries.
      if (!shard_metadata_->IsSelfShardLeader()) {
        LOG(WARNING) << "non-leader received sharded global NEW_TXNS. node:"
                     << shard_metadata_->SelfNodeId();
        return 0;
      }
      uint64_t proxy_id = request->proxy_id();
      std::string hash = request->hash();
      int ret =
          commitment_3pc_->ProcessNewRequest(std::move(context),
                                             std::move(request));
      if (ret == -3) {
        // Preserve PBFT/3PC complaint plumbing for the global phase. POE has no
        // local complaint behavior yet.
        std::pair<std::unique_ptr<Context>, std::unique_ptr<Request>>
            request_complained;
        {
          std::lock_guard<std::mutex> lk(commitment_3pc_->rc_mutex_);
          request_complained =
              std::move(commitment_3pc_->request_complained_.front());
          commitment_3pc_->request_complained_.pop();
        }
        AddComplainedRequest(std::move(request_complained.first),
                             std::move(request_complained.second));
        if (view_change_manager_ != nullptr) {
          view_change_manager_->AddComplaintTimer(proxy_id, hash);
        }
      }
      return ret;
    }

    case Request::TYPE_3PC_PREPARE:
      // Only shard leaders participate in global 3PC. Non-leader replicas join
      // later through local POE once their leader broadcasts TYPE_POE_EXECUTE.
      if (!shard_metadata_->IsSelfShardLeader()) {
        return 0;
      }
      return GetCommitment3PC()->ProcessPrepareMsg(std::move(context),
                                                   std::move(request));
    case Request::TYPE_3PC_VOTE_COMMIT:
      if (!shard_metadata_->IsSelfShardLeader()) {
        return 0;
      }
      return GetCommitment3PC()->ProcessVoteCommitMsg(std::move(context),
                                                      std::move(request));
    case Request::TYPE_3PC_VOTE_ABORT:
      if (!shard_metadata_->IsSelfShardLeader()) {
        return 0;
      }
      return GetCommitment3PC()->ProcessVoteAbortMsg(std::move(context),
                                                     std::move(request));
    case Request::TYPE_3PC_PRECOMMIT:
      if (!shard_metadata_->IsSelfShardLeader()) {
        return 0;
      }
      return GetCommitment3PC()->ProcessPreCommitMsg(std::move(context),
                                                     std::move(request));
    case Request::TYPE_3PC_PRECOMMIT_ACK:
      if (!shard_metadata_->IsSelfShardLeader()) {
        return 0;
      }
      return GetCommitment3PC()->ProcessPreCommitAckMsg(std::move(context),
                                                        std::move(request));
    case Request::TYPE_3PC_GLOBAL_COMMIT:
      if (!shard_metadata_->IsSelfShardLeader()) {
        return 0;
      }
      return GetCommitment3PC()->ProcessGlobalCommitMsg(std::move(context),
                                                        std::move(request));
    case Request::TYPE_3PC_GLOBAL_ABORT:
      if (!shard_metadata_->IsSelfShardLeader()) {
        return 0;
      }
      return GetCommitment3PC()->ProcessGlobalAbortMsg(std::move(context),
                                                       std::move(request));
    case Request::TYPE_POE_EXECUTE:
      return GetShardPOECommitment()->ProcessPOEExecuteMsg(std::move(context), 
                                                           std::move(request));
    case Request::TYPE_POE_PROOF:
      return GetShardPOECommitment()->ProcessPOEProofMsg(std::move(context),
                                                         std::move(request));
    case Request::TYPE_POE_CERT:
      return GetShardPOECommitment()->ProcessPOECertMsg(std::move(context),
                                                        std::move(request));
    case Request::TYPE_POE_ROLLBACK:
      return GetShardPOECommitment()->ProcessPOERollbackMsg(std::move(context), 
                                                            std::move(request));
    case Request::TYPE_QUERY:
      return query_->ProcessQuery(std::move(context), 
                                  std::move(request));
    case Request::TYPE_REPLICA_STATE:
      return query_->ProcessGetReplicaState(std::move(context),
                                            std::move(request));
    case Request::TYPE_CUSTOM_QUERY:
      return query_->ProcessCustomQuery(std::move(context),
                                        std::move(request));
    case Request::TYPE_RECOVERY_DATA:
      return ProcessRecoveryData(std::move(context), 
                                 std::move(request));
    case Request::TYPE_RECOVERY_DATA_RESP:
      return ProcessRecoveryDataResponse(std::move(context),
                                         std::move(request));
    default:
      LOG(WARNING) << "Unknown request type under sharded 3PC/POE: "
                   << request->type();
      return 0;
  }
}

}  // namespace resdb

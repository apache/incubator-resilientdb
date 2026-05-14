#include "platform/consensus/ordering/sharded_3pc/consensus_manager_sharded_3pc.h"

#include <glog/logging.h>

#include <map>
#include <stdexcept>
#include <utility>

#include "platform/consensus/ordering/sharded_3pc/shard_3pc_commitment.h"
#include "platform/consensus/ordering/sharded_3pc/shard_pbft_commitment.h"
#include "platform/consensus/ordering/sharded_3pc/sharded_response_manager.h"

namespace resdb {
namespace {

// Helper functions to determine which commitment module should process a request based on its type. 
// This centralizes the logic for routing requests to the global 3PC commitment or the local shard PBFT commitment.
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

bool NeedsLocalPBFTCommitment(int request_type) {
  switch (request_type) {
    case Request::TYPE_SHARD_PBFT_NEW_TXN:
    case Request::TYPE_PRE_PREPARE:
    case Request::TYPE_PREPARE:
    case Request::TYPE_COMMIT:
      return true;
    default:
      return false;
  }
}

ResDBConfig BuildConsensusConfigFromMetadata(const ResDBConfig& config,
                                             const ShardMetadata& metadata) {
  // Client/proxy nodes have no local PBFT shard. They still need the
  // network-wide config so the proxy can route requests to any shard leader.
  if (!metadata.HasLocalShard()) {
    return config;
  }

  // Build a lookup from the network-wide ResDB config. The shard config stores
  // only node ids, so the full ReplicaInfo records must come from server.config.
  std::map<int64_t, ReplicaInfo> replicas_by_node;
  for (const auto& replica : config.GetReplicaInfos()) {
    replicas_by_node.emplace(replica.id(), replica);
  }

  // PBFT quorum sizing is driven by ResDBConfig::GetReplicaNum(). For sharded
  // PBFT this must be the local shard size, not the full deployment size.
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
  // The PBFT base constructor needs the module config before this derived
  // object can initialize its shard_metadata_ member, so parse once here.
  ShardMetadata metadata(shard_config_path, config.GetSelfInfo().id());
  return BuildConsensusConfigFromMetadata(config, metadata);
}

}  // namespace

ConsensusManagerSharded3PC::ConsensusManagerSharded3PC(
    const ResDBConfig& config, const std::string& shard_config_path,
    std::unique_ptr<TransactionManager> executor)
    : ConsensusManagerPBFT(config,
                           BuildShardedConsensusConfig(config,
                                                       shard_config_path),
                           std::move(executor),
                           // The sharded manager installs its own local PBFT
                           // and global 3PC commitments after metadata exists.
                           /*defer_recovery_init=*/true,
                           /*defer_commitment_init=*/true,
                           /*query_executor=*/nullptr,
                           // Proxy nodes need ShardedResponseManager, and
                           // server nodes do not need a client batching thread.
                           /*defer_response_manager_init=*/true) {
  shard_metadata_.emplace(shard_config_path, config.GetSelfInfo().id());

  if (shard_metadata_->IsSelfClient()) {
    // Client/proxy nodes only batch client requests and route them to shard
    // leaders. They do not own PBFT/3PC commitments or recovery state.
    response_manager_ = std::make_unique<ShardedResponseManager>(
        config_, GetBroadCastClient(), system_info_.get(),
        GetSignatureVerifier(), &(*shard_metadata_));
    LOG(ERROR) << "Sharded 3PC proxy initialized. node:"
               << shard_metadata_->SelfNodeId();
    return;
  }

  local_consensus_config_.emplace(
      BuildConsensusConfigFromMetadata(config, *shard_metadata_));
  // Local PBFT should view the shard leader as its primary. Global 3PC
  // coordinator identity is carried separately in request metadata.
  system_info_->SetPrimary(static_cast<uint32_t>(
      shard_metadata_->LeaderForShard(shard_metadata_->SelfShardId())));

  // ShardCommunicator keeps the network communicator intact but constrains
  // outgoing consensus targets to local shard replicas or shard leaders.
  shard_communicator_ = std::make_unique<ShardCommunicator>(
      GetBroadCastClient(), &(*shard_metadata_), config.GetReplicaInfos());
  commitment_ = std::make_unique<ShardPBFTCommitment>(
      *local_consensus_config_, message_manager_.get(), GetBroadCastClient(),
      shard_communicator_.get(), GetSignatureVerifier());
  // The global 3PC commitment does not execute directly. On global commit it
  // calls back into this manager, which starts local PBFT in the current shard.
  commitment_3pc_ = std::make_unique<Shard3PCCommitment>(
      config_, message_manager_.get(), GetBroadCastClient(),
      shard_communicator_.get(), &(*shard_metadata_), GetSignatureVerifier(),
      [this](const Request& request) {
        return StartLocalPBFTFromGlobalCommit(request);
      });

  // All shards execute the transaction, but only replicas in the coordinator
  // shard are allowed to enqueue responses back to the proxy.
  message_manager_->SetResponseFilter([this](const Request& request) {
    return ShouldEnqueueClientResponse(request);
  });
  // Shard3PCCommitment also has a DuplicateManager through its PBFT base class,
  // so reinstall the local PBFT duplicate manager as the executor authority.
  message_manager_->SetDuplicateManager(commitment_->GetDuplicateManager());

  if (view_change_manager_ != nullptr) {
    view_change_manager_->SetDuplicateManager(
        commitment_->GetDuplicateManager());
  }

  LOG(ERROR) << "Sharded 3PC placeholder initialized. node:"
             << shard_metadata_->SelfNodeId()
             << " shard:" << shard_metadata_->SelfShardId()
             << " shard leader:" << shard_metadata_->IsSelfShardLeader();

  InitRecovery3PC();
}

void ConsensusManagerSharded3PC::InitRecovery3PC() {
  // Reuse PBFT recovery storage and replay machinery, but replay recovered
  // messages through the sharded 3PC/PBFT dispatcher below.
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
  LOG(ERROR) << "sharded 3PC recovery is done";
}

ShardPBFTCommitment* ConsensusManagerSharded3PC::GetShardPBFTCommitment() {
  return static_cast<ShardPBFTCommitment*>(commitment_.get());
}

bool ConsensusManagerSharded3PC::ShouldEnqueueClientResponse(
    const Request& request) const {
  // Requests without coordinator_shard_id are not sharded client transactions.
  // Suppressing them here avoids accidental replies from non-coordinator shards.
  if (!shard_metadata_.has_value() || !shard_metadata_->HasLocalShard() ||
      !request.has_coordinator_shard_id()) {
    return false;
  }
  return request.coordinator_shard_id() == shard_metadata_->SelfShardId();
}

int ConsensusManagerSharded3PC::StartLocalPBFTFromGlobalCommit(
    const Request& committed_request) {
  // Global 3PC traffic is leader-to-leader. Only the local shard leader starts
  // PBFT replication inside its shard after the global commit decision.
  if (!shard_metadata_.has_value() || !shard_metadata_->IsSelfShardLeader()) {
    return 0;
  }
  if (commitment_ == nullptr) {
    LOG(ERROR) << "local shard PBFT commitment is not initialized";
    return -2;
  }

  Request local_request(committed_request);
  // TYPE_SHARD_PBFT_NEW_TXN is an internal handoff marker. The PBFT wrapper
  // converts it into TYPE_PRE_PREPARE while preserving the global sequence.
  local_request.set_type(Request::TYPE_SHARD_PBFT_NEW_TXN);
  local_request.set_local_shard_id(shard_metadata_->SelfShardId());
  local_request.set_is_local_shard_pbft(true);
  if (!local_request.has_global_coordinator_id()) {
    // Recovery or older requests may not carry the sharded metadata. Fall back
    // to the request's coordinator so replay can still progress.
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

  return GetShardPBFTCommitment()->ProcessGlobalCommittedRequest(local_request);
}

int ConsensusManagerSharded3PC::ConsensusCommit(
    std::unique_ptr<Context> context, std::unique_ptr<Request> request) {
  if (request == nullptr) {
    return -2;
  }
  LOG(INFO) << "recv sharded 3PC type:" << request->type()
            << " sender id:" << request->sender_id()
            << " primary/coordinator:" << system_info_->GetPrimaryId();

  if (view_change_manager_ && view_change_manager_->IsInViewChange()) {
    // Hold ordering messages during view change, mirroring PBFT behavior. They
    // are replayed through InternalConsensusCommit3PC when the view stabilizes.
    switch (request->type()) {
      case Request::TYPE_NEW_TXNS:
      case Request::TYPE_SHARD_PBFT_NEW_TXN:
      case Request::TYPE_PRE_PREPARE:
      case Request::TYPE_PREPARE:
      case Request::TYPE_COMMIT:
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

int ConsensusManagerSharded3PC::InternalConsensusCommit3PC(
    std::unique_ptr<Context> context, std::unique_ptr<Request> request) {
  if (request == nullptr) {
    return -2;
  }
  const int request_type = request->type();
  // These checks catch topology mistakes early, for example a client node
  // receiving server-side consensus traffic before its commitments exist.
  if (NeedsGlobal3PCCommitment(request_type) && commitment_3pc_ == nullptr) {
    LOG(ERROR) << "global sharded 3PC commitment is not initialized";
    return -2;
  }
  if (NeedsLocalPBFTCommitment(request_type) && commitment_ == nullptr) {
    LOG(ERROR) << "local shard PBFT commitment is not initialized";
    return -2;
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
        return performance_manager_->ProcessResponseMsg(std::move(context),
                                                        std::move(request));
      }
      return response_manager_->ProcessResponseMsg(std::move(context),
                                                   std::move(request));

    case Request::TYPE_NEW_TXNS: {
      // Proxies route TYPE_NEW_TXNS directly to the selected shard leader.
      // Non-leaders ignore accidental deliveries.
      if (!shard_metadata_->IsSelfShardLeader()) {
        LOG(WARNING) << "non-leader received sharded global NEW_TXNS. node:"
                     << shard_metadata_->SelfNodeId();
        return 0;
      }
      if (commitment_3pc_ == nullptr) {
        LOG(ERROR) << "sharded 3PC commitment is not initialized";
        return -2;
      }
      uint64_t proxy_id = request->proxy_id();
      std::string hash = request->hash();
      int ret =
          commitment_3pc_->ProcessNewRequest(std::move(context),
                                             std::move(request));
      if (ret == -3) {
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

    case Request::TYPE_SHARD_PBFT_NEW_TXN:
      // This type is the boundary between global ordering and local ordering.
      // Checks that PBFT local commiitment exists.
      if (commitment_ == nullptr) {
        LOG(ERROR) << "local shard PBFT commitment is not initialized";
        return -2;
      }
      return StartLocalPBFTFromGlobalCommit(*request);

    case Request::TYPE_3PC_PREPARE:
      // Global 3PC is scoped to shard leaders. Non-leader replicas only
      // participate once local PBFT starts inside their shard.
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

    case Request::TYPE_PRE_PREPARE:
      return commitment_->ProcessProposeMsg(std::move(context),
                                            std::move(request));
    case Request::TYPE_PREPARE:
      return commitment_->ProcessPrepareMsg(std::move(context),
                                            std::move(request));
    case Request::TYPE_COMMIT:
      return commitment_->ProcessCommitMsg(std::move(context),
                                           std::move(request));

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
    default:
      LOG(WARNING) << "Unknown request type under sharded 3PC: "
                   << request->type();
      return 0;
  }
}

}  // namespace resdb

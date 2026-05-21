#include "platform/consensus/ordering/sharded_3pc/shard_poe_commitment.h"

#include <glog/logging.h>

#include <exception>
#include <stdexcept>

namespace resdb {

ShardPOECommitment::ShardPOECommitment(
    const ResDBConfig& config, MessageManager* message_manager,
    ReplicaCommunicator* replica_communicator,
    ShardCommunicator* shard_communicator,
    const ShardMetadata* shard_metadata, SignatureVerifier* verifier)
    : Commitment(config, message_manager, replica_communicator, verifier),
      shard_communicator_(shard_communicator),
      shard_metadata_(shard_metadata) {
  if (shard_communicator_ == nullptr) {
    throw std::invalid_argument("ShardPOECommitment requires communicator");
  }
  if (shard_metadata_ == nullptr) {
    throw std::invalid_argument("ShardPOECommitment requires metadata");
  }
}

int ShardPOECommitment::ProcessGlobalCommittedRequest(
    const Request& committed_request) {
  // Global 3PC is leader-to-leader. Once that global decision commits, each
  // shard leader is responsible for starting the local POE phase in its shard.
  if (!shard_metadata_->IsSelfShardLeader()) {
    LOG(INFO) << "non-leader ignored POE local start. node:"
              << shard_metadata_->SelfNodeId()
              << " seq:" << committed_request.seq();
    return 0;
  }

  // Reuse the PBFT duplicate manager as the local "already started" guard. In
  // Phase 2 this prevents repeated POE_EXECUTE broadcasts for the same global
  // commit. Later phases can reuse the same guard around proof/cert handling.
  if (duplicate_manager_->CheckIfExecuted(committed_request.hash()) ||
      duplicate_manager_->CheckAndAddProposed(committed_request.hash())) {
    return 0;
  }

  // TYPE_POE_EXECUTE is the local-shard handoff message. It preserves the
  // globally ordered request, then adds the local shard target and POE marker 
  // so receivers can validate the message cheaply.
  Request local_request(committed_request);
  local_request.set_type(Request::TYPE_POE_EXECUTE);
  local_request.set_current_view(message_manager_->GetCurrentView());
  local_request.set_sender_id(config_.GetSelfInfo().id());
  local_request.set_primary_id(config_.GetSelfInfo().id());
  local_request.set_local_shard_id(shard_metadata_->SelfShardId());
  local_request.set_is_local_shard_poe(true);
  // Recovery/replay or older generated requests may be missing the new
  // sharded metadata. Fill with defaults so Phase 2 routing remains
  // debuggable. Normal proxy-routed requests already carry these fields.
  if (!local_request.has_global_coordinator_id()) {
    local_request.set_global_coordinator_id(committed_request.primary_id() != 0
                                                ? committed_request.primary_id()
                                                : committed_request.sender_id());
  }

  // Not neccessary for poe execution but these makes logs consistent.
  if (!local_request.has_coordinator_shard_id()) {
    local_request.set_coordinator_shard_id(shard_metadata_->SelfShardId());
  }
  if (!local_request.has_global_txn_id()) {
    local_request.set_global_txn_id(local_request.seq());
  }

  LOG(ERROR) << "[shard_poe_trace]"
             << " local POE execute broadcast"
             << " node:" << shard_metadata_->SelfNodeId()
             << " shard:" << shard_metadata_->SelfShardId()
             << " seq:" << local_request.seq()
             << " global_txn_id:" << local_request.global_txn_id()
             << " coordinator_shard_id:"
             << local_request.coordinator_shard_id();

  // Broadcast the local POE execute trigger to the shard. Only local shard leaders send this message.
  BroadcastConsensusMsg(local_request);
  return 0;
}

int ShardPOECommitment::ProcessPOEExecuteMsg(
    std::unique_ptr<Context> context, std::unique_ptr<Request> request) {
  if (context == nullptr) {
    LOG(ERROR) << "POE EXECUTE missing context";
    return -2;
  }
  if (request == nullptr) {
    LOG(ERROR) << "POE EXECUTE missing request";
    return -2;
  }
  // This is deliberately validation-only in at this point. 
  // Execution will be implemented when the proof and cert phases are added. 
  if (!ValidatePOEExecuteRequest(*context, *request)) {
    return -2;
  }

  LOG(ERROR) << "[shard_poe_trace]"
             << " validated local POE execute"
             << " node:" << shard_metadata_->SelfNodeId()
             << " shard:" << shard_metadata_->SelfShardId()
             << " seq:" << request->seq()
             << " sender_id:" << request->sender_id()
             << " global_txn_id:" << request->global_txn_id()
             << " coordinator_shard_id:" << request->coordinator_shard_id();
  return 0;
}

int ShardPOECommitment::ProcessPOEProofMsg(
    std::unique_ptr<Context> context, std::unique_ptr<Request> request) {
  (void)context;
  (void)request;
  LOG(ERROR) << "[shard_poe_trace] POE proof handler placeholder";
  return 0;
}

int ShardPOECommitment::ProcessPOECertMsg(
    std::unique_ptr<Context> context, std::unique_ptr<Request> request) {
  (void)context;
  (void)request;
  LOG(ERROR) << "[shard_poe_trace] POE cert handler placeholder";
  return 0;
}

int ShardPOECommitment::ProcessPOERollbackMsg(
    std::unique_ptr<Context> context, std::unique_ptr<Request> request) {
  (void)context;
  (void)request;
  LOG(ERROR) << "[shard_poe_trace] POE rollback handler placeholder";
  return 0;
}

bool ShardPOECommitment::ValidatePOEExecuteRequest(
    const Context& context, const Request& request) const {
  // TYPE_POE_EXECUTE is the local execution trigger, so require a
  // signed context and make sure the signature metadata matches the sender. Similar to PBFT.
  if (context.signature.signature().empty()) {
    LOG(ERROR) << "POE EXECUTE missing signature";
    return false;
  }
  if (context.signature.node_id() != 0 &&
      context.signature.node_id() != request.sender_id()) {
    LOG(ERROR) << "POE EXECUTE signature sender mismatch. signature node:"
               << context.signature.node_id()
               << " request sender:" << request.sender_id();
    return false;
  }
  if (request.type() != Request::TYPE_POE_EXECUTE) {
    LOG(ERROR) << "POE EXECUTE wrong type:" << request.type();
    return false;
  }
  if (!shard_metadata_->HasLocalShard()) {
    LOG(ERROR) << "POE EXECUTE received on node without local shard";
    return false;
  }
  // Only the configured leader for this shard may start local POE. This is the
  // equivalent in sharded PBFT validation.
  const uint32_t local_shard_id = shard_metadata_->SelfShardId();
  const int64_t local_leader = shard_metadata_->LeaderForShard(local_shard_id);
  if (request.sender_id() != local_leader) {
    LOG(ERROR) << "POE EXECUTE invalid sender:" << request.sender_id()
               << " expected leader:" << local_leader;
    return false;
  }
  // Ensure replica has valid shard and that the request has been marked as POE.
  if (!request.has_local_shard_id() ||
      request.local_shard_id() != local_shard_id) {
    LOG(ERROR) << "POE EXECUTE wrong local shard. request shard:"
               << (request.has_local_shard_id() ? request.local_shard_id()
                                                : 0)
               << " expected:" << local_shard_id;
    return false;
  }
  if (!request.is_local_shard_poe()) {
    LOG(ERROR) << "POE EXECUTE missing local POE marker";
    return false;
  }
  // These fields bind the local POE trigger back to the globally ordered
  // transaction and the client/proxy response path.
  if (request.seq() == 0 || request.hash().empty() ||
      request.proxy_id() == 0) {
    LOG(ERROR) << "POE EXECUTE missing core request identity. seq:"
               << request.seq() << " hash_size:" << request.hash().size()
               << " proxy_id:" << request.proxy_id();
    return false;
  }
  if (!request.has_global_txn_id() ||
      !request.has_coordinator_shard_id() ||
      !request.has_global_coordinator_id()) {
    LOG(ERROR) << "POE EXECUTE missing sharded global metadata";
    return false;
  }
  // The coordinator shard metadata is not local-shard-specific: every shard
  // should agree which shard leader coordinated global 3PC for this request.
  int64_t expected_global_coordinator = 0;
  try {
    expected_global_coordinator =
        shard_metadata_->LeaderForShard(request.coordinator_shard_id());
  } catch (const std::exception& e) {
    LOG(ERROR) << "POE EXECUTE invalid coordinator shard metadata: "
               << e.what();
    return false;
  }
  if (expected_global_coordinator != request.global_coordinator_id()) {
    LOG(ERROR) << "POE EXECUTE coordinator metadata mismatch. shard:"
               << request.coordinator_shard_id()
               << " coordinator:" << request.global_coordinator_id();
    return false;
  }
  return true;
}

int ShardPOECommitment::BroadcastConsensusMsg(
    const google::protobuf::Message& msg) {
  return shard_communicator_->BroadcastToLocalShard(msg);
}

int ShardPOECommitment::SendConsensusMsgToReplica(
    const google::protobuf::Message& msg, int64_t node_id) {
  return shard_communicator_->SendToNode(msg, node_id);
}

}  // namespace resdb

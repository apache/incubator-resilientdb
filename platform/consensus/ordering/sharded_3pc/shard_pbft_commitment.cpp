#include "platform/consensus/ordering/sharded_3pc/shard_pbft_commitment.h"

#include <stdexcept>

namespace resdb {

ShardPBFTCommitment::ShardPBFTCommitment(
    const ResDBConfig& config, MessageManager* message_manager,
    ReplicaCommunicator* replica_communicator,
    ShardCommunicator* shard_communicator, SignatureVerifier* verifier)
    : Commitment(config, message_manager, replica_communicator, verifier),
      shard_communicator_(shard_communicator) {
  if (shard_communicator_ == nullptr) {
    throw std::invalid_argument("ShardPBFTCommitment requires communicator");
  }
}

int ShardPBFTCommitment::ProcessGlobalCommittedRequest(
    const Request& committed_request) {
  if (duplicate_manager_->CheckIfExecuted(committed_request.hash()) ||
      duplicate_manager_->CheckAndAddProposed(committed_request.hash())) {
    return 0;
  }

  // The global 3PC layer has committed this request. Now it must be fed into local PBFT for ordering within the shard. 
  // This is done by broadcasting a new PRE_PREPARE message to the local shard's PBFT protocol with the same request content and a flag indicating it's for local shard PBFT. 
  // The local PBFT commitment module will recognize the flag and process it accordingly, rather than treating it as a new client request. 
  Request local_request(committed_request);
  local_request.set_type(Request::TYPE_PRE_PREPARE);
  local_request.set_current_view(message_manager_->GetCurrentView());
  local_request.set_sender_id(config_.GetSelfInfo().id());
  local_request.set_primary_id(config_.GetSelfInfo().id());
  local_request.set_is_local_shard_pbft(true);

  BroadcastConsensusMsg(local_request);
  return 0;
}

int ShardPBFTCommitment::BroadcastConsensusMsg(
    const google::protobuf::Message& msg) {
  return shard_communicator_->BroadcastToLocalShard(msg);
}

int ShardPBFTCommitment::SendConsensusMsgToReplica(
    const google::protobuf::Message& msg, int64_t node_id) {
  return shard_communicator_->SendToNode(msg, node_id);
}

}  // namespace resdb

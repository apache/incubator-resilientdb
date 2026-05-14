#include "platform/consensus/ordering/sharded_3pc/shard_3pc_commitment.h"

#include <stdexcept>
#include <utility>

namespace resdb {

Shard3PCCommitment::Shard3PCCommitment(
    const ResDBConfig& config, MessageManager* message_manager,
    ReplicaCommunicator* replica_communicator,
    ShardCommunicator* shard_communicator,
    const ShardMetadata* shard_metadata, SignatureVerifier* verifier,
    std::function<int(const Request&)> global_commit_handler)
    // The global 3PC object should not drain execution responses. Local PBFT
    // owns execution response sending after the callback starts local ordering.
    : Commitment3PC(config, message_manager, replica_communicator, verifier,
                    /*start_response_thread=*/false),
      shard_communicator_(shard_communicator),
      shard_metadata_(shard_metadata),
      global_commit_handler_(std::move(global_commit_handler)) {
  if (shard_communicator_ == nullptr) {
    throw std::invalid_argument("Shard3PCCommitment requires communicator");
  }
  if (shard_metadata_ == nullptr) {
    throw std::invalid_argument("Shard3PCCommitment requires metadata");
  }
}

int Shard3PCCommitment::BroadcastConsensusMsg(
    const google::protobuf::Message& msg) {
  return shard_communicator_->BroadcastToShardLeaders(msg);
}

int Shard3PCCommitment::SendConsensusMsgToReplica(
    const google::protobuf::Message& msg, int64_t node_id) {
  return shard_communicator_->SendToNode(msg, node_id);
}

size_t Shard3PCCommitment::ExpectedThreePCParticipantCount() const {
  return shard_metadata_->NumShards();
}

int Shard3PCCommitment::OnGlobalCommit(const Request& committed_request) {
  // Only shard leaders participate in global 3PC, so only leaders should
  // trigger the local PBFT handoff for their shard.
  if (!shard_metadata_->IsSelfShardLeader()) {
    return 0;
  }
  if (global_commit_handler_) {
    // Normal sharded path: ConsensusManagerSharded3PC::StartLocalPBFTFromGlobalCommit.
    return global_commit_handler_(committed_request);
  }
  // Test/fallback path: preserve flat 3PC behavior if no handler is installed.
  return Commitment3PC::OnGlobalCommit(committed_request);
}

}  // namespace resdb

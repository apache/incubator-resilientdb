#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

#include "platform/consensus/ordering/3pc/commitment_3pc.h"
#include "platform/consensus/ordering/sharded_3pc/shard_communicator.h"
#include "platform/consensus/ordering/sharded_3pc/shard_metadata.h"

namespace resdb {
/**************************
 * Shard3PCCommitment extends Commitment3PC to implement the 3PC protocol in a sharded deployment.
 * It overrides message broadcasting and participant counting to align with shard membership,
 * using ShardCommunicator to send messages to the appropriate nodes within the shard.
 * - config: ResDB configuration object.
 * - message_manager: pointer to the MessageManager instance for managing consensus messages.
 * - replica_communicator: pointer to the underlying ReplicaCommunicator for sending messages.
 * - shard_communicator: pointer to the ShardCommunicator for shard-aware message sending.
 * - shard_metadata: pointer to the ShardMetadata containing shard configuration and node-shard mappings.
 * - verifier: pointer to the SignatureVerifier for verifying message signatures.
 ***************************/
class Shard3PCCommitment : public Commitment3PC {
 public:
  Shard3PCCommitment(const ResDBConfig& config,
                     MessageManager* message_manager,
                     ReplicaCommunicator* replica_communicator,
                     ShardCommunicator* shard_communicator,
                     const ShardMetadata* shard_metadata,
                     SignatureVerifier* verifier,
                     // Called after global 3PC commits so the manager can
                     // hand the request into local shard PBFT instead of
                     // executing it directly in the global 3PC layer.
                     std::function<int(const Request&)> global_commit_handler =
                         nullptr);

 protected:
  // Override message broadcasting and sending to use ShardCommunicator for shard-aware message sending.
  int BroadcastConsensusMsg(const google::protobuf::Message& msg) override;
  int SendConsensusMsgToReplica(const google::protobuf::Message& msg,
                                int64_t node_id) override;
  // Override expected participant count to return shard count instead of total replica count.
  size_t ExpectedThreePCParticipantCount() const override;
  // Global commit is the boundary between leader-to-leader 3PC and local PBFT.
  int OnGlobalCommit(const Request& committed_request) override;

 private:
  // Shard-aware communicator for sending messages to specific nodes, shards, or shard leaders.
  ShardCommunicator* shard_communicator_;
  // Metadata containing shard configuration and node-shard mappings.
  const ShardMetadata* shard_metadata_;
  // Optional callback installed by ConsensusManagerSharded3PC. When present,
  // it starts local PBFT for the committed transaction.
  std::function<int(const Request&)> global_commit_handler_;
};

}  // namespace resdb

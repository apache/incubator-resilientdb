#pragma once

#include <cstdint>

#include "platform/consensus/ordering/pbft/commitment.h"
#include "platform/consensus/ordering/sharded_3pc/shard_communicator.h"

namespace resdb {

/**************************
 * ShardPBFTCommitment extends Commitment to implement PBFT commitment logic in a sharded deployment.
 * It overrides message broadcasting and sending to use ShardCommunicator for shard-aware message sending (i.e., local shard pbft).
 * It also includes logic to process globally committed requests from the 3PC commitment module, by broadcasting them as pre-prepare messages to the local shard's PBFT protocol.
 * - config: ResDB configuration object.
 * - message_manager: pointer to the MessageManager instance for managing consensus messages.
 * - replica_communicator: pointer to the underlying ReplicaCommunicator for sending messages.
 * - shard_communicator: pointer to the ShardCommunicator for shard-aware message sending.
 * - verifier: pointer to the SignatureVerifier for verifying message signatures.
 **************************/
class ShardPBFTCommitment : public Commitment {
 public:
  ShardPBFTCommitment(const ResDBConfig& config,
                      MessageManager* message_manager,
                      ReplicaCommunicator* replica_communicator,
                      ShardCommunicator* shard_communicator,
                      SignatureVerifier* verifier);
  // Process a globally committed request from the 3PC commitment module by broadcasting it as a pre-prepare message to the local shard's PBFT protocol.
  int ProcessGlobalCommittedRequest(const Request& committed_request);

 protected:
  // Override message broadcasting and sending to use ShardCommunicator for shard-aware message sending (i.e., local shard pbft).
  int BroadcastConsensusMsg(const google::protobuf::Message& msg) override;
  int SendConsensusMsgToReplica(const google::protobuf::Message& msg,
                                int64_t node_id) override;

 private:
  // Shard-aware communicator for sending messages to specific nodes, shards, or shard leaders.
  ShardCommunicator* shard_communicator_;
};

}  // namespace resdb

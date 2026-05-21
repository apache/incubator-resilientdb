#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "platform/consensus/ordering/pbft/commitment.h"
#include "platform/consensus/ordering/sharded_3pc/shard_communicator.h"
#include "platform/consensus/ordering/sharded_3pc/shard_metadata.h"

namespace resdb {

// ShardPOECommitment is the local-shard POE commitment module used by the
// future sharded 3PC/POE path. In Phase 2 it only routes and validates
// TYPE_POE_EXECUTE; actual execution, proof generation, cert release, and
// rollback handling are intentionally left for later phases.
class ShardPOECommitment : public Commitment {
 public:
  ShardPOECommitment(const ResDBConfig& config,
                     MessageManager* message_manager,
                     ReplicaCommunicator* replica_communicator,
                     ShardCommunicator* shard_communicator,
                     const ShardMetadata* shard_metadata,
                     SignatureVerifier* verifier);

  // Called by the sharded POE consensus manager after global 3PC reaches
  // GLOBAL_COMMIT. Only the local shard leader should turn that globally
  // committed request into a shard-local TYPE_POE_EXECUTE broadcast.
  int ProcessGlobalCommittedRequest(const Request& committed_request);

  // Validates a shard-local POE execute trigger. Phase 2 stops after
  // validation; later phases will execute optimistically and emit proofs here.
  int ProcessPOEExecuteMsg(std::unique_ptr<Context> context,
                           std::unique_ptr<Request> request);

  // Placeholder handlers for later POE phases. They exist now so dispatch and
  // build wiring can stabilize before proof/cert/rollback behavior is added.
  int ProcessPOEProofMsg(std::unique_ptr<Context> context,
                         std::unique_ptr<Request> request);
  int ProcessPOECertMsg(std::unique_ptr<Context> context,
                        std::unique_ptr<Request> request);
  int ProcessPOERollbackMsg(std::unique_ptr<Context> context,
                            std::unique_ptr<Request> request);

 protected:
  // Override the PBFT commitment routing hooks so local POE messages stay
  // inside the caller's shard instead of using flat ReplicaCommunicator
  // broadcast semantics.
  int BroadcastConsensusMsg(const google::protobuf::Message& msg) override;
  int SendConsensusMsgToReplica(const google::protobuf::Message& msg,
                                int64_t node_id) override;

 private:
  // Shared validation for TYPE_POE_EXECUTE. Keeping this separate makes the
  // later execution/proof code easier to read because all metadata checks stay
  // in one place.
  bool ValidatePOEExecuteRequest(const Context& context,
                                 const Request& request) const;

  ShardCommunicator* shard_communicator_;
  const ShardMetadata* shard_metadata_;
};

}  // namespace resdb

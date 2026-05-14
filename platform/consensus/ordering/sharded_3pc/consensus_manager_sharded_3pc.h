#pragma once

#include <memory>
#include <optional>
#include <string>

#include "executor/common/custom_query.h"
#include "executor/common/transaction_manager.h"
#include "platform/config/resdb_config.h"
#include "platform/consensus/ordering/pbft/consensus_manager_pbft.h"
#include "platform/consensus/ordering/3pc/commitment_3pc.h"
#include "platform/consensus/ordering/sharded_3pc/shard_pbft_commitment.h"
#include "platform/consensus/ordering/sharded_3pc/shard_communicator.h"
#include "platform/consensus/ordering/sharded_3pc/shard_metadata.h"

namespace resdb {
/**************************
 * ConsensusManagerSharded3PC extends ConsensusManagerPBFT to implement a sharded 3PC protocol.
 * It initializes shard-specific components such as ShardCommunicator and ShardMetadata.
 * Includes logic to determine whether a request should be processed by the local shard's PBFT commitment or the global 3PC commitment, based on the request type and shard configuration.
 * - config: ResDB configuration object.
 * - shard_config_path: file path to the shard configuration, which contains information about shard assignments and node roles.
 * - executor: transaction manager for executing committed transactions.
 ***************************/
class ConsensusManagerSharded3PC : public ConsensusManagerPBFT {
 public:
  ConsensusManagerSharded3PC(const ResDBConfig& config,
                             const std::string& shard_config_path,
                             std::unique_ptr<TransactionManager> executor);
  ~ConsensusManagerSharded3PC() override = default;

  // Override PBFT commit path. Passes 3PC requests to InternalConsensusCommit3PC.
  int ConsensusCommit(std::unique_ptr<Context> context,
                      std::unique_ptr<Request> request) override;

 private:
  // 3PC specific initated recovery module - links InternalConsensusCommit3PC to the recovery process.
  void InitRecovery3PC();

  // Internal commit acts as 3PC request state machine and dispatches to comittment_3pc.
  int InternalConsensusCommit3PC(std::unique_ptr<Context> context,
                                 std::unique_ptr<Request> request);

  // Helper to get the 3PC-specific commitment module, which contains the 3PC state machine and logic.
  Commitment3PC* GetCommitment3PC() { return commitment_3pc_.get(); }
  // Helper to get the shard-specific PBFT commitment module, which contains the PBFT state machine and logic for the local shard.
  ShardPBFTCommitment* GetShardPBFTCommitment();
  // Helper functions to determine if a request type should be processed by the global 3PC commitment or the local shard PBFT commitment.
  int StartLocalPBFTFromGlobalCommit(const Request& committed_request);
  // Helper function to determine if a client response should be enqueued based on the shard metadata and request's coordinator shard id.
  bool ShouldEnqueueClientResponse(const Request& request) const;

  std::optional<ResDBConfig> local_consensus_config_;           // Local consensus config for the shard-specific PBFT commitment.
  std::unique_ptr<Commitment3PC> commitment_3pc_;               // Commitment module for the global sharded 3PC protocol.
  std::optional<ShardMetadata> shard_metadata_;                 // Metadata about shard assignments and node roles, used for routing requests and determining processing logic.
  std::unique_ptr<ShardCommunicator> shard_communicator_;       // Communicator for shard-aware message sending, used by the commitment modules to send messages to specific nodes or shards.
};

}  // namespace resdb

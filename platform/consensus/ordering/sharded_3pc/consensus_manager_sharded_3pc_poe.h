#pragma once

#include <memory>
#include <optional>
#include <string>

#include "executor/common/transaction_manager.h"
#include "platform/config/resdb_config.h"
#include "platform/consensus/ordering/3pc/commitment_3pc.h"
#include "platform/consensus/ordering/pbft/consensus_manager_pbft.h"
#include "platform/consensus/ordering/sharded_3pc/shard_communicator.h"
#include "platform/consensus/ordering/sharded_3pc/shard_metadata.h"
#include "platform/consensus/ordering/sharded_3pc/shard_poe_commitment.h"

namespace resdb {

// ConsensusManagerSharded3PCPOE is the experimental POE variant of the
// sharded consensus manager. It keeps the existing global leader-to-leader 3PC
// path, but replaces the local shard PBFT commitment with ShardPOECommitment.
class ConsensusManagerSharded3PCPOE : public ConsensusManagerPBFT {
 public:
  ConsensusManagerSharded3PCPOE(
      const ResDBConfig& config, const std::string& shard_config_path,
      std::unique_ptr<TransactionManager> executor);
  ~ConsensusManagerSharded3PCPOE() override = default;

  int ConsensusCommit(std::unique_ptr<Context> context,
                      std::unique_ptr<Request> request) override;

 private:
  // Reuses PBFT recovery log replay but sends recovered messages through this
  // manager's global-3PC/local-POE dispatcher.
  void InitRecovery3PC();

  // Internal dispatcher for the POE variant. Global 3PC messages go to
  // commitment_3pc_, while local POE messages go to ShardPOECommitment.
  int InternalConsensusCommitPOE(std::unique_ptr<Context> context,
                                 std::unique_ptr<Request> request);
  Commitment3PC* GetCommitment3PC() { return commitment_3pc_.get(); }
  ShardPOECommitment* GetShardPOECommitment();

  // Callback installed into Shard3PCCommitment. It is invoked when global 3PC
  // commits and starts the local POE handoff for this shard.
  int StartLocalPOEFromGlobalCommit(const Request& committed_request);

  // Preserves the sharded response rule: only replicas in the coordinator shard
  // may enqueue client/proxy responses.
  bool ShouldEnqueueClientResponse(const Request& request) const;

  std::optional<ResDBConfig> local_consensus_config_;
  std::unique_ptr<Commitment3PC> commitment_3pc_;
  std::optional<ShardMetadata> shard_metadata_;
  std::unique_ptr<ShardCommunicator> shard_communicator_;
};

}  // namespace resdb

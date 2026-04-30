#pragma once

#include <memory>

#include "executor/common/custom_query.h"
#include "platform/config/resdb_config.h"
#include "platform/consensus/ordering/pbft/consensus_manager_pbft.h"
#include "platform/consensus/ordering/3pc/commitment_3pc.h"

namespace resdb {
/**************************
 * Inherits from ConsensusManagerPBFT to reuse consensus managementent scaffolding.
 * - Reuse PBFT modules such as message manager, transaction manager, response manager, etc.
 * - Replaces commit protocol with 3PC state machine handling.
 ***************************/
class ConsensusManager3PC : public ConsensusManagerPBFT {
 public:
  ConsensusManager3PC(const ResDBConfig& config,
                      std::unique_ptr<TransactionManager> executor,
                      std::unique_ptr<CustomQuery> query_executor = nullptr);
  ~ConsensusManager3PC() override = default;

  // Override PBFT commit path. Passes 3PC requests to InternalConsensusCommit3PC.
  int ConsensusCommit(std::unique_ptr<Context> context,
                      std::unique_ptr<Request> request) override;

 private:
  // 3PC specific initated recovery module - links InternalConsensusCommit3PC to the recovery process.
  void InitRecovery3PC();

  // Internal commit acts as 3PC request state machine and dispatches to comittment_3pc.
  int InternalConsensusCommit3PC(std::unique_ptr<Context> context,
                                 std::unique_ptr<Request> request);

  // Helper to get 3PC-specific commitment module.                               
  Commitment3PC* GetCommitment3PC() {
    return static_cast<Commitment3PC*>(commitment_.get());
  }
};

}  // namespace resdb

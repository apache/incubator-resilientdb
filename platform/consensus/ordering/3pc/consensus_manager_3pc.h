#pragma once

#include <memory>

#include "executor/common/custom_query.h"
#include "platform/config/resdb_config.h"
#include "platform/consensus/ordering/pbft/consensus_manager_pbft.h"
#include "platform/consensus/ordering/3pc/commitment_3pc.h"

namespace resdb {

// Reuse the PBFT consensus-manager scaffolding, but replace the actual
// consensus/commit path with a 3PC state machine.
class ConsensusManager3PC : public ConsensusManagerPBFT {
 public:
  ConsensusManager3PC(const ResDBConfig& config,
                      std::unique_ptr<TransactionManager> executor,
                      std::unique_ptr<CustomQuery> query_executor = nullptr);
  ~ConsensusManager3PC() override = default;

  int ConsensusCommit(std::unique_ptr<Context> context,
                      std::unique_ptr<Request> request) override;

 private:
  void InitRecovery3PC();
  int InternalConsensusCommit3PC(std::unique_ptr<Context> context,
                                 std::unique_ptr<Request> request);

  Commitment3PC* GetCommitment3PC() {
    return static_cast<Commitment3PC*>(commitment_.get());
  }
};

}  // namespace resdb

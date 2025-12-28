#pragma once

#include "platform/config/resdb_poc_config.h"
#include "platform/consensus/ordering/poc/pow/pow_manager.h"
#include "platform/consensus/ordering/poc/pow/miner_manager.h"
#include "platform/consensus/ordering/common/framework/consensus.h"
#include "platform/networkstrate/consensus_manager.h"

namespace resdb {

class ConsensusServicePoW : public common::Consensus {
 public:
  ConsensusServicePoW(const ResDBPoCConfig& config);
  virtual ~ConsensusServicePoW();

  // Start the service.
  void Start() override;

  int ProcessCustomConsensus(std::unique_ptr<Request> request) override;
  std::vector<ReplicaInfo> GetReplicas() override;

 protected:
  std::unique_ptr<PoWManager> pow_manager_;
  std::unique_ptr<MinerManager> miner_manager_;
};

}  // namespace resdb

#pragma once

#include "execution/geo_transaction_executor.h"
#include "ordering/geo_pbft/geo_pbft_commitment.h"
#include "ordering/pbft/consensus_service_pbft.h"

namespace resdb {

class ConsensusServiceGeoPBFT : public ConsensusServicePBFT {
 public:
  ConsensusServiceGeoPBFT(
      const ResDBConfig& config,
      std::unique_ptr<GeoTransactionExecutor> local_executor,
      std::unique_ptr<GeoGlobalExecutor> global_executor);
  virtual ~ConsensusServiceGeoPBFT() = default;

  int ConsensusCommit(std::unique_ptr<Context> context,
                      std::unique_ptr<Request> request) override;

 private:
  std::unique_ptr<GeoPBFTCommitment> commitment_;
};

}  // namespace resdb

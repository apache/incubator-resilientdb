#include "ordering/geo_pbft/consensus_service_geo_pbft.h"

namespace resdb {

ConsensusServiceGeoPBFT::ConsensusServiceGeoPBFT(
    const ResDBConfig& config,
    std::unique_ptr<GeoTransactionExecutor> local_executor,
    std::unique_ptr<GeoGlobalExecutor> global_executor)
    : ConsensusServicePBFT(config, std::move(local_executor)) {
  commitment_ = std::make_unique<GeoPBFTCommitment>(
      std::move(global_executor), config, std::make_unique<SystemInfo>(config),
      GetBroadCastClient());
}

int ConsensusServiceGeoPBFT::ConsensusCommit(std::unique_ptr<Context> context,
                                             std::unique_ptr<Request> request) {
  LOG(ERROR) << "recv impl type:" << request->type() << " "
             << "sender id:" << request->sender_id();
  switch (request->type()) {
    case Request::TYPE_GEO_REQUEST:
      return commitment_->GeoProcessCcm(std::move(context), std::move(request));
  }
  return ConsensusServicePBFT::ConsensusCommit(std::move(context),
                                               std::move(request));
}

}  // namespace resdb

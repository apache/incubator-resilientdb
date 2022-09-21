#include "ordering/geo_pbft/geo_pbft_commitment.h"

#include <glog/logging.h>

namespace resdb {

GeoPBFTCommitment::GeoPBFTCommitment(
    std::unique_ptr<GeoGlobalExecutor> global_executor,
    const ResDBConfig& config, std::unique_ptr<SystemInfo> system_info,
    ResDBReplicaClient* replica_client)
    : global_executor_(std::move(global_executor)),
      config_(std::move(config)),
      system_info_(std::move(system_info)),
      replica_client_(std::move(replica_client)) {}

int GeoPBFTCommitment::GeoProcessCcm(std::unique_ptr<Context> context,
                                     std::unique_ptr<Request> request) {
  if (ccm_checklist_.exists(request->hash())) {
    LOG(ERROR)
        << "[GeoProcessCcm] geo_request already received, from sender id: "
        << request->sender_id() << " ,no more action needed.";
    return 1;
  }
  // put GEO_REQ into checklist
  ccm_checklist_.add(request->hash());
  LOG(ERROR) << "[GeoProcessCcm] add to checklist, from sender id: "
             << request->sender_id();

  // if the request comes from another region, do local broadcast
  ResConfigData config_data = config_.GetConfigData();
  int self_region_id = config_data.self_region_id();
  if (request->region_info().region_id() != self_region_id) {
    request->mutable_region_info()->set_region_id(self_region_id);
    LOG(ERROR) << "[GeoProcessCcm] start broadcasting geo_request locally... ";
    replica_client_->BroadCast(*request);
  }
  LOG(ERROR) << "[GeoProcessCcm] start to execute the transacition in global "
                "executor...";
  return global_executor_->Execute(std::move(request));
}

}  // namespace resdb

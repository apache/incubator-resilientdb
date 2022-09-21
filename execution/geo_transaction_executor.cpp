#include "execution/geo_transaction_executor.h"

#include <glog/logging.h>

#include "ordering/pbft/transaction_utils.h"

namespace resdb {

GeoTransactionExecutor::GeoTransactionExecutor(
    const ResDBConfig& config, std::unique_ptr<SystemInfo> system_info,
    std::unique_ptr<ResDBReplicaClient> replica_client,
    std::unique_ptr<TransactionExecutorImpl> geo_executor_impl)
    : config_(config),
      system_info_(std::move(system_info)),
      replica_client_(std::move(replica_client)),
      geo_executor_impl_(std::move(geo_executor_impl)) {}

std::unique_ptr<BatchClientResponse> GeoTransactionExecutor::ExecuteBatch(
    const BatchClientRequest& request) {
  LOG(ERROR) << "get request:" << request.DebugString();

  ResConfigData config_data = config_.GetConfigData();

  std::unique_ptr<Request> geo_request = resdb::NewRequest(
      Request::TYPE_GEO_REQUEST, Request(), config_.GetSelfInfo().id(),
      config_data.self_region_id());

  request.SerializeToString(geo_request->mutable_data());

  // Only for primary node: send out GEO_REQUEST to other regions.
  if (config_.GetSelfInfo().id() == system_info_->GetPrimaryId()) {
    for (const auto& region : config_data.region()) {
      if (region.region_id() == config_data.self_region_id()) {
        continue;
      }
      // maximum number of faulty replicas in this region
      int max_faulty = (region.replica_info_size() - 1) / 3;
      int num_request_sent = 0;
      for (const auto& replica : region.replica_info()) {
        // send to f + 1 replicas in the region
        if (num_request_sent > max_faulty) {
          break;
        }
        int ret = replica_client_->SendMessage(*geo_request, replica);
        LOG(ERROR) << "[Primary Sending GEO_REQ to other regions] send to ("
                   << replica.ip() << ":" << replica.port() << ") ret:" << ret;
        if (ret >= 0) {
          num_request_sent++;
        }
      }
    }
  }

  return geo_executor_impl_->ExecuteBatch(request);
}

}  // namespace resdb

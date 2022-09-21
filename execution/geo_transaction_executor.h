#pragma once
#include "config/resdb_config.h"
#include "execution/system_info.h"
#include "execution/transaction_executor_impl.h"
#include "server/resdb_replica_client.h"

namespace resdb {

class GeoTransactionExecutor : public TransactionExecutorImpl {
 public:
  GeoTransactionExecutor(
      const ResDBConfig& config, std::unique_ptr<SystemInfo> system_info,
      std::unique_ptr<ResDBReplicaClient> replica_client,
      std::unique_ptr<TransactionExecutorImpl> geo_executor_impl);

  virtual std::unique_ptr<BatchClientResponse> ExecuteBatch(
      const BatchClientRequest& request);

 protected:
  ResDBConfig config_;
  std::unique_ptr<ResDBReplicaClient> replica_client_;
  std::unique_ptr<TransactionExecutorImpl> geo_executor_impl_ = nullptr;
  std::unique_ptr<SystemInfo> system_info_;
};

}  // namespace resdb

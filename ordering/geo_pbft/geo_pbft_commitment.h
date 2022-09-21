#pragma once

#include "config/resdb_config.h"
#include "execution/geo_global_executor.h"
#include "execution/system_info.h"
#include "hash_set.h"
#include "proto/resdb.pb.h"
#include "server/resdb_replica_client.h"
#include "server/server_comm.h"

namespace resdb {

class GeoPBFTCommitment {
 public:
  GeoPBFTCommitment(std::unique_ptr<GeoGlobalExecutor> global_executor,
                    const ResDBConfig& config,
                    std::unique_ptr<SystemInfo> system_info_,
                    ResDBReplicaClient* replica_client_);

  int GeoProcessCcm(std::unique_ptr<Context> context,
                    std::unique_ptr<Request> request);

 private:
  std::unique_ptr<GeoGlobalExecutor> global_executor_;
  SpinLockSet<std::string> ccm_checklist_;
  ResDBConfig config_;
  std::unique_ptr<SystemInfo> system_info_ = nullptr;
  ResDBReplicaClient* replica_client_;
};

}  // namespace resdb

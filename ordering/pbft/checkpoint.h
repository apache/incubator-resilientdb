#pragma once

#include "config/resdb_config.h"
#include "ordering/pbft/checkpoint_collector.h"
#include "ordering/status/checkpoint/check_point_info.h"
#include "server/resdb_replica_client.h"

namespace resdb {

class CheckPoint {
 public:
  CheckPoint(const ResDBConfig& config, ResDBReplicaClient* replica_client);
  virtual ~CheckPoint();

  virtual int ProcessCheckPoint(std::unique_ptr<Context> context,
                                std::unique_ptr<Request> request);

  CheckPointInfo* GetCheckPointInfo();

 private:
  void UpdateCheckPointStatus();

 protected:
  ResDBConfig config_;
  ResDBReplicaClient* replica_client_;
  std::unique_ptr<CheckPointInfo> checkpoint_info_;
  std::unique_ptr<CheckPointCollector> checkpoint_collector_;
  std::thread checkpoint_thread_;
  std::atomic<bool> stop_;
};

}  // namespace resdb

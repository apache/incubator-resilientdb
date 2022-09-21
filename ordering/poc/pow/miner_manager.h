#pragma once

#include "config/resdb_poc_config.h"

namespace resdb {

class MinerManager {
 public:
  MinerManager(const ResDBPoCConfig& config);

  std::vector<ReplicaInfo> GetReplicas();

 private:
  ResDBPoCConfig config_;
  std::vector<ReplicaInfo> replicas_;
};

}  // namespace resdb

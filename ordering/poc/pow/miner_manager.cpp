#include "ordering/poc/pow/miner_manager.h"

#include <assert.h>
#include <glog/logging.h>

namespace resdb {

MinerManager::MinerManager(const ResDBPoCConfig& config) : config_(config) {}

std::vector<ReplicaInfo> MinerManager::GetReplicas() { return replicas_; }

}  // namespace resdb

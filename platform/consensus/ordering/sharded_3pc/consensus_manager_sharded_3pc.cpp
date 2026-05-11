#include "platform/consensus/ordering/sharded_3pc/consensus_manager_sharded_3pc.h"

#include <glog/logging.h>

namespace resdb {

ConsensusManagerSharded3PC::ConsensusManagerSharded3PC(
    const ResDBConfig& config, const std::string& shard_config_path,
    std::unique_ptr<TransactionManager> executor)
    : ConsensusManagerPBFT(config, std::move(executor)) {
  if (!IsSelfServerReplica(config)) {
    LOG(ERROR) << "Sharded 3PC placeholder initialized for client/proxy node:"
               << config.GetSelfInfo().id();
    return;
  }

  shard_metadata_.emplace(shard_config_path, config.GetSelfInfo().id());
  LOG(ERROR) << "Sharded 3PC placeholder initialized. node:"
             << shard_metadata_->SelfNodeId()
             << " shard:" << shard_metadata_->SelfShardId()
             << " shard leader:" << shard_metadata_->IsSelfShardLeader();
}

bool ConsensusManagerSharded3PC::IsSelfServerReplica(
    const ResDBConfig& config) const {
  const int64_t self_node_id = config.GetSelfInfo().id();
  for (const auto& replica : config.GetReplicaInfos()) {
    if (replica.id() == self_node_id) {
      return true;
    }
  }
  return false;
}

}  // namespace resdb

#pragma once

#include <memory>
#include <optional>
#include <string>

#include "executor/common/transaction_manager.h"
#include "platform/config/resdb_config.h"
#include "platform/consensus/ordering/pbft/consensus_manager_pbft.h"
#include "platform/consensus/ordering/sharded_3pc/shard_metadata.h"

namespace resdb {

class ConsensusManagerSharded3PC : public ConsensusManagerPBFT {
 public:
  ConsensusManagerSharded3PC(const ResDBConfig& config,
                             const std::string& shard_config_path,
                             std::unique_ptr<TransactionManager> executor);

 private:
  bool IsSelfServerReplica(const ResDBConfig& config) const;

  std::optional<ShardMetadata> shard_metadata_;
};

}  // namespace resdb

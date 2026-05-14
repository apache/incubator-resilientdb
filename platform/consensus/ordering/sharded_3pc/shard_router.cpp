#include "platform/consensus/ordering/sharded_3pc/shard_router.h"

#include <stdexcept>

namespace resdb {

ShardRouter::ShardRouter(const ShardMetadata* shard_metadata)
    : shard_metadata_(shard_metadata) {
  if (shard_metadata_ == nullptr) {
    throw std::invalid_argument("ShardRouter requires shard metadata");
  }
  shard_ids_ = shard_metadata_->AllShardIds();
  if (shard_ids_.empty()) {
    throw std::invalid_argument("ShardRouter requires at least one shard");
  }
}

ShardRoute ShardRouter::NextShard() {
  // Lock the mutex to ensure thread-safe access to next_index_ when determining the next shard route.
  std::lock_guard<std::mutex> lk(mutex_);
  const uint32_t shard_id = shard_ids_[next_index_];
  // Move to the next shard in round-robin fashion for the next call.
  next_index_ = (next_index_ + 1) % shard_ids_.size();
  return ShardRoute{shard_id, shard_metadata_->LeaderForShard(shard_id)};
}

}  // namespace resdb

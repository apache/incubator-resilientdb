#pragma once

#include <cstdint>
#include <mutex>
#include <vector>

#include "platform/consensus/ordering/sharded_3pc/shard_metadata.h"

namespace resdb {

// Simple struct to represent a shard route, containing the shard ID and its leader's node ID.
struct ShardRoute {
  uint32_t shard_id = 0;
  int64_t leader_id = 0;
};

/**************************
 * ShardRouter provides a simple round-robin routing mechanism to determine the next shard to route a request to, based on the shard metadata.
 * It maintains an internal index to track the next shard in the list of shard IDs, and provides thread-safe access to get the next shard route.
 * - shard_metadata: metadata containing shard configuration and node-shard mappings.
 **************************/
class ShardRouter {
 public:
  explicit ShardRouter(const ShardMetadata* shard_metadata);

  // Get the next shard route in a round-robin fashion. Returns the shard ID and leader ID for the next shard to route to.
  ShardRoute NextShard();

 private:
  const ShardMetadata* shard_metadata_;
  std::vector<uint32_t> shard_ids_;             // List of shard IDs for routing.
  size_t next_index_ = 0;                       // Index to track the next shard in the round-robin rotation.
  std::mutex mutex_;                            // Mutex to protect access to next_index_ for thread safety in batch operations.
};

}  // namespace resdb

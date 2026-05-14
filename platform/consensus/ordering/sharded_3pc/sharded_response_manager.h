#pragma once

#include <map>
#include <mutex>

#include "platform/consensus/ordering/pbft/response_manager.h"
#include "platform/consensus/ordering/sharded_3pc/shard_router.h"

namespace resdb {

/**************************
 * ShardedResponseManager extends ResponseManager to provide shard-aware response handling for a sharded system.
 * It uses the ShardRouter to determine the appropriate shard for each request and manages routing information for local nodes.
 * Routing rotates through shard leaders and mom-leader shard metadata is limited to local replicas.
 * - config: ResDB configuration object.
 * - replica_communicator: pointer to the underlying ReplicaCommunicator for sending messages.
 * - system_info: pointer to the SystemInfo for accessing replica information and view state.
 * - verifier: pointer to the SignatureVerifier for verifying message signatures.
 * - shard_metadata: pointer to the ShardMetadata containing shard configuration and node-shard mappings
 **************************/
class ShardedResponseManager : public ResponseManager {
 public:
  ShardedResponseManager(const ResDBConfig& config,
                         ReplicaCommunicator* replica_communicator,
                         SystemInfo* system_info, SignatureVerifier* verifier,
                         const ShardMetadata* shard_metadata);

 protected:
  // Override consensus status change logic to route requests to the appropriate shard based on local_id and shard metadata, 
  //   and to determine if consensus can progress based on responses received from the relevant shard.    
  bool MayConsensusChangeStatus(
      int type, uint64_t local_id, int received_count,
      std::atomic<TransactionStatue>* status) override;
  // Override to determine the target replica for a request based on its local_id and shard metadata.
  int64_t GetRequestTarget(Request* request, uint64_t local_id) override;
  // Override to resend requests to the appropriate shard leader on timeout.
  void ResendRequestOnTimeout(const Request& request) override;

 private:
  // Helper to determine the number of required responses for a given shard based on its size and the configured fault tolerance.
  int RequiredResponsesForShard(uint32_t shard_id) const;
  // Helper to determine the shard route for a given local_id, which may involve looking up existing routing information or determining a new route using the ShardRouter.
  bool RouteForLocalId(uint64_t local_id, ShardRoute* route) const;

  const ShardMetadata* shard_metadata_;
  ShardRouter shard_router_;
  mutable std::mutex mutex_;                            // Mutex to protect access to route_by_local_id_ for thread safety.
  std::map<uint64_t, ShardRoute> route_by_local_id_;    // Mapping of local_id to ShardRoute for routing requests to the appropriate shard based on their local_id.
};

}  // namespace resdb

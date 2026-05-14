#include "platform/consensus/ordering/sharded_3pc/sharded_response_manager.h"

#include <algorithm>
#include <stdexcept>

namespace resdb {

ShardedResponseManager::ShardedResponseManager(
    const ResDBConfig& config, ReplicaCommunicator* replica_communicator,
    SystemInfo* system_info, SignatureVerifier* verifier,
    const ShardMetadata* shard_metadata)
    : ResponseManager(config, replica_communicator, system_info, verifier,
                      /*defer_user_req_thread=*/true),
      shard_metadata_(shard_metadata),
      shard_router_(shard_metadata) {
  if (shard_metadata_ == nullptr) {
    throw std::invalid_argument("ShardedResponseManager requires metadata");
  }
  // Start the user request thread after initializing the shard router and metadata, since the routing logic may depend on this information.
  if (ShouldRunUserRequestThread()) {
    StartUserRequestThread();
  }
}

int64_t ShardedResponseManager::GetRequestTarget(Request* request,
                                                 uint64_t local_id) {
  // Determine the shard route for this request and store it in the route_by_local_id_ map for future reference (e.g., when processing responses or handling timeouts).
  ShardRoute route = shard_router_.NextShard();
  {
    std::lock_guard<std::mutex> lk(mutex_);
    route_by_local_id_[local_id] = route;
  }
  // Update the request with routing information so that it can be properly handled by the target shard and its leader.
  if (request != nullptr) {
    request->set_coordinator_shard_id(route.shard_id);
    request->set_global_coordinator_id(route.leader_id);
    request->set_global_txn_id(local_id);
    request->set_is_global_3pc(true);
  }
  return route.leader_id;
}

bool ShardedResponseManager::MayConsensusChangeStatus(
    int type, uint64_t local_id, int received_count,
    std::atomic<TransactionStatue>* status) {
  if (type != Request::TYPE_RESPONSE) {
    return false;
  }
  // Check if we have received enough responses from the relevant shard to move to the next consensus status. This involves looking up the shard route for this local_id and determining the required number of responses based on the shard size and fault tolerance.
  ShardRoute route;
  if (!RouteForLocalId(local_id, &route)) {
    return false;
  }
  if (*status == TransactionStatue::None &&
      RequiredResponsesForShard(route.shard_id) <= received_count) {
    TransactionStatue old_status = TransactionStatue::None;
    return status->compare_exchange_strong(
        old_status, TransactionStatue::EXECUTED, std::memory_order_acq_rel,
        std::memory_order_acq_rel);
  }
  return false;
}

void ShardedResponseManager::ResendRequestOnTimeout(const Request& request) {
  ShardRoute route;
  // Resend the request based on stored shard route.
  if (RouteForLocalId(request.global_txn_id(), &route)) {
    replica_communicator_->SendMessage(request, route.leader_id);
  }
}

int ShardedResponseManager::RequiredResponsesForShard(uint32_t shard_id) const {
  const size_t shard_size = shard_metadata_->ReplicasForShard(shard_id).size();
  const int f = static_cast<int>((shard_size - 1) / 3);
  return std::max(f + 1, 1);
}

bool ShardedResponseManager::RouteForLocalId(uint64_t local_id,
                                             ShardRoute* route) const {
  std::lock_guard<std::mutex> lk(mutex_);
  auto it = route_by_local_id_.find(local_id);
  if (it == route_by_local_id_.end()) {
    return false;
  }
  if (route != nullptr) {
    *route = it->second;
  }
  return true;
}

}  // namespace resdb

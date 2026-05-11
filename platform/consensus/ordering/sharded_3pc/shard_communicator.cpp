#include "platform/consensus/ordering/sharded_3pc/shard_communicator.h"

#include <limits>
#include <stdexcept>

namespace resdb {

ShardCommunicator::ShardCommunicator(
    ReplicaCommunicator* replica_communicator,
    const ShardMetadata* shard_metadata,
    const std::vector<ReplicaInfo>& replicas)
    : replica_communicator_(replica_communicator),
      shard_metadata_(shard_metadata) {
  if (replica_communicator_ == nullptr) {
    throw std::invalid_argument("ShardCommunicator requires communicator");
  }
  if (shard_metadata_ == nullptr) {
    throw std::invalid_argument("ShardCommunicator requires shard metadata");
  }

  for (const auto& replica : replicas) {
    if (replica.id() < 0 ||
        replica.id() > std::numeric_limits<uint32_t>::max()) {
      throw std::invalid_argument("replica id is outside uint32_t range");
    }
    const uint32_t node_id = static_cast<uint32_t>(replica.id());
    if (!replicas_by_node_id_.emplace(node_id, replica).second) {
      throw std::invalid_argument("duplicate replica id " +
                                  std::to_string(node_id));
    }
  }
}

int ShardCommunicator::SendToNodes(const google::protobuf::Message& message,
                                   const std::vector<uint32_t>& node_ids,
                                   bool include_self) {
  int success_count = 0;
  for (const uint32_t node_id : node_ids) {
    if (!include_self && node_id == shard_metadata_->SelfNodeId()) {
      continue;
    }
    success_count += SendToNode(message, node_id);
  }
  return success_count;
}

int ShardCommunicator::SendToNode(const google::protobuf::Message& message,
                                  uint32_t node_id) {
  const int ret = replica_communicator_->SendMessage(message,
                                                     ReplicaForNode(node_id));
  return ret >= 0 ? 1 : 0;
}

int ShardCommunicator::BroadcastToShard(
    const google::protobuf::Message& message, uint32_t shard_id,
    bool include_self) {
  return SendToNodes(message, shard_metadata_->ReplicasForShard(shard_id),
                     include_self);
}

int ShardCommunicator::BroadcastToLocalShard(
    const google::protobuf::Message& message, bool include_self) {
  return SendToNodes(message, shard_metadata_->LocalShardReplicas(),
                     include_self);
}

int ShardCommunicator::BroadcastToShardLeaders(
    const google::protobuf::Message& message, bool include_self) {
  return SendToNodes(message, shard_metadata_->AllShardLeaders(),
                     include_self);
}

int ShardCommunicator::SendToShardLeader(
    const google::protobuf::Message& message, uint32_t shard_id) {
  return SendToNode(message, shard_metadata_->LeaderForShard(shard_id));
}

const ReplicaInfo& ShardCommunicator::ReplicaForNode(uint32_t node_id) const {
  const auto it = replicas_by_node_id_.find(node_id);
  if (it == replicas_by_node_id_.end()) {
    throw std::invalid_argument("unknown replica id " +
                                std::to_string(node_id));
  }
  return it->second;
}


}  // namespace resdb

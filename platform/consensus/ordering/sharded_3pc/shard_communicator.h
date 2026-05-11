#pragma once

#include <cstdint>
#include <map>
#include <vector>

#include "platform/consensus/ordering/sharded_3pc/shard_metadata.h"
#include "platform/networkstrate/replica_communicator.h"
#include "platform/proto/replica_info.pb.h"

namespace resdb {

/**************************
 * ShardCommunicator provides an abstraction layer for sending messages to specific nodes, shards, or shard leaders based on the shard metadata. 
 * It uses the underlying ReplicaCommunicator to send messages to the appropriate recipients as determined by the ShardMetadata.
 * - replica_communicator: instance of ReplicaCommunicator.
 * - shard_metadata: metadata object containing shard configuration and node-shard mappings.
 * - replicas_by_node_id: a mapping of node IDs to their corresponding ReplicaInfo object.
 **************************/
class ShardCommunicator {
 public:
  ShardCommunicator(ReplicaCommunicator* replica_communicator,
                    const ShardMetadata* shard_metadata,
                    const std::vector<ReplicaInfo>& replicas);

  // Methods to send messages to specific nodes, shards, or shard leaders.
  int SendToNode(const google::protobuf::Message& message, uint32_t node_id);
  int BroadcastToShard(const google::protobuf::Message& message,
                       uint32_t shard_id, bool include_self = true);
  int BroadcastToLocalShard(const google::protobuf::Message& message,
                            bool include_self = true);
  int BroadcastToShardLeaders(const google::protobuf::Message& message,
                              bool include_self = true);
  int SendToShardLeader(const google::protobuf::Message& message,
                        uint32_t shard_id);

 private:
  // Helper methods to get ReplicaInfo for a given node ID and to send messages to a list of node IDs.
  const ReplicaInfo& ReplicaForNode(uint32_t node_id) const;
  int SendToNodes(const google::protobuf::Message& message,
                  const std::vector<uint32_t>& node_ids, bool include_self);

  ReplicaCommunicator* replica_communicator_;
  const ShardMetadata* shard_metadata_;
  std::map<uint32_t, ReplicaInfo> replicas_by_node_id_;
};

}  // namespace resdb

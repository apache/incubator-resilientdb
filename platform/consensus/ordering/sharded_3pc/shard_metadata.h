#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace resdb {

/**************************
 * ShardMetadata is responsible for parsing and storing the shard configuration, which includes the mapping of nodes to shards, 
 *    shard leaders, and client nodes. It provides helper methods to query this metadata, such as determining which shard a node belongs to, 
 *    who the leader of a shard is, and which nodes are in the same shard.
 * - shard_config_path: path to the shard configuration file.
 * - self_node_id: the node ID of the current node, used to determine its shard and role.
 **************************/
class ShardMetadata {
 public:
  ShardMetadata(const std::string& shard_config_path, uint32_t self_node_id);

  // Return node or shard ID.
  uint32_t SelfNodeId() const;
  uint32_t SelfShardId() const;
  // Check if node is shard leader.
  bool IsShardLeader(uint32_t node_id) const;
  bool IsSelfShardLeader() const;

  // Retreive shard leader for a given shard, shard for a given node, all shard ids, all shard leaders, replicas for a given shard, and local shard replicas.
  uint32_t LeaderForShard(uint32_t shard_id) const;
  uint32_t ShardForNode(uint32_t node_id) const;
  std::vector<uint32_t> AllShardIds() const;
  std::vector<uint32_t> AllShardLeaders() const;
  std::vector<uint32_t> ReplicasForShard(uint32_t shard_id) const;
  std::vector<uint32_t> LocalShardReplicas() const;

  // Retreive shard membership information, such as number of shards, size of local shard, whether two nodes are in the same shard, and whether a node is in the local shard.
  size_t NumShards() const;
  size_t LocalShardSize() const;
  bool SameShard(uint32_t a, uint32_t b) const;
  bool IsLocalShardReplica(uint32_t node_id) const;

 private:
  // Internal struct to hold shard information parsed from the config file.
  struct ShardInfo {
    uint32_t shard_id = 0;
    uint32_t leader_id = 0;
    std::vector<uint32_t> replica_ids;
  };

  // Helper methods to parse the shard configuration file and validate the configuration.
  void ParseConfig(const std::string& path);
  void Validate();

  uint32_t self_node_id_;
  uint32_t self_shard_id_ = 0;

  // Mappings of shard and node information. Shard ID to ShardInfo, node ID to shard ID, shard ID to leader ID, set of shard leaders, and list of client node IDs.
  std::map<uint32_t, ShardInfo> shards_by_id_;
  std::map<uint32_t, uint32_t> shard_for_node_;
  std::map<uint32_t, uint32_t> leader_for_shard_;
  std::set<uint32_t> shard_leaders_;
  std::vector<uint32_t> client_ids_;
};

}  // namespace resdb

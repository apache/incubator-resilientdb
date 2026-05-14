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
  ShardMetadata(const std::string& shard_config_path, int64_t self_node_id);

  // Return node or shard ID.
  int64_t SelfNodeId() const;
  uint32_t SelfShardId() const;
  // Return role information about the self node, such as whether it's a client, a server replica, or has a local shard (check if client).
  bool IsSelfClient() const;
  bool IsSelfServerReplica() const;
  bool HasLocalShard() const;
  // Check if node is shard leader.
  bool IsShardLeader(int64_t node_id) const;
  bool IsSelfShardLeader() const;

  // Retreive shard leader for a given shard, shard for a given node, all shard ids, all shard leaders, replicas for a given shard, and local shard replicas.
  int64_t LeaderForShard(uint32_t shard_id) const;
  uint32_t ShardForNode(int64_t node_id) const;
  std::vector<uint32_t> AllShardIds() const;
  std::vector<int64_t> AllShardLeaders() const;
  std::vector<int64_t> ReplicasForShard(uint32_t shard_id) const;
  std::vector<int64_t> LocalShardReplicas() const;

  // Retreive shard membership information, such as number of shards, size of local shard, whether two nodes are in the same shard, and whether a node is in the local shard.
  size_t NumShards() const;
  size_t LocalShardSize() const;
  bool SameShard(int64_t a, int64_t b) const;
  bool IsLocalShardReplica(int64_t node_id) const;

 private:
  // Internal struct to hold shard information parsed from the config file.
  struct ShardInfo {
    uint32_t shard_id = 0;
    int64_t leader_id = 0;
    std::vector<int64_t> replica_ids;
  };

  // Helper methods to parse the shard configuration file and validate the configuration.
  void ParseConfig(const std::string& path);
  void Validate();

  int64_t self_node_id_;
  uint32_t self_shard_id_ = 0;
  bool self_is_client_ = false; 
  bool self_is_server_replica_ = false;

  // Mappings of shard and node information. Shard ID to ShardInfo, node ID to shard ID, shard ID to leader ID, set of shard leaders, and list of client node IDs.
  std::map<uint32_t, ShardInfo> shards_by_id_;
  std::map<int64_t, uint32_t> shard_for_node_;
  std::map<uint32_t, int64_t> leader_for_shard_;
  std::set<int64_t> shard_leaders_;
  std::vector<int64_t> client_ids_;
};

}  // namespace resdb

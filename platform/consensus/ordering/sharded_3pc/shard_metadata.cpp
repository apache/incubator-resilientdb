#include "platform/consensus/ordering/sharded_3pc/shard_metadata.h"

#include <algorithm>
#include <fstream>
#include <limits>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace resdb {

namespace {
using json = nlohmann::json;

uint32_t ReadShardId(const json& value, const std::string& field_name) {
  // Shard ids are local topology labels, so keep them bounded to uint32_t even
  // though ResilientDB node ids use int64_t.
  uint64_t id = 0;
  try {
    if (value.is_number_unsigned()) {
      id = value.get<uint64_t>();
    } else if (value.is_number_integer()) {
      const int64_t signed_id = value.get<int64_t>();
      if (signed_id < 0) {
        throw std::invalid_argument(field_name + " must be non-negative");
      }
      id = static_cast<uint64_t>(signed_id);
    } else {
      throw std::invalid_argument(field_name + " must be an integer");
    }
  } catch (const json::exception& e) {
    throw std::invalid_argument(field_name + " must be an integer: " +
                                e.what());
  }

  if (id > std::numeric_limits<uint32_t>::max()) {
    throw std::invalid_argument(field_name + " exceeds uint32_t range");
  }
  return static_cast<uint32_t>(id);
}

int64_t ReadNodeId(const json& value, const std::string& field_name) {
  // ReplicaInfo.id is int64 in ResilientDB. Reject negative ids so the shard
  // config cannot describe nodes that normal generated configs never create.
  try {
    if (value.is_number_unsigned()) {
      const uint64_t id = value.get<uint64_t>();
      if (id > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
        throw std::invalid_argument(field_name + " exceeds int64_t range");
      }
      return static_cast<int64_t>(id);
    }
    if (value.is_number_integer()) {
      const int64_t id = value.get<int64_t>();
      if (id < 0) {
        throw std::invalid_argument(field_name + " must be non-negative");
      }
      return id;
    }
  } catch (const json::exception& e) {
    throw std::invalid_argument(field_name + " must be an integer: " +
                                e.what());
  }

  throw std::invalid_argument(field_name + " must be an integer");
}

uint32_t RequiredShardId(const json& object, const std::string& field_name,
                         const std::string& context) {
  if (!object.contains(field_name)) {
    throw std::invalid_argument(context + " missing required field " +
                                field_name);
  }
  return ReadShardId(object.at(field_name), context + "." + field_name);
}

int64_t RequiredNodeId(const json& object, const std::string& field_name,
                       const std::string& context) {
  if (!object.contains(field_name)) {
    throw std::invalid_argument(context + " missing required field " +
                                field_name);
  }
  return ReadNodeId(object.at(field_name), context + "." + field_name);
}

bool ContainsId(const std::vector<int64_t>& ids, int64_t target) {
  return std::find(ids.begin(), ids.end(), target) != ids.end();
}

}  // namespace

ShardMetadata::ShardMetadata(const std::string& shard_config_path,
                             int64_t self_node_id)
    : self_node_id_(self_node_id) {
  ParseConfig(shard_config_path);
  Validate();
}

int64_t ShardMetadata::SelfNodeId() const { return self_node_id_; }

uint32_t ShardMetadata::SelfShardId() const {
  if (!HasLocalShard()) {
    throw std::invalid_argument("self node has no local shard");
  }
  return self_shard_id_;
}

bool ShardMetadata::IsSelfClient() const { return self_is_client_; }

bool ShardMetadata::IsSelfServerReplica() const {
  return self_is_server_replica_;
}

bool ShardMetadata::HasLocalShard() const { return self_is_server_replica_; }

bool ShardMetadata::IsShardLeader(int64_t node_id) const {
  return shard_leaders_.find(node_id) != shard_leaders_.end();
}

bool ShardMetadata::IsSelfShardLeader() const {
  return IsShardLeader(self_node_id_);
}

int64_t ShardMetadata::LeaderForShard(uint32_t shard_id) const {
  const auto it = leader_for_shard_.find(shard_id);
  if (it == leader_for_shard_.end()) {
    throw std::invalid_argument("unknown shard id " +
                                std::to_string(shard_id));
  }
  return it->second;
}

uint32_t ShardMetadata::ShardForNode(int64_t node_id) const {
  const auto it = shard_for_node_.find(node_id);
  if (it == shard_for_node_.end()) {
    throw std::invalid_argument("unknown server node id " +
                                std::to_string(node_id));
  }
  return it->second;
}

std::vector<uint32_t> ShardMetadata::AllShardIds() const {
  std::vector<uint32_t> shard_ids;
  shard_ids.reserve(shards_by_id_.size());
  for (const auto& [shard_id, _] : shards_by_id_) {
    shard_ids.push_back(shard_id);
  }
  return shard_ids;
}

std::vector<int64_t> ShardMetadata::AllShardLeaders() const {
  std::vector<int64_t> leaders;
  // Preserve shard order by looping through leader_for_shard_ instead of shard_leaders_ set.
  leaders.reserve(leader_for_shard_.size());
  for (const auto& [_, leader_id] : leader_for_shard_) {
    leaders.push_back(leader_id);
  }
  return leaders;
}

std::vector<int64_t> ShardMetadata::ReplicasForShard(
    uint32_t shard_id) const {
  const auto it = shards_by_id_.find(shard_id);
  if (it == shards_by_id_.end()) {
    throw std::invalid_argument("unknown shard id " +
                                std::to_string(shard_id));
  }
  return it->second.replica_ids;
}

std::vector<int64_t> ShardMetadata::LocalShardReplicas() const {
  if (!HasLocalShard()) {
    throw std::invalid_argument("self node has no local shard");
  }
  return ReplicasForShard(self_shard_id_);
}

size_t ShardMetadata::NumShards() const { return shards_by_id_.size(); }

size_t ShardMetadata::LocalShardSize() const {
  return LocalShardReplicas().size();
}

bool ShardMetadata::SameShard(int64_t a, int64_t b) const {
  return ShardForNode(a) == ShardForNode(b);
}

bool ShardMetadata::IsLocalShardReplica(int64_t node_id) const {
  if (!HasLocalShard()) {
    throw std::invalid_argument("self node has no local shard");
  }
  const auto it = shard_for_node_.find(node_id);
  return it != shard_for_node_.end() && it->second == self_shard_id_;
}

void ShardMetadata::ParseConfig(const std::string& path) {
  std::ifstream input(path);
  if (!input.is_open()) {
    throw std::runtime_error("failed to open shard config " + path);
  }

  json config;
  try {
    input >> config;
  } catch (const json::exception& e) {
    throw std::invalid_argument("failed to parse shard config " + path + ": " +
                                e.what());
  }

  if (!config.contains("shards") || !config.at("shards").is_array()) {
    throw std::invalid_argument("shard config missing array field shards");
  }

  // Parse shard entries first but defer cross-shard validation until Validate().
  // That lets duplicate nodes, missing self, and leader membership all be
  // checked in one pass over the normalized in-memory ShardInfo records.
  for (const auto& shard_json : config.at("shards")) {
    if (!shard_json.is_object()) {
      throw std::invalid_argument("each shard entry must be an object");
    }

    ShardInfo shard;
    const std::string context = "shard";
    shard.shard_id = RequiredShardId(shard_json, "shard_id", context);
    shard.leader_id = RequiredNodeId(shard_json, "leader_id", context);

    if (!shard_json.contains("replica_ids") ||
        !shard_json.at("replica_ids").is_array()) {
      throw std::invalid_argument("shard " + std::to_string(shard.shard_id) +
                                  " missing array field replica_ids");
    }

    std::set<int64_t> shard_replica_ids;
    for (const auto& replica_json : shard_json.at("replica_ids")) {
      const int64_t node_id =
          ReadNodeId(replica_json, "shard " + std::to_string(shard.shard_id) +
                                       ".replica_ids");
      if (!shard_replica_ids.insert(node_id).second) {
        throw std::invalid_argument("duplicate replica id " +
                                    std::to_string(node_id) + " in shard " +
                                    std::to_string(shard.shard_id));
      }
      shard.replica_ids.push_back(node_id);
    }

    // std::map gives deterministic shard order for AllShardIds(),
    // AllShardLeaders(), and round-robin proxy routing.
    if (!shards_by_id_.emplace(shard.shard_id, shard).second) {
      throw std::invalid_argument("duplicate shard id " +
                                  std::to_string(shard.shard_id));
    }
  }

  if (config.contains("client_ids")) {
    if (!config.at("client_ids").is_array()) {
      throw std::invalid_argument("client_ids must be an array when present");
    }
    // Client ids are retained so proxy nodes can construct ShardMetadata even
    // though they do not belong to any local PBFT shard.
    std::set<int64_t> seen_client_ids;
    for (const auto& client_json : config.at("client_ids")) {
      const int64_t client_id = ReadNodeId(client_json, "client_ids");
      if (!seen_client_ids.insert(client_id).second) {
        throw std::invalid_argument("duplicate client id " +
                                    std::to_string(client_id));
      }
      client_ids_.push_back(client_id);
    }
  }
}

void ShardMetadata::Validate() {
  if (shards_by_id_.empty()) {
    throw std::invalid_argument("shard config must contain at least one shard");
  }

  std::map<int64_t, uint32_t> shard_for_node;
  std::set<int64_t> leaders;
  bool self_server_found = false;

  // Build temporary lookup structures first. The permanent maps are populated
  // only after all validation passes, keeping partially valid configs from
  // leaking into accessor state.
  for (const auto& [shard_id, shard] : shards_by_id_) {
    if (shard.replica_ids.empty()) {
      throw std::invalid_argument("shard " + std::to_string(shard_id) +
                                  " must contain at least one replica");
    }
    if (!ContainsId(shard.replica_ids, shard.leader_id)) {
      throw std::invalid_argument("leader " + std::to_string(shard.leader_id) +
                                  " is not a replica in shard " +
                                  std::to_string(shard_id));
    }

    leaders.insert(shard.leader_id);
    for (const int64_t node_id : shard.replica_ids) {
      // A server replica can belong to exactly one shard. Cross-shard overlap
      // would make PBFT quorum sizing and local routing ambiguous.
      const auto [_, inserted] = shard_for_node.emplace(node_id, shard_id);
      if (!inserted) {
        throw std::invalid_argument("server node " + std::to_string(node_id) +
                                    " appears in multiple shards");
      }
      if (node_id == self_node_id_) {
        self_server_found = true;
      }
    }
  }

  if (leaders.empty()) {
    throw std::invalid_argument("shard config must contain at least one leader");
  }
  // Server replicas and client/proxy nodes are mutually exclusive roles. A
  // proxy node may parse all shards, but it must not claim a local shard.
  self_is_client_ = ContainsId(client_ids_, self_node_id_);
  if (self_server_found && self_is_client_) {
    throw std::invalid_argument("self node " + std::to_string(self_node_id_) +
                                " appears as both server and client");
  }
  if (!self_server_found && !self_is_client_) {
    throw std::invalid_argument("self node " + std::to_string(self_node_id_) +
                                " does not appear in any shard or client_ids");
  }
  self_is_server_replica_ = self_server_found;

  // Populate the permanent lookup tables used by all accessors. These maps are
  // intentionally redundant so hot routing checks do not have to scan shards.
  for (const auto& [shard_id, shard] : shards_by_id_) {
    leader_for_shard_.emplace(shard_id, shard.leader_id);
    shard_leaders_.insert(shard.leader_id);
    for (const int64_t node_id : shard.replica_ids) {
      shard_for_node_.emplace(node_id, shard_id);
    }
  }
  if (self_is_server_replica_) {
    // Only server replicas get a local shard id. Client/proxy nodes make
    // SelfShardId() and LocalShardReplicas() fail explicitly.
    self_shard_id_ = ShardForNode(self_node_id_);
  }
}

}  // namespace resdb

#include "platform/consensus/ordering/sharded_3pc/shard_metadata.h"

#include <gtest/gtest.h>
#include <unistd.h>

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace resdb {
namespace {

// Each test writes a temporary JSON topology so ShardMetadata is exercised
// through the same file-based parser used by the service entrypoint.
std::string WriteShardConfig(const std::string& contents) {
  static int file_count = 0;
  const char* test_tmpdir = std::getenv("TEST_TMPDIR");
  const std::string directory = test_tmpdir == nullptr ? "/tmp" : test_tmpdir;
  const std::string path = directory + "/shard_metadata_test_" +
                           std::to_string(getpid()) + "_" +
                           std::to_string(file_count++) + ".json";

  std::ofstream output(path);
  if (!output.is_open()) {
    throw std::runtime_error("failed to write test shard config " + path);
  }
  output << contents;
  return path;
}

std::string FourShardConfig() {
  return R"json(
{
  "shards": [
    {"shard_id": 0, "leader_id": 1, "replica_ids": [1, 2, 3, 4]},
    {"shard_id": 1, "leader_id": 5, "replica_ids": [5, 6, 7, 8]},
    {"shard_id": 2, "leader_id": 9, "replica_ids": [9, 10, 11, 12]},
    {"shard_id": 3, "leader_id": 13, "replica_ids": [13, 14, 15, 16]}
  ],
  "client_ids": [17]
}
)json";
}

TEST(ShardMetadataTest, ParsesFourShardConfig) {
  // Validates the default 4x4 topology: membership, leaders, local shard
  // helpers, and deterministic ordered accessors.
  ShardMetadata metadata(WriteShardConfig(FourShardConfig()), 6);

  EXPECT_EQ(metadata.SelfNodeId(), 6);
  EXPECT_FALSE(metadata.IsSelfClient());
  EXPECT_TRUE(metadata.IsSelfServerReplica());
  EXPECT_TRUE(metadata.HasLocalShard());
  EXPECT_EQ(metadata.SelfShardId(), 1);
  EXPECT_FALSE(metadata.IsSelfShardLeader());
  EXPECT_TRUE(metadata.IsShardLeader(1));
  EXPECT_TRUE(metadata.IsShardLeader(5));
  EXPECT_FALSE(metadata.IsShardLeader(6));
  EXPECT_EQ(metadata.LeaderForShard(2), 9);
  EXPECT_EQ(metadata.ShardForNode(14), 3);
  EXPECT_EQ(metadata.NumShards(), 4);
  EXPECT_EQ(metadata.LocalShardSize(), 4);
  EXPECT_TRUE(metadata.SameShard(5, 8));
  EXPECT_FALSE(metadata.SameShard(5, 9));
  EXPECT_TRUE(metadata.IsLocalShardReplica(8));
  EXPECT_FALSE(metadata.IsLocalShardReplica(9));

  EXPECT_EQ(metadata.AllShardIds(),
            (std::vector<uint32_t>{0, 1, 2, 3}));
  EXPECT_EQ(metadata.AllShardLeaders(),
            (std::vector<int64_t>{1, 5, 9, 13}));
  EXPECT_EQ(metadata.LocalShardReplicas(),
            (std::vector<int64_t>{5, 6, 7, 8}));
  EXPECT_EQ(metadata.ReplicasForShard(3),
            (std::vector<int64_t>{13, 14, 15, 16}));
}

TEST(ShardMetadataTest, AllowsClientSelfWithoutLocalShard) {
  // Client/proxy nodes parse the same shard config, but they should not have
  // server-only local shard state.
  ShardMetadata metadata(WriteShardConfig(FourShardConfig()), 17);

  EXPECT_EQ(metadata.SelfNodeId(), 17);
  EXPECT_TRUE(metadata.IsSelfClient());
  EXPECT_FALSE(metadata.IsSelfServerReplica());
  EXPECT_FALSE(metadata.HasLocalShard());
  EXPECT_EQ(metadata.NumShards(), 4);
  EXPECT_EQ(metadata.LeaderForShard(0), 1);
  EXPECT_EQ(metadata.AllShardLeaders(),
            (std::vector<int64_t>{1, 5, 9, 13}));
  EXPECT_THROW(metadata.SelfShardId(), std::invalid_argument);
  EXPECT_THROW(metadata.LocalShardReplicas(), std::invalid_argument);
  EXPECT_THROW(metadata.LocalShardSize(), std::invalid_argument);
  EXPECT_THROW(metadata.IsLocalShardReplica(1), std::invalid_argument);
}

TEST(ShardMetadataTest, RejectsSelfListedAsServerAndClient) {
  // A node cannot safely act as both a server replica and proxy/client in the
  // same topology.
  const std::string config = R"json(
{
  "shards": [
    {"shard_id": 0, "leader_id": 1, "replica_ids": [1, 2, 3, 4]}
  ],
  "client_ids": [1]
}
)json";

  EXPECT_THROW(ShardMetadata(WriteShardConfig(config), 1),
               std::invalid_argument);
}

TEST(ShardMetadataTest, ParsesNonUniformShardIds) {
  // Shard and replica ids do not need to be contiguous; lookups should follow
  // the config rather than a formula.
  const std::string config = R"json(
{
  "shards": [
    {"shard_id": 10, "leader_id": 101, "replica_ids": [101, 102]},
    {"shard_id": 30, "leader_id": 301, "replica_ids": [300, 301, 302]}
  ],
  "client_ids": [900, 901]
}
)json";

  ShardMetadata metadata(WriteShardConfig(config), 300);

  EXPECT_EQ(metadata.SelfShardId(), 30);
  EXPECT_EQ(metadata.LeaderForShard(10), 101);
  EXPECT_EQ(metadata.LeaderForShard(30), 301);
  EXPECT_EQ(metadata.ShardForNode(102), 10);
  EXPECT_FALSE(metadata.IsSelfShardLeader());
  EXPECT_EQ(metadata.LocalShardReplicas(),
            (std::vector<int64_t>{300, 301, 302}));
}

TEST(ShardMetadataTest, AcceptsNodeIdsAboveUint32) {
  // Node ids are int64_t to match ResilientDB ReplicaInfo.id. This guards
  // against accidentally truncating node ids to the shard-id type.
  const int64_t large_node_id =
      static_cast<int64_t>(std::numeric_limits<uint32_t>::max()) + 5;
  const std::string config = R"json(
{
  "shards": [
    {"shard_id": 1, "leader_id": )json" +
                             std::to_string(large_node_id) + R"json(,
     "replica_ids": [)json" +
                             std::to_string(large_node_id) + R"json(, 2]}
  ],
  "client_ids": [3]
}
)json";

  ShardMetadata metadata(WriteShardConfig(config), large_node_id);

  EXPECT_EQ(metadata.SelfNodeId(), large_node_id);
  EXPECT_EQ(metadata.SelfShardId(), 1);
  EXPECT_TRUE(metadata.IsSelfShardLeader());
  EXPECT_EQ(metadata.LeaderForShard(1), large_node_id);
  EXPECT_EQ(metadata.ShardForNode(large_node_id), 1);
  EXPECT_EQ(metadata.LocalShardReplicas(),
            (std::vector<int64_t>{large_node_id, 2}));
}

TEST(ShardMetadataTest, RejectsMissingShards) {
  // A shard config without the shards array cannot define server membership.
  EXPECT_THROW(ShardMetadata(WriteShardConfig(R"json({"client_ids": [5]})json"),
                             1),
               std::invalid_argument);
}

TEST(ShardMetadataTest, RejectsEmptyShardList) {
  // At least one shard is required for leader routing and global 3PC.
  EXPECT_THROW(ShardMetadata(WriteShardConfig(R"json({"shards": []})json"), 1),
               std::invalid_argument);
}

TEST(ShardMetadataTest, RejectsShardWithoutReplicas) {
  // Every shard needs at least one server replica, and its leader must be part
  // of that replica set.
  const std::string config = R"json(
{
  "shards": [
    {"shard_id": 0, "leader_id": 1, "replica_ids": []}
  ]
}
)json";

  EXPECT_THROW(ShardMetadata(WriteShardConfig(config), 1),
               std::invalid_argument);
}

TEST(ShardMetadataTest, RejectsLeaderOutsideReplicaList) {
  // The leader is a member of its shard, not an external coordinator.
  const std::string config = R"json(
{
  "shards": [
    {"shard_id": 0, "leader_id": 1, "replica_ids": [2, 3, 4]}
  ]
}
)json";

  EXPECT_THROW(ShardMetadata(WriteShardConfig(config), 2),
               std::invalid_argument);
}

TEST(ShardMetadataTest, RejectsDuplicateServerNodeAcrossShards) {
  // A server replica can belong to exactly one shard; otherwise local PBFT
  // membership and response filtering become ambiguous.
  const std::string config = R"json(
{
  "shards": [
    {"shard_id": 0, "leader_id": 1, "replica_ids": [1, 2]},
    {"shard_id": 1, "leader_id": 3, "replica_ids": [2, 3]}
  ]
}
)json";

  EXPECT_THROW(ShardMetadata(WriteShardConfig(config), 1),
               std::invalid_argument);
}

TEST(ShardMetadataTest, RejectsDuplicateShardIds) {
  // Shard ids are lookup keys, so duplicates must be rejected during parsing.
  const std::string config = R"json(
{
  "shards": [
    {"shard_id": 0, "leader_id": 1, "replica_ids": [1, 2]},
    {"shard_id": 0, "leader_id": 3, "replica_ids": [3, 4]}
  ]
}
)json";

  EXPECT_THROW(ShardMetadata(WriteShardConfig(config), 1),
               std::invalid_argument);
}

TEST(ShardMetadataTest, RejectsShardIdAboveUint32) {
  // Shard ids remain uint32_t even though node ids are int64_t.
  const uint64_t large_shard_id =
      static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()) + 1;
  const std::string config = R"json(
{
  "shards": [
    {"shard_id": )json" +
                             std::to_string(large_shard_id) + R"json(,
     "leader_id": 1, "replica_ids": [1, 2]}
  ]
}
)json";

  EXPECT_THROW(ShardMetadata(WriteShardConfig(config), 1),
               std::invalid_argument);
}

TEST(ShardMetadataTest, RejectsSelfNodeOutsideAllShards) {
  // A process can only start if its node id appears as either a server replica
  // or a configured client/proxy.
  EXPECT_THROW(ShardMetadata(WriteShardConfig(FourShardConfig()), 99),
               std::invalid_argument);
}

TEST(ShardMetadataTest, RejectsMalformedIds) {
  // Negative ids are invalid for this topology, even though node ids are stored
  // in signed int64_t form for compatibility with ReplicaInfo.
  const std::string config = R"json(
{
  "shards": [
    {"shard_id": 0, "leader_id": 1, "replica_ids": [1, -2]}
  ]
}
)json";

  EXPECT_THROW(ShardMetadata(WriteShardConfig(config), 1),
               std::invalid_argument);
}

TEST(ShardMetadataTest, RejectsUnknownLookups) {
  // Accessors fail loudly when asked about unknown shards or nodes. This makes
  // bad config easier to find during launch.
  ShardMetadata metadata(WriteShardConfig(FourShardConfig()), 1);

  EXPECT_THROW(metadata.LeaderForShard(99), std::invalid_argument);
  EXPECT_THROW(metadata.ReplicasForShard(99), std::invalid_argument);
  EXPECT_THROW(metadata.ShardForNode(99), std::invalid_argument);
  EXPECT_THROW(metadata.SameShard(1, 99), std::invalid_argument);
}

}  // namespace
}  // namespace resdb

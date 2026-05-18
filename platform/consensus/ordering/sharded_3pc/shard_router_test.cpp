#include "platform/consensus/ordering/sharded_3pc/shard_router.h"

#include <gtest/gtest.h>
#include <unistd.h>

#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace resdb {
namespace {

// The router is tested through real ShardMetadata so the ordering comes from
// parsed shard ids and leaders, not from hard-coded test state.
std::string WriteShardConfig() {
  static int file_count = 0;
  const char* test_tmpdir = std::getenv("TEST_TMPDIR");
  const std::string directory = test_tmpdir == nullptr ? "/tmp" : test_tmpdir;
  const std::string path = directory + "/shard_router_test_" +
                           std::to_string(getpid()) + "_" +
                           std::to_string(file_count++) + ".json";

  std::ofstream output(path);
  if (!output.is_open()) {
    throw std::runtime_error("failed to write test shard config " + path);
  }
  output << R"json(
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
  return path;
}

TEST(ShardRouterTest, ReturnsShardLeadersRoundRobin) {
  // The proxy uses this sequence to distribute consecutive batches across
  // shard leaders: 0, 1, 2, 3, then wrap back to 0.
  ShardMetadata metadata(WriteShardConfig(), 17);
  ShardRouter router(&metadata);

  std::vector<ShardRoute> routes;
  for (int i = 0; i < 5; ++i) {
    routes.push_back(router.NextShard());
  }

  EXPECT_EQ(routes[0].shard_id, 0);
  EXPECT_EQ(routes[0].leader_id, 1);
  EXPECT_EQ(routes[1].shard_id, 1);
  EXPECT_EQ(routes[1].leader_id, 5);
  EXPECT_EQ(routes[2].shard_id, 2);
  EXPECT_EQ(routes[2].leader_id, 9);
  EXPECT_EQ(routes[3].shard_id, 3);
  EXPECT_EQ(routes[3].leader_id, 13);
  EXPECT_EQ(routes[4].shard_id, 0);
  EXPECT_EQ(routes[4].leader_id, 1);
}

TEST(ShardRouterTest, RejectsNullMetadata) {
  // The router cannot choose leaders without topology metadata.
  EXPECT_THROW(ShardRouter(nullptr), std::invalid_argument);
}

}  // namespace
}  // namespace resdb

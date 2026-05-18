#include "platform/consensus/ordering/sharded_3pc/sharded_response_manager.h"

#include <gtest/gtest.h>
#include <unistd.h>

#include <atomic>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "platform/config/resdb_config_utils.h"
#include "platform/consensus/execution/system_info.h"

namespace resdb {
namespace {

// These tests exercise the proxy-side sharded response manager. The temporary
// config mirrors the default 4-shard local launch topology.
std::string WriteShardConfig() {
  static int file_count = 0;
  const char* test_tmpdir = std::getenv("TEST_TMPDIR");
  const std::string directory = test_tmpdir == nullptr ? "/tmp" : test_tmpdir;
  const std::string path = directory + "/sharded_response_manager_test_" +
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

ResDBConfig GenerateConfig() {
  std::vector<ReplicaInfo> replicas;
  for (int64_t node_id = 1; node_id <= 16; ++node_id) {
    replicas.push_back(
        GenerateReplicaInfo(node_id, "127.0.0.1", 10000 + node_id));
  }
  return ResDBConfig(replicas, GenerateReplicaInfo(17, "127.0.0.1", 10017));
}

class ExposedShardedResponseManager : public ShardedResponseManager {
 public:
  using ShardedResponseManager::ShardedResponseManager;

  // Expose protected hooks so routing and response-quorum decisions can be
  // tested without starting a background batching thread.
  int64_t GetRequestTargetForTest(Request* request, uint64_t local_id) {
    return GetRequestTarget(request, local_id);
  }

  bool MayConsensusChangeStatusForTest(
      int type, uint64_t local_id, int received_count,
      std::atomic<TransactionStatue>* status) {
    return MayConsensusChangeStatus(type, local_id, received_count, status);
  }
};

TEST(ShardedResponseManagerTest, RoutesBatchesRoundRobinAndAnnotatesRequests) {
  // Consecutive proxy batches should rotate through shard leaders. Each
  // outgoing request also receives metadata needed by global 3PC.
  ResDBConfig config = GenerateConfig();
  SystemInfo system_info(config);
  ReplicaCommunicator communicator(config.GetReplicaInfos());
  ShardMetadata metadata(WriteShardConfig(), config.GetSelfInfo().id());
  ExposedShardedResponseManager manager(config, &communicator, &system_info,
                                        nullptr, &metadata);

  Request first;
  Request second;
  Request third;
  Request fourth;
  Request fifth;

  EXPECT_EQ(manager.GetRequestTargetForTest(&first, 1), 1);
  EXPECT_EQ(manager.GetRequestTargetForTest(&second, 2), 5);
  EXPECT_EQ(manager.GetRequestTargetForTest(&third, 3), 9);
  EXPECT_EQ(manager.GetRequestTargetForTest(&fourth, 4), 13);
  EXPECT_EQ(manager.GetRequestTargetForTest(&fifth, 5), 1);

  EXPECT_EQ(first.coordinator_shard_id(), 0);
  EXPECT_EQ(first.global_coordinator_id(), 1);
  EXPECT_EQ(first.global_txn_id(), 1);
  EXPECT_TRUE(first.is_global_3pc());
  EXPECT_EQ(second.coordinator_shard_id(), 1);
  EXPECT_EQ(third.coordinator_shard_id(), 2);
  EXPECT_EQ(fourth.coordinator_shard_id(), 3);
}

TEST(ShardedResponseManagerTest, UsesCoordinatorShardResponseQuorum) {
  // The proxy should finish after enough responses from the coordinator shard,
  // not after a threshold based on the full 16-replica deployment.
  ResDBConfig config = GenerateConfig();
  SystemInfo system_info(config);
  ReplicaCommunicator communicator(config.GetReplicaInfos());
  ShardMetadata metadata(WriteShardConfig(), config.GetSelfInfo().id());
  ExposedShardedResponseManager manager(config, &communicator, &system_info,
                                        nullptr, &metadata);
  Request request;
  ASSERT_EQ(manager.GetRequestTargetForTest(&request, 42), 1);

  std::atomic<TransactionStatue> status(TransactionStatue::None);
  EXPECT_FALSE(manager.MayConsensusChangeStatusForTest(
      Request::TYPE_RESPONSE, 42, 1, &status));
  EXPECT_EQ(status.load(), TransactionStatue::None);
  EXPECT_TRUE(manager.MayConsensusChangeStatusForTest(
      Request::TYPE_RESPONSE, 42, 2, &status));
  EXPECT_EQ(status.load(), TransactionStatue::EXECUTED);
}

}  // namespace
}  // namespace resdb

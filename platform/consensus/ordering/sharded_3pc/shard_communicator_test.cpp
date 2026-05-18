#include "platform/consensus/ordering/sharded_3pc/shard_communicator.h"

#include <gtest/gtest.h>
#include <unistd.h>

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "platform/proto/resdb.pb.h"

namespace resdb {
namespace {

// Test helpers build a small in-memory network and a temporary shard config so
// target selection can be checked without opening real sockets.
ReplicaInfo MakeReplica(int64_t node_id) {
  ReplicaInfo replica;
  replica.set_id(node_id);
  replica.set_ip("127.0.0.1");
  replica.set_port(10000 + static_cast<int>(node_id % 1000));
  return replica;
}

std::vector<ReplicaInfo> MakeReplicas(int64_t begin, int64_t end) {
  std::vector<ReplicaInfo> replicas;
  for (int64_t node_id = begin; node_id <= end; ++node_id) {
    replicas.push_back(MakeReplica(node_id));
  }
  return replicas;
}

std::string WriteShardConfig(const std::string& contents) {
  static int file_count = 0;
  const char* test_tmpdir = std::getenv("TEST_TMPDIR");
  const std::string directory = test_tmpdir == nullptr ? "/tmp" : test_tmpdir;
  const std::string path = directory + "/shard_communicator_test_" +
                           std::to_string(getpid()) + "_" +
                           std::to_string(file_count++) + ".json";

  std::ofstream output(path);
  if (!output.is_open()) {
    throw std::runtime_error("failed to write test shard config " + path);
  }
  output << contents;
  return path;
}

std::string ShardConfig() {
  return R"json(
{
  "shards": [
    {"shard_id": 0, "leader_id": 1, "replica_ids": [1, 2, 3, 4]},
    {"shard_id": 1, "leader_id": 5, "replica_ids": [5, 6, 7, 8]},
    {"shard_id": 2, "leader_id": 9, "replica_ids": [9, 10, 11, 12]}
  ],
  "client_ids": [13]
}
)json";
}

class RecordingReplicaCommunicator : public ReplicaCommunicator {
 public:
  RecordingReplicaCommunicator() : ReplicaCommunicator({}) {}

  // Record the selected target instead of sending over the network. These
  // tests care about routing decisions, not transport behavior.
  int SendMessage(const google::protobuf::Message&,
                  const ReplicaInfo& replica) override {
    sent_node_ids.push_back(replica.id());
    return next_return;
  }

  int next_return = 0;
  std::vector<int64_t> sent_node_ids;
};

class ShardCommunicatorTest : public ::testing::Test {
 protected:
  // Self node 6 is inside shard 1, so local-shard broadcasts should target
  // nodes 5, 6, 7, and 8.
  ShardCommunicatorTest()
      : metadata_(WriteShardConfig(ShardConfig()), 6),
        replicas_(MakeReplicas(1, 12)),
        communicator_(&replica_communicator_, &metadata_, replicas_) {}

  Request request_;
  ShardMetadata metadata_;
  std::vector<ReplicaInfo> replicas_;
  RecordingReplicaCommunicator replica_communicator_;
  ShardCommunicator communicator_;
};

TEST_F(ShardCommunicatorTest, SendsToOneRequestedNode) {
  // Direct sends should resolve exactly the requested node id.
  EXPECT_EQ(communicator_.SendToNode(request_, 7), 1);
  EXPECT_EQ(replica_communicator_.sent_node_ids,
            (std::vector<int64_t>{7}));
}

TEST_F(ShardCommunicatorTest, BroadcastsToLocalShard) {
  // Local broadcasts are the core local-PBFT path: only replicas in the
  // caller's shard should receive the message.
  EXPECT_EQ(communicator_.BroadcastToLocalShard(request_), 4);
  EXPECT_EQ(replica_communicator_.sent_node_ids,
            (std::vector<int64_t>{5, 6, 7, 8}));
}

TEST_F(ShardCommunicatorTest, BroadcastsToShard) {
  // Explicit shard broadcasts let later code target a known shard by id.
  EXPECT_EQ(communicator_.BroadcastToShard(request_, 2), 4);
  EXPECT_EQ(replica_communicator_.sent_node_ids,
            (std::vector<int64_t>{9, 10, 11, 12}));
}

TEST_F(ShardCommunicatorTest, BroadcastsToShardLeaders) {
  // Global 3PC uses this path: one target per shard leader, not every replica.
  EXPECT_EQ(communicator_.BroadcastToShardLeaders(request_), 3);
  EXPECT_EQ(replica_communicator_.sent_node_ids,
            (std::vector<int64_t>{1, 5, 9}));
}

TEST_F(ShardCommunicatorTest, ExcludesSelfWhenRequested) {
  // Some protocol steps need to broadcast to peers while skipping the sender.
  EXPECT_EQ(communicator_.BroadcastToLocalShard(request_, false), 3);
  EXPECT_EQ(replica_communicator_.sent_node_ids,
            (std::vector<int64_t>{5, 7, 8}));
}

TEST(ShardCommunicatorStandaloneTest, ExcludesSelfFromShardLeaders) {
  // When the caller is itself a shard leader, leader broadcasts can skip that
  // leader while still reaching the other shards.
  ShardMetadata metadata(WriteShardConfig(ShardConfig()), 5);
  auto replicas = MakeReplicas(1, 12);
  RecordingReplicaCommunicator replica_communicator;
  ShardCommunicator communicator(&replica_communicator, &metadata, replicas);

  Request request;
  EXPECT_EQ(communicator.BroadcastToShardLeaders(request, false), 2);
  EXPECT_EQ(replica_communicator.sent_node_ids,
            (std::vector<int64_t>{1, 9}));
}

TEST_F(ShardCommunicatorTest, SendsToShardLeader) {
  // This is the single-leader target used by proxy routing and direct handoff.
  EXPECT_EQ(communicator_.SendToShardLeader(request_, 2), 1);
  EXPECT_EQ(replica_communicator_.sent_node_ids,
            (std::vector<int64_t>{9}));
}

TEST_F(ShardCommunicatorTest, FailedUnderlyingSendIsNotCounted) {
  // Return values report successful sends, so a failed transport call should
  // not increase the success count even though a target was selected.
  replica_communicator_.next_return = -1;

  EXPECT_EQ(communicator_.SendToNode(request_, 7), 0);
  EXPECT_EQ(replica_communicator_.sent_node_ids,
            (std::vector<int64_t>{7}));
}

TEST(ShardCommunicatorStandaloneTest, RoutesNodeIdsAboveUint32) {
  // Node ids follow ResilientDB's int64_t ids, so large node ids should route
  // correctly even though shard ids remain uint32_t.
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
  RecordingReplicaCommunicator replica_communicator;
  std::vector<ReplicaInfo> replicas = {MakeReplica(large_node_id),
                                       MakeReplica(2)};
  ShardCommunicator communicator(&replica_communicator, &metadata, replicas);
  Request request;

  EXPECT_EQ(communicator.SendToNode(request, large_node_id), 1);
  EXPECT_EQ(replica_communicator.sent_node_ids,
            (std::vector<int64_t>{large_node_id}));
}

TEST(ShardCommunicatorStandaloneTest, RejectsNullCommunicator) {
  // Constructor validation catches missing dependencies before consensus code
  // tries to route a message.
  ShardMetadata metadata(WriteShardConfig(ShardConfig()), 1);

  EXPECT_THROW(ShardCommunicator(nullptr, &metadata, MakeReplicas(1, 12)),
               std::invalid_argument);
}

TEST(ShardCommunicatorStandaloneTest, RejectsNullMetadata) {
  // Metadata is required because all target selection flows through it.
  RecordingReplicaCommunicator replica_communicator;

  EXPECT_THROW(ShardCommunicator(&replica_communicator, nullptr,
                                 MakeReplicas(1, 12)),
               std::invalid_argument);
}

TEST(ShardCommunicatorStandaloneTest, RejectsDuplicateReplicaIds) {
  // A duplicate ReplicaInfo id would make node-id routing ambiguous.
  ShardMetadata metadata(WriteShardConfig(ShardConfig()), 1);
  RecordingReplicaCommunicator replica_communicator;
  std::vector<ReplicaInfo> replicas = MakeReplicas(1, 12);
  replicas.push_back(MakeReplica(1));

  EXPECT_THROW(ShardCommunicator(&replica_communicator, &metadata, replicas),
               std::invalid_argument);
}

TEST_F(ShardCommunicatorTest, RejectsUnknownNode) {
  // Unknown node ids should fail loudly instead of silently dropping messages.
  EXPECT_THROW(communicator_.SendToNode(request_, 99), std::invalid_argument);
}

TEST_F(ShardCommunicatorTest, RejectsUnknownShard) {
  // Unknown shard ids are topology/configuration errors.
  EXPECT_THROW(communicator_.BroadcastToShard(request_, 99),
               std::invalid_argument);
}

TEST(ShardCommunicatorStandaloneTest, RejectsMetadataNodeMissingFromReplicas) {
  // The shard config and network server config must agree. If metadata names a
  // replica that is absent from server.config, routing cannot continue.
  ShardMetadata metadata(WriteShardConfig(ShardConfig()), 1);
  RecordingReplicaCommunicator replica_communicator;
  ShardCommunicator communicator(&replica_communicator, &metadata,
                                 MakeReplicas(1, 11));
  Request request;

  EXPECT_THROW(communicator.BroadcastToShard(request, 2),
               std::invalid_argument);
}

}  // namespace
}  // namespace resdb

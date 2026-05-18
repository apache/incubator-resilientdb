#include "platform/consensus/ordering/sharded_3pc/shard_3pc_commitment.h"
#include "platform/consensus/ordering/sharded_3pc/shard_pbft_commitment.h"

#include <gtest/gtest.h>
#include <unistd.h>

#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "common/crypto/mock_signature_verifier.h"
#include "platform/config/resdb_config_utils.h"
#include "platform/consensus/ordering/pbft/checkpoint_manager.h"
#include "platform/consensus/ordering/pbft/message_manager.h"
#include "platform/proto/resdb.pb.h"

namespace resdb {
namespace {

// Build a small two-shard network so the commitment wrappers can be tested
// without running the full sharded consensus manager.
ResDBConfig GenerateConfig() {
  ResConfigData data;
  data.set_duplicate_check_frequency_useconds(100000);
  return ResDBConfig({GenerateReplicaInfo(1, "127.0.0.1", 1234),
                      GenerateReplicaInfo(2, "127.0.0.1", 1235),
                      GenerateReplicaInfo(3, "127.0.0.1", 1236),
                      GenerateReplicaInfo(4, "127.0.0.1", 1237),
                      GenerateReplicaInfo(5, "127.0.0.1", 1238),
                      GenerateReplicaInfo(6, "127.0.0.1", 1239),
                      GenerateReplicaInfo(7, "127.0.0.1", 1240),
                      GenerateReplicaInfo(8, "127.0.0.1", 1241)},
                     GenerateReplicaInfo(2, "127.0.0.1", 1235), data);
}

std::string WriteShardConfig() {
  static int file_count = 0;
  const char* test_tmpdir = std::getenv("TEST_TMPDIR");
  const std::string directory = test_tmpdir == nullptr ? "/tmp" : test_tmpdir;
  const std::string path = directory + "/sharded_commitment_test_" +
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
    {"shard_id": 1, "leader_id": 5, "replica_ids": [5, 6, 7, 8]}
  ],
  "client_ids": [9]
}
)json";
  return path;
}

class RecordingReplicaCommunicator : public ReplicaCommunicator {
 public:
  RecordingReplicaCommunicator() : ReplicaCommunicator({}) {}

  // The sharded commitment tests only need to know which nodes were targeted.
  int SendMessage(const google::protobuf::Message&,
                  const ReplicaInfo& replica) override {
    sent_node_ids.push_back(replica.id());
    return next_return;
  }

  int next_return = 0;
  std::vector<int64_t> sent_node_ids;
};

class ExposedShardPBFTCommitment : public ShardPBFTCommitment {
 public:
  using ShardPBFTCommitment::ShardPBFTCommitment;

  // Expose the protected routing hook so the test can verify the wrapper's
  // target selection without driving an entire PBFT round.
  int BroadcastForTest(const google::protobuf::Message& msg) {
    return BroadcastConsensusMsg(msg);
  }
};

class ExposedShard3PCCommitment : public Shard3PCCommitment {
 public:
  using Shard3PCCommitment::Shard3PCCommitment;

  // Expose the global 3PC routing hook and participant count hook directly.
  int BroadcastForTest(const google::protobuf::Message& msg) {
    return BroadcastConsensusMsg(msg);
  }

  size_t ExpectedParticipantCountForTest() const {
    return ExpectedThreePCParticipantCount();
  }
};

class ShardedCommitmentTest : public ::testing::Test {
 protected:
  // Self node 2 is in shard 0. Local PBFT should target nodes 1-4, while
  // global 3PC should target leaders 1 and 5.
  ShardedCommitmentTest()
      : config_(GenerateConfig()),
        system_info_(config_),
        checkpoint_manager_(config_, &replica_communicator_, &verifier_,
                            &system_info_),
        message_manager_(std::make_unique<MessageManager>(
            config_, nullptr, &checkpoint_manager_, &system_info_)),
        shard_metadata_(WriteShardConfig(), config_.GetSelfInfo().id()),
        shard_communicator_(&replica_communicator_, &shard_metadata_,
                            config_.GetReplicaInfos()) {}

  ResDBConfig config_;
  SystemInfo system_info_;
  RecordingReplicaCommunicator replica_communicator_;
  MockSignatureVerifier verifier_;
  CheckPointManager checkpoint_manager_;
  std::unique_ptr<MessageManager> message_manager_;
  ShardMetadata shard_metadata_;
  ShardCommunicator shard_communicator_;
};

TEST_F(ShardedCommitmentTest, PBFTBroadcastTargetsLocalShard) {
  // Local PBFT must not broadcast to every server in the deployment.
  ExposedShardPBFTCommitment commitment(
      config_, message_manager_.get(), &replica_communicator_,
      &shard_communicator_, &verifier_);
  Request request;

  EXPECT_EQ(commitment.BroadcastForTest(request), 4);
  EXPECT_EQ(replica_communicator_.sent_node_ids,
            (std::vector<int64_t>{1, 2, 3, 4}));
}

TEST_F(ShardedCommitmentTest, ThreePCBroadcastTargetsShardLeaders) {
  // Global 3PC runs between shard leaders, so only one node per shard should
  // receive global phase messages.
  ExposedShard3PCCommitment commitment(
      config_, message_manager_.get(), &replica_communicator_,
      &shard_communicator_, &shard_metadata_, &verifier_);
  Request request;

  EXPECT_EQ(commitment.BroadcastForTest(request), 2);
  EXPECT_EQ(replica_communicator_.sent_node_ids,
            (std::vector<int64_t>{1, 5}));
}

TEST_F(ShardedCommitmentTest, ThreePCParticipantCountIsShardCount) {
  // The 3PC vote/ack thresholds are based on number of shards, not number of
  // replicas in the full network.
  ExposedShard3PCCommitment commitment(
      config_, message_manager_.get(), &replica_communicator_,
      &shard_communicator_, &shard_metadata_, &verifier_);

  EXPECT_EQ(commitment.ExpectedParticipantCountForTest(), 2);
}

}  // namespace
}  // namespace resdb

#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <thread>

#include "platform/config/resdb_config_utils.h"
#include "platform/consensus/ordering/raft/algorithm/leaderelection_manager.h"
#include "platform/consensus/ordering/raft/algorithm/mock_raft.h"

namespace resdb {
namespace raft {

using ::testing::Invoke;

ResDBConfig GenerateConfig() {
  ResConfigData data;
  data.set_duplicate_check_frequency_useconds(100000);
  data.set_enable_viewchange(true);
  return ResDBConfig({GenerateReplicaInfo(1, "127.0.0.1", 1234),
                      GenerateReplicaInfo(2, "127.0.0.1", 1235),
                      GenerateReplicaInfo(3, "127.0.0.1", 1236),
                      GenerateReplicaInfo(4, "127.0.0.1", 1237)},
                     GenerateReplicaInfo(1, "127.0.0.1", 1234), data);
}

class TestLeaderElectionManager : public LeaderElectionManager {
 public:
  TestLeaderElectionManager(const ResDBConfig& config)
      : LeaderElectionManager(config) {}
  uint64_t GetHeartbeatCount() {
    std::lock_guard<std::mutex> lk(cv_mutex_);
    return heartbeat_count_;
  }
  uint64_t GetBroadcastCount() {
    std::lock_guard<std::mutex> lk(cv_mutex_);
    return broadcast_count_;
  }

 private:
  // Overriding this is used to set the timeout timer to start an election to 50
  // ms.
  uint64_t RandomInt(uint64_t min, uint64_t max) { return 50; }
};

class LeaderElectionManagerTest : public ::testing::Test {
 protected:
  LeaderElectionManagerTest() : config_(GenerateConfig()) {}

  void SetUp() override {
    verifier_ = nullptr;
    replica_communicator_ = nullptr;
    leader_election_manager_ =
        std::make_unique<TestLeaderElectionManager>(config_);
    mock_raft_ = std::make_unique<MockRaft>(1, 1, 3, verifier_.get(),
                                            leader_election_manager_.get(),
                                            replica_communicator_.get());
  }

  void TearDown() override {
    if (leader_election_manager_) {
      leader_election_manager_.reset();
    }
    if (mock_raft_) {
      mock_raft_.reset();
    }
  }

  ResDBConfig config_;
  std::unique_ptr<SignatureVerifier> verifier_;
  std::unique_ptr<ReplicaCommunicator> replica_communicator_;
  std::unique_ptr<TestLeaderElectionManager> leader_election_manager_;
  std::unique_ptr<MockRaft> mock_raft_;
};

// Test 1: Follower timeout should trigger election.
TEST_F(LeaderElectionManagerTest, FollowerTimeoutTriggersElection) {
  mock_raft_->SetRole(Role::FOLLOWER);

  std::promise<bool> election_started;
  std::future<bool> election_started_future = election_started.get_future();

  leader_election_manager_->SetRaft(mock_raft_.get());
  leader_election_manager_->MayStart();

  EXPECT_CALL(*mock_raft_, StartElection).WillOnce(Invoke([&]() {
    election_started.set_value(true);
  }));

  auto status =
      election_started_future.wait_for(std::chrono::milliseconds(100));
  ASSERT_EQ(status, std::future_status::ready);
}

// Test 2: Follower should not start election before timing out.
TEST_F(LeaderElectionManagerTest, FollowerShouldNotStartElectionEarly) {
  mock_raft_->SetRole(Role::FOLLOWER);

  std::promise<bool> election_started;
  std::future<bool> election_started_future = election_started.get_future();

  EXPECT_CALL(*mock_raft_, StartElection()).Times(0);

  leader_election_manager_->SetRaft(mock_raft_.get());
  leader_election_manager_->MayStart();

  std::this_thread::sleep_for(std::chrono::milliseconds(45));
  // Since the timeout timer is set to 50 ms, StartElection should never be
  // called.
}

// Test 3: Follower receiving heartbeat should NOT trigger election.
TEST_F(LeaderElectionManagerTest,
       FollowerReceivingHeartbeatDoesNotStartElection) {
  mock_raft_->SetRole(Role::FOLLOWER);

  std::promise<bool> election_started;
  std::future<bool> election_started_future = election_started.get_future();

  EXPECT_CALL(*mock_raft_, StartElection()).Times(0);

  leader_election_manager_->SetRaft(mock_raft_.get());
  leader_election_manager_->MayStart();

  std::this_thread::sleep_for(std::chrono::milliseconds(45));
  leader_election_manager_->OnHeartBeat();

  std::this_thread::sleep_for(std::chrono::milliseconds(45));
  ASSERT_EQ(leader_election_manager_->GetHeartbeatCount(), 1);
  // Since the timeout timer is set to 50 ms, StartElection should never be
  // called.
}

// Test 4: Leader timeout should send heartbeat.
TEST_F(LeaderElectionManagerTest, LeaderTimeoutSendsHeartbeat) {
  mock_raft_->SetRole(Role::LEADER);

  std::promise<bool> heartbeat_sent;
  std::future<bool> heartbeat_sent_future = heartbeat_sent.get_future();

  leader_election_manager_->SetRaft(mock_raft_.get());
  leader_election_manager_->MayStart();

  EXPECT_CALL(*mock_raft_, SendHeartBeat).WillOnce(Invoke([&]() {
    heartbeat_sent.set_value(true);
  }));

  auto status = heartbeat_sent_future.wait_for(std::chrono::milliseconds(105));
  ASSERT_EQ(status, std::future_status::ready);
}

// Test 5: Leader should not send heartbeat before timing out.
TEST_F(LeaderElectionManagerTest, LeaderShouldNotSendHeartbeatEarly) {
  mock_raft_->SetRole(Role::LEADER);

  std::promise<bool> heartbeat_sent;
  std::future<bool> heartbeat_sent_future = heartbeat_sent.get_future();

  EXPECT_CALL(*mock_raft_, SendHeartBeat()).Times(0);

  leader_election_manager_->SetRaft(mock_raft_.get());
  leader_election_manager_->MayStart();

  std::this_thread::sleep_for(std::chrono::milliseconds(95));
  // Since the heartbeat timer is set to 100 ms, SendHeartBeat should never be
  // called.
}

// Test 6: Leader sending some broadcast should not be sending heartbeats.
TEST_F(LeaderElectionManagerTest, LeaderWithBroadcastDoesNotSendHeartbeat) {
  mock_raft_->SetRole(Role::LEADER);

  std::promise<bool> heartbeat_sent;
  std::future<bool> heartbeat_sent_future = heartbeat_sent.get_future();

  EXPECT_CALL(*mock_raft_, SendHeartBeat()).Times(0);
  leader_election_manager_->SetRaft(mock_raft_.get());
  leader_election_manager_->MayStart();

  // Send broadcasts to reset the timer.
  for (int i = 0; i < 3; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(95));
    leader_election_manager_->OnAeBroadcast();
  }

  ASSERT_EQ(leader_election_manager_->GetBroadcastCount(), 3);
}

}  // namespace raft
}  // namespace resdb

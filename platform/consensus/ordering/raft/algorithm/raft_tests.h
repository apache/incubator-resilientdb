#include "platform/consensus/ordering/raft/algorithm/raft_test_util.h"
#include "platform/consensus/recovery/mock_raft_recovery.h"

namespace resdb {
namespace raft {
using resdb::raft::test_utils::CreateAeFields;
using resdb::raft::test_utils::CreateAeMessage;
using resdb::raft::test_utils::CreateLogEntries;
using resdb::raft::test_utils::GenerateConfig;
using resdb::raft::test_utils::MockBroadcastFunction;
using resdb::raft::test_utils::MockCommitFunction;
using resdb::raft::test_utils::MockSendMessageFunction;
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Invoke;
using ::testing::Matcher;

class RaftTest : public ::testing::Test {

 protected:
  void SetUp() override {
    verifier_ = std::make_unique<MockSignatureVerifier>();
    ResDBConfig config_ = GenerateConfig();
    leader_election_manager_ =
        std::make_unique<MockLeaderElectionManager>(config_);
    replica_communicator_ = std::make_unique<MockReplicaCommunicator>();
    recovery_ = std::make_unique<MockRaftRecovery>(config_);
    raft_ = std::make_unique<Raft>(
        /*id=*/1,
        /*f=*/1,
        /*total=*/4, verifier_.get(), leader_election_manager_.get(),
        replica_communicator_.get(), recovery_.get());

    raft_->SetSingleCallFunc(
        [&](int type, const google::protobuf::Message& msg, int node_id) {
          return mock_call.Call(type, msg, node_id);
        });

    raft_->SetBroadcastCallFunc(
        [&](int type, const google::protobuf::Message& msg) {
          return mock_broadcast.Broadcast(type, msg);
        });

    raft_->SetCommitFunc([&](const google::protobuf::Message& msg) {
      return mock_commit.Commit(msg);
    });
  }

  std::unique_ptr<MockSignatureVerifier> verifier_;
  std::unique_ptr<MockLeaderElectionManager> leader_election_manager_;
  std::unique_ptr<MockReplicaCommunicator> replica_communicator_;
  std::unique_ptr<MockRaftRecovery> recovery_;
  std::unique_ptr<Raft> raft_;
  MockSendMessageFunction mock_call;
  MockBroadcastFunction mock_broadcast;
  MockCommitFunction mock_commit;
};

}  // namespace raft
}  // namespace resdb

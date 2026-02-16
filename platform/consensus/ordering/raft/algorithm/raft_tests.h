#include <gmock/gmock.h>

#include "platform/config/resdb_config_utils.h"
#include "common/crypto/mock_signature_verifier.h"
#include "platform/networkstrate/mock_replica_communicator.h"
#include "platform/consensus/ordering/raft/algorithm/mock_leader_election_manager.h"
#include "platform/consensus/ordering/raft/algorithm/raft.h"
#include "platform/proto/client_test.pb.h"

namespace resdb {
namespace raft {
using ::testing::Invoke;
using ::testing::_;
using ::testing::Matcher;
using ::testing::AnyNumber;

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

class RaftTest : public ::testing::Test {
 private:
  class MockSendMessageFunction {
  public:
      MOCK_METHOD(int, Call, (int, const google::protobuf::Message&, int));
  };
  class MockBroadcastFunction {
  public:
      MOCK_METHOD(int, Broadcast, (int, const google::protobuf::Message&));
  };
  class MockCommitFunction {
  public:
      MOCK_METHOD(int, Commit, (const google::protobuf::Message&));
  };

 protected:
  void SetUp() override {
    verifier_ = std::make_unique<MockSignatureVerifier>();
    leader_election_manager_ = std::make_unique<MockLeaderElectionManager>(GenerateConfig());
    replica_communicator_ = std::make_unique<MockReplicaCommunicator>();
    raft_ = std::make_unique<Raft>(
        /*id=*/1,
        /*f=*/1,
        /*total=*/4,
        verifier_.get(),
        leader_election_manager_.get(),
        replica_communicator_.get());

    raft_->SetSingleCallFunc(
    [&](int type, const google::protobuf::Message& msg, int node_id) {
        return mock_call.Call(type, msg, node_id);
    });

    raft_->SetBroadcastCallFunc(
    [&](int type, const google::protobuf::Message& msg) {
        return mock_broadcast.Broadcast(type, msg);
    });

    raft_->SetCommitFunc(
    [&](const google::protobuf::Message& msg) {
        return mock_commit.Commit(msg);
    });
  }

  AeFields CreateAeFields(uint64_t term, int leaderId, uint64_t prevLogIndex, uint64_t prevLogTerm, const std::vector<std::unique_ptr<LogEntry>>& entries, uint64_t leaderCommit, int followerId)  {
    AeFields fields{};
    fields.term = term;
    fields.leaderId = leaderId;
    fields.leaderCommit = leaderCommit;
    fields.prevLogIndex = prevLogIndex;
    fields.prevLogTerm = prevLogTerm;
    fields.followerId = followerId;

    for (const auto& logEntry : entries) {
      LogEntry entry;
      entry.term = logEntry->term;
      entry.command = logEntry->command;
      fields.entries.push_back(std::move(entry));
    }

    return fields;
  };

  // Helper to create a single log entry
  std::unique_ptr<LogEntry> CreateLogEntry(uint64_t term, const std::string& command_data) {
    auto entry = std::make_unique<LogEntry>();
    entry->term = term;
    entry->command = command_data;
    return entry;
  }

  // Helper to create a vector of log entries for testing
  std::vector<std::unique_ptr<LogEntry>> CreateLogEntries(const std::vector<std::pair<uint64_t, std::string>>& term_and_cmds, bool usedForLogPatch = false) {
    std::vector<std::unique_ptr<LogEntry>> entries;

    if (usedForLogPatch) {
      std::unique_ptr<LogEntry> first_entry = std::make_unique<LogEntry>();
      first_entry->term = 0;
      first_entry->command = "COMMON_PREFIX";
      entries.push_back(std::move(first_entry));
    }
    
    for (const auto& [term, cmd] : term_and_cmds) {
        std::unique_ptr<LogEntry> entry = std::make_unique<LogEntry>();
        entry->term = term;
        // entry->command = cmd;

        ClientTestRequest req;
        req.set_value(cmd);
        std::string serialized;
        req.SerializeToString(&serialized);
        entry->command = serialized;
        entries.push_back(std::move(entry));
    }
    return entries;
  }

  AppendEntries CreateAeMessage(const AeFields& fields) {
    AppendEntries ae;
    ae.set_term(fields.term);
    ae.set_leaderid(fields.leaderId);
    ae.set_prevlogindex(fields.prevLogIndex);
    ae.set_prevlogterm(fields.prevLogTerm);
    ae.set_leadercommitindex(fields.leaderCommit);
    for (const auto& entry : fields.entries) {
      auto* newEntry = ae.add_entries();
      newEntry->set_term(entry.term);
      newEntry->set_command(entry.command);
    }

    return ae;
  }

  std::unique_ptr<MockSignatureVerifier> verifier_;
  std::unique_ptr<MockLeaderElectionManager> leader_election_manager_;
  std::unique_ptr<MockReplicaCommunicator> replica_communicator_;
  std::unique_ptr<Raft> raft_;
  MockSendMessageFunction mock_call;
  MockBroadcastFunction mock_broadcast;
  MockCommitFunction mock_commit;
};


} // namespace raft
} // namespace resdb

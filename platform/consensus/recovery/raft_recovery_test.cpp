#include "platform/consensus/recovery/raft_recovery.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>

#include "chain/storage/mock_storage.h"
#include "platform/consensus/checkpoint/mock_checkpoint.h"
#include "platform/consensus/ordering/common/transaction_utils.h"
#include "platform/consensus/ordering/raft/proto/proposal.pb.h"

namespace resdb {
namespace raft {
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Invoke;
using ::testing::Matcher;
using ::testing::Test;

const std::string log_path = "./log/test_log";

ResConfigData GetConfigData(int buf_size = 10) {
  ResConfigData data;
  data.set_recovery_enabled(true);
  data.set_recovery_path(log_path);
  data.set_recovery_buffer_size(buf_size);
  data.set_recovery_ckpt_time_s(1);

  return data;
}

std::vector<std::string> Listlogs(const std::string &path) {
  std::vector<std::string> ret;
  std::string dir = std::filesystem::path(path).parent_path();
  for (const auto &entry : std::filesystem::directory_iterator(dir)) {
    LOG(ERROR) << "path:" << entry.path();
    ret.push_back(entry.path());
  }
  return ret;
}

class RaftRecoveryTest : public Test {
 public:
  RaftRecoveryTest()
      : config_(GetConfigData(), ReplicaInfo(), KeyInfo(), CertificateInfo()) {
    std::string dir = std::filesystem::path(log_path).parent_path();
    std::filesystem::remove_all(dir);
  }

 protected:
  ResDBConfig config_;
  MockCheckPoint checkpoint_;
};

TEST_F(RaftRecoveryTest, WriteAndReadLog) {
  int entries_to_add = 3;
  {
    RaftRecovery recovery(config_, &checkpoint_, nullptr);

    for (int i = 0; i < entries_to_add; i++) {
      Entry logEntry;
      logEntry.set_term(i + 1);
      auto req = std::make_unique<Request>();
      req->set_seq(i + 1);
      req->set_data("Request " + std::to_string(i + 1));
      std::string serialized;
      if (!req->SerializeToString(&serialized)) {
        assert(false);
      }
      logEntry.set_command(std::move(serialized));

      recovery.AddLogEntry(&logEntry);
    }
  }
  {
    std::vector<Entry> list;
    RaftRecovery recovery(config_, &checkpoint_, nullptr);
    recovery.ReadLogs(
        [&](const RaftMetadata &data) {},
        [&](std::unique_ptr<Entry> entry) { list.push_back(*entry); }, nullptr);

    EXPECT_EQ(list.size(), entries_to_add);

    for (size_t i = 0; i < entries_to_add; ++i) {
      EXPECT_EQ(list[i].term(), i + 1);
      Request req;
      req.ParseFromString(list[i].command());
      EXPECT_EQ(req.data(), "Request " + std::to_string(i + 1));
    }
  }
}

TEST_F(RaftRecoveryTest, WriteAndReadMetadata) {
  {
    RaftRecovery recovery(config_, &checkpoint_, nullptr);

    recovery.WriteMetadata(2, 1);
  }
  {
    int64_t current_term;
    int32_t voted_for;
    RaftRecovery recovery(config_, &checkpoint_, nullptr);
    recovery.ReadLogs(
        [&](const RaftMetadata &data) {
          current_term = data.current_term;
          voted_for = data.voted_for;
        },
        [&](std::unique_ptr<Entry> entry) {}, nullptr);

    EXPECT_EQ(current_term, 2);
    EXPECT_EQ(voted_for, 1);
  }
}

}  // namespace raft
}  // namespace resdb

#include <filesystem>
#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>


#include "platform/consensus/ordering/raft/framework/raft_recovery.h"
#include "platform/consensus/ordering/raft/proto/proposal.pb.h"
#include "chain/storage/mock_storage.h"
#include "platform/consensus/checkpoint/mock_checkpoint.h"
#include "platform/consensus/ordering/raft/framework/transaction_utils.h"


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
      : config_(GetConfigData(), ReplicaInfo(), KeyInfo(), CertificateInfo()),
        system_info_() {
    std::string dir = std::filesystem::path(log_path).parent_path();
    std::filesystem::remove_all(dir);
  }

 protected:
  ResDBConfig config_;
  SystemInfo system_info_;
  MockCheckPoint checkpoint_;
};

TEST_F(RaftRecoveryTest, ReadLog) {
  std::vector<int> types = {Request::TYPE_PRE_PREPARE, Request::TYPE_PREPARE,
                            Request::TYPE_COMMIT,      Request::TYPE_CHECKPOINT,
                            Request::TYPE_NEWVIEW,     Request::TYPE_NEW_TXNS};

  std::vector<int> expected_types = {
      Request::TYPE_PRE_PREPARE, Request::TYPE_PREPARE, Request::TYPE_COMMIT,
      Request::TYPE_CHECKPOINT,  Request::TYPE_NEWVIEW,
  };

  int entries_to_add = 3;
  {
    RaftRecovery recovery(config_, &checkpoint_, &system_info_, nullptr);

    for (int i = 0; i < entries_to_add; i++) {
        // Set up the Log Entry to be added
        Entry logEntry;
        logEntry.set_term(i);
        auto req = std::make_unique<Request>();
        req->set_seq(i);
        std::string serialized;
        if (!req->SerializeToString(&serialized)) {
            assert(false);
        }
        logEntry.set_command(std::move(serialized));

        recovery.AddLogEntry(&logEntry);
    }
  }
  {
    std::vector<Request> list;
    RaftRecovery recovery(config_, &checkpoint_, &system_info_, nullptr);
    recovery.ReadLogs(
        [&](const RaftMetadata &data) {},
        [&](std::unique_ptr<Request> request) { list.push_back(*request); },
        nullptr);

    EXPECT_EQ(list.size(), entries_to_add);

    for (size_t i = 0; i < entries_to_add; ++i) {
      EXPECT_EQ(list[i].seq(), i);
    }
  }
}

}  // namespace raft
}  // namespace resdb

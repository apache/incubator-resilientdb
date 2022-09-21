#include "ordering/poc/pow/transaction_accessor.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <future>

#include "client/mock_resdb_txn_client.h"
#include "common/test/test_macros.h"
#include "config/resdb_config_utils.h"

namespace resdb {
namespace {

using ::resdb::testing::EqualsProto;
using ::testing::_;
using ::testing::Invoke;
using ::testing::Pointee;
using ::testing::Return;

ResConfigData GetConfigData(const std::vector<ReplicaInfo>& replicas) {
  ResConfigData config_data;
  auto region = config_data.add_region();
  region->set_region_id(1);
  for (auto replica : replicas) {
    *region->add_replica_info() = replica;
  }
  config_data.set_self_region_id(1);
  return config_data;
}
class MockTransactionAccessor : public TransactionAccessor {
 public:
  MockTransactionAccessor(const ResDBPoCConfig& config)
      : TransactionAccessor(config, false) {}
  MOCK_METHOD(std::unique_ptr<ResDBTxnClient>, GetResDBTxnClient, (),
              (override));
};

ResDBPoCConfig GetConfig() {
  ResDBConfig bft_config({GenerateReplicaInfo(1, "127.0.0.1", 2001),
                          GenerateReplicaInfo(2, "127.0.0.1", 2002),
                          GenerateReplicaInfo(3, "127.0.0.1", 2003),
                          GenerateReplicaInfo(4, "127.0.0.1", 2004)},
                         GenerateReplicaInfo(1, "127.0.0.1", 2001), KeyInfo(),
                         CertificateInfo());

  return ResDBPoCConfig(
      bft_config,
      GetConfigData({GenerateReplicaInfo(1, "127.0.0.1", 1234),
                     GenerateReplicaInfo(2, "127.0.0.1", 1235),
                     GenerateReplicaInfo(3, "127.0.0.1", 1236),
                     GenerateReplicaInfo(4, "127.0.0.1", 1237)}),
      GenerateReplicaInfo(1, "127.0.0.1", 1234), KeyInfo(), CertificateInfo());
}

TEST(TransactionAccessorTest, GetTransactionsFail) {
  std::promise<bool> cli_done;
  std::future<bool> cli_done_future = cli_done.get_future();

  ResDBPoCConfig config = GetConfig();
  config.SetBatchTransactionNum(1);
  QueryRequest request;
  request.set_min_seq(1);
  request.set_max_seq(1);
  MockTransactionAccessor accessor(config);
  EXPECT_CALL(accessor, GetResDBTxnClient).WillRepeatedly(Invoke([&]() {
    auto client = std::make_unique<MockResDBTxnClient>(*config.GetBFTConfig());
    EXPECT_CALL(*client, GetTxn(1, 1)).WillOnce(Invoke([&]() {
      cli_done.set_value(true);
      return absl::InternalError("recv data fail.");
    }));
    return client;
  }));
  accessor.Start();
  cli_done_future.get();
  std::unique_ptr<BatchClientTransactions> resp =
      accessor.ConsumeTransactions(0);
  EXPECT_EQ(resp, nullptr);
}

TEST(TransactionAccessorTest, GetTransactions) {
  std::promise<bool> cli_done;
  std::future<bool> cli_done_future = cli_done.get_future();
  ResDBPoCConfig config = GetConfig();
  config.SetBatchTransactionNum(1);

  ClientTransactions expected_resp;
  expected_resp.set_seq(1);
  expected_resp.set_transaction_data("test");

  BatchClientTransactions expected_batch_txn;
  *expected_batch_txn.add_transactions() = expected_resp;
  expected_batch_txn.set_min_seq(1);
  expected_batch_txn.set_max_seq(1);

  QueryRequest request;
  request.set_min_seq(1);
  request.set_max_seq(1);
  MockTransactionAccessor accessor(config);
  EXPECT_CALL(accessor, GetResDBTxnClient).WillRepeatedly(Invoke([&]() {
    auto client = std::make_unique<MockResDBTxnClient>(*config.GetBFTConfig());
    ON_CALL(*client, GetTxn(1, 1)).WillByDefault(Invoke([&]() {
      std::vector<std::pair<uint64_t, std::string>> resp;
      resp.push_back(std::make_pair(expected_resp.seq(),
                                    expected_resp.transaction_data()));
      cli_done.set_value(true);
      return resp;
    }));
    ON_CALL(*client, GetTxn(2, 2)).WillByDefault(Invoke([&]() {
      return absl::InternalError("recv data fail.");
    }));

    return client;
  }));
  accessor.Start();

  cli_done_future.get();
  while (true) {
    std::unique_ptr<BatchClientTransactions> resp =
        accessor.ConsumeTransactions(1);
    if (resp == nullptr) {
      continue;
    }
    EXPECT_THAT(resp, Pointee(EqualsProto(expected_batch_txn)));
    break;
  }
}

}  // namespace
}  // namespace resdb

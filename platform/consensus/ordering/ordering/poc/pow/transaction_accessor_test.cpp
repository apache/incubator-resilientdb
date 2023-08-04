/*
 * Copyright (c) 2019-2022 ExpoLab, UC Davis
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "platform/consensus/ordering/poc/pow/transaction_accessor.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <future>

#include "common/test/test_macros.h"
#include "interface/common/mock_resdb_txn_accessor.h"
#include "platform/config/resdb_config_utils.h"

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
  MOCK_METHOD(std::unique_ptr<ResDBTxnAccessor>, GetResDBTxnAccessor, (),
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
  EXPECT_CALL(accessor, GetResDBTxnAccessor).WillRepeatedly(Invoke([&]() {
    auto client =
        std::make_unique<MockResDBTxnAccessor>(*config.GetBFTConfig());
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
  EXPECT_CALL(accessor, GetResDBTxnAccessor).WillRepeatedly(Invoke([&]() {
    auto client =
        std::make_unique<MockResDBTxnAccessor>(*config.GetBFTConfig());
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
    for (auto& txn : *resp->mutable_transactions()) {
      EXPECT_TRUE(txn.create_time() > 0);
      txn.clear_create_time();
    }
    EXPECT_THAT(resp, Pointee(EqualsProto(expected_batch_txn)));
    break;
  }
}

}  // namespace
}  // namespace resdb

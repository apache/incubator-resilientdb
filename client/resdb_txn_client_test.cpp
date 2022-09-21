#include "client/resdb_txn_client.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "client/mock_resdb_client.h"
#include "common/test/test_macros.h"
#include "config/resdb_config_utils.h"

namespace resdb {
namespace {

using ::resdb::testing::EqualsProto;
using ::testing::_;
using ::testing::ElementsAre;
using ::testing::Invoke;
using ::testing::Pointee;
using ::testing::Return;

class MockResDBTxnClient : public ResDBTxnClient {
 public:
  MockResDBTxnClient(const ResDBConfig& config) : ResDBTxnClient(config) {}
  MOCK_METHOD(std::unique_ptr<ResDBClient>, GetResDBClient,
              (const std::string&, int), (override));
};

TEST(ResDBTxnClientTest, GetTransactionsFail) {
  ResDBConfig config({GenerateReplicaInfo(1, "127.0.0.1", 1234),
                      GenerateReplicaInfo(2, "127.0.0.1", 1235),
                      GenerateReplicaInfo(3, "127.0.0.1", 1236),
                      GenerateReplicaInfo(4, "127.0.0.1", 1237)},
                     GenerateReplicaInfo(1, "127.0.0.1", 1234));

  QueryRequest request;
  request.set_min_seq(0);
  request.set_max_seq(0);
  MockResDBTxnClient client(config);
  EXPECT_CALL(client, GetResDBClient)
      .Times(4)
      .WillRepeatedly(Invoke([&](const std::string& ip, int port) {
        auto client = std::make_unique<MockResDBClient>(ip, port);
        EXPECT_CALL(*client,
                    SendRequest(EqualsProto(request), Request::TYPE_QUERY, _))
            .WillOnce(Return(0));
        EXPECT_CALL(*client, RecvRawMessageStr)
            .WillOnce(Invoke([&](std::string* resp) { return -1; }));
        return client;
      }));
  absl::StatusOr<std::vector<std::pair<uint64_t, std::string>>> resp =
      client.GetTxn(0, 0);
  EXPECT_FALSE(resp.ok());
}

TEST(ResDBTxnClientTest, GetTransactions) {
  ResDBConfig config({GenerateReplicaInfo(1, "127.0.0.1", 1234),
                      GenerateReplicaInfo(2, "127.0.0.1", 1235),
                      GenerateReplicaInfo(3, "127.0.0.1", 1236),
                      GenerateReplicaInfo(4, "127.0.0.1", 1237)},
                     GenerateReplicaInfo(1, "127.0.0.1", 1234));

  QueryResponse query_resp;
  auto txn = query_resp.add_transactions();
  txn->set_seq(1);
  txn->set_data("test_resp");

  QueryRequest request;
  request.set_min_seq(1);
  request.set_max_seq(1);
  MockResDBTxnClient client(config);
  EXPECT_CALL(client, GetResDBClient)
      .Times(4)
      .WillRepeatedly(Invoke([&](const std::string& ip, int port) {
        auto client = std::make_unique<MockResDBClient>(ip, port);
        EXPECT_CALL(*client,
                    SendRequest(EqualsProto(request), Request::TYPE_QUERY, _))
            .WillOnce(Return(0));

        EXPECT_CALL(*client, RecvRawMessageStr)
            .WillOnce(Invoke([&](std::string* resp) {
              query_resp.SerializeToString(resp);
              return 0;
            }));
        return client;
      }));

  absl::StatusOr<std::vector<std::pair<uint64_t, std::string>>> resp =
      client.GetTxn(1, 1);
  EXPECT_TRUE(resp.ok());
  EXPECT_THAT(*resp, ElementsAre(std::make_pair(1, "test_resp")));
}

TEST(ResDBTxnClientTest, GetTransactionsOneDelay) {
  ResDBConfig config({GenerateReplicaInfo(1, "127.0.0.1", 1234),
                      GenerateReplicaInfo(2, "127.0.0.1", 1235),
                      GenerateReplicaInfo(3, "127.0.0.1", 1236),
                      GenerateReplicaInfo(4, "127.0.0.1", 1237)},
                     GenerateReplicaInfo(1, "127.0.0.1", 1234));

  QueryResponse query_resp;
  auto txn = query_resp.add_transactions();
  txn->set_seq(1);
  txn->set_data("test_resp");

  QueryRequest request;
  request.set_min_seq(1);
  request.set_max_seq(1);
  MockResDBTxnClient client(config);
  EXPECT_CALL(client, GetResDBClient)
      .Times(4)
      .WillRepeatedly(Invoke([&](const std::string& ip, int port) {
        auto client = std::make_unique<MockResDBClient>(ip, port);
        EXPECT_CALL(*client,
                    SendRequest(EqualsProto(request), Request::TYPE_QUERY, _))
            .WillOnce(Return(0));

        if (port == 1237) {
          EXPECT_CALL(*client, RecvRawMessageStr)
              .WillOnce(Invoke([&](std::string* resp) {
                sleep(2);
                return 0;
              }));
        } else {
          EXPECT_CALL(*client, RecvRawMessageStr)
              .WillOnce(Invoke([&](std::string* resp) {
                query_resp.SerializeToString(resp);
                return 0;
              }));
        }
        return client;
      }));

  absl::StatusOr<std::vector<std::pair<uint64_t, std::string>>> resp =
      client.GetTxn(1, 1);
  EXPECT_TRUE(resp.ok());
  EXPECT_THAT(*resp, ElementsAre(std::make_pair(1, "test_resp")));
}

}  // namespace
}  // namespace resdb

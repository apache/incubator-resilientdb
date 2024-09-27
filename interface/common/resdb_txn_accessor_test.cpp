/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "interface/common/resdb_txn_accessor.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "common/test/test_macros.h"
#include "interface/rdbc/mock_net_channel.h"
#include "platform/config/resdb_config_utils.h"

namespace resdb {
namespace {

using ::resdb::testing::EqualsProto;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::ElementsAre;
using ::testing::Invoke;
using ::testing::Pointee;
using ::testing::Return;

class MockResDBTxnAccessor : public ResDBTxnAccessor {
 public:
  MockResDBTxnAccessor(const ResDBConfig& config) : ResDBTxnAccessor(config) {}
  MOCK_METHOD(std::unique_ptr<NetChannel>, GetNetChannel,
              (const std::string&, int), (override));
};

TEST(ResDBTxnAccessorTest, GetTransactionsFail) {
  ResDBConfig config({GenerateReplicaInfo(1, "127.0.0.1", 1234),
                      GenerateReplicaInfo(2, "127.0.0.1", 1235),
                      GenerateReplicaInfo(3, "127.0.0.1", 1236),
                      GenerateReplicaInfo(4, "127.0.0.1", 1237)},
                     GenerateReplicaInfo(1, "127.0.0.1", 1234));

  QueryRequest request;
  request.set_min_seq(0);
  request.set_max_seq(0);
  MockResDBTxnAccessor client(config);
  EXPECT_CALL(client, GetNetChannel)
      .Times(4)
      .WillRepeatedly(Invoke([&](const std::string& ip, int port) {
        auto client = std::make_unique<MockNetChannel>(ip, port);
        EXPECT_CALL(*client,
                    SendRequest(EqualsProto(request), Request::TYPE_QUERY, _))
            .Times(AtLeast(1))
            .WillRepeatedly(Return(0));
        EXPECT_CALL(*client, RecvRawMessageStr)
            .Times(AtLeast(1))
            .WillRepeatedly(Invoke([&](std::string* resp) { return -1; }));
        return client;
      }));
  absl::StatusOr<std::vector<std::pair<uint64_t, std::string>>> resp =
      client.GetTxn(0, 0);
  EXPECT_FALSE(resp.ok());
}

TEST(ResDBTxnAccessorTest, GetTransactions) {
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
  MockResDBTxnAccessor client(config);
  EXPECT_CALL(client, GetNetChannel)
      .Times(4)
      .WillRepeatedly(Invoke([&](const std::string& ip, int port) {
        auto client = std::make_unique<MockNetChannel>(ip, port);
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

TEST(ResDBTxnAccessorTest, GetTransactionsOneDelay) {
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
  MockResDBTxnAccessor client(config);
  EXPECT_CALL(client, GetNetChannel)
      .Times(4)
      .WillRepeatedly(Invoke([&](const std::string& ip, int port) {
        auto client = std::make_unique<MockNetChannel>(ip, port);
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

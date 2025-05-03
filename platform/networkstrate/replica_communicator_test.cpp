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

#include "platform/networkstrate/replica_communicator.h"

#include <gtest/gtest.h>

#include <future>

#include "interface/rdbc/mock_net_channel.h"
#include "platform/common/network/mock_socket.h"
#include "platform/networkstrate/mock_async_replica_client.h"

namespace resdb {
namespace {

using ::testing::Invoke;
using ::testing::Return;

ReplicaInfo GenerateReplicaInfo(const std::string& ip, int port) {
  ReplicaInfo replica_info;
  replica_info.set_ip(ip);
  replica_info.set_port(port);
  return replica_info;
}

class MockReplicaCommunicator : public ReplicaCommunicator {
 public:
  MockReplicaCommunicator(const std::vector<ReplicaInfo>& replicas,
                          bool use_lonn_conn = false)
      : ReplicaCommunicator(replicas, nullptr, use_lonn_conn){};
  MOCK_METHOD(std::unique_ptr<NetChannel>, GetClient, (const std::string&, int),
              (override));
  MOCK_METHOD(AsyncReplicaClient*, GetClientFromPool, (const std::string&, int),
              (override));
};

TEST(ReplicaCommunicatorTest, SendMessage) {
  std::vector<ReplicaInfo> replicas;
  replicas.push_back(GenerateReplicaInfo("127.0.0.1", 1234));
  replicas.push_back(GenerateReplicaInfo("127.0.0.1", 1235));

  MockReplicaCommunicator client(replicas);
  EXPECT_CALL(client, GetClient("127.0.0.1", 1234))
      .WillOnce(Invoke([&](const std::string& ip, int port) {
        return std::make_unique<MockNetChannel>(ip, port);
      }));
  EXPECT_CALL(client, GetClient("127.0.0.1", 1235))
      .WillOnce(Invoke([&](const std::string& ip, int port) {
        return std::make_unique<MockNetChannel>(ip, port);
      }));

  Request expected_request;
  expected_request.set_type(Request::TYPE_HEART_BEAT);
  EXPECT_EQ(client.SendMessage(expected_request), 2);
}

TEST(ReplicaCommunicatorTest, SendMessageToClient) {
  std::vector<ReplicaInfo> replicas;
  replicas.push_back(GenerateReplicaInfo("127.0.0.1", 1234));
  replicas.push_back(GenerateReplicaInfo("127.0.0.1", 1235));

  MockReplicaCommunicator client(replicas);

  std::vector<ReplicaInfo> client_replicas;
  client_replicas.push_back(GenerateReplicaInfo("127.0.0.1", 1236));
  client_replicas.back().set_id(3);

  client.UpdateClientReplicas(client_replicas);

  EXPECT_CALL(client, GetClient("127.0.0.1", 1236))
      .WillOnce(Invoke([&](const std::string& ip, int port) {
        return std::make_unique<MockNetChannel>(ip, port);
      }));

  Request expected_request;
  expected_request.set_type(Request::TYPE_HEART_BEAT);
  client.SendMessage(expected_request, 3);
}

TEST(ReplicaCommunicatorTest, PartialSendMessage) {
  std::vector<ReplicaInfo> replicas;
  replicas.push_back(GenerateReplicaInfo("127.0.0.1", 1234));
  replicas.push_back(GenerateReplicaInfo("127.0.0.1", 1235));

  MockReplicaCommunicator client(replicas);
  EXPECT_CALL(client, GetClient("127.0.0.1", 1234))
      .WillOnce(Invoke([&](const std::string& ip, int port) {
        return std::make_unique<MockNetChannel>(ip, port);
      }));
  EXPECT_CALL(client, GetClient("127.0.0.1", 1235))
      .WillOnce(
          Invoke([&](const std::string& ip, int port) { return nullptr; }));

  Request expected_request;
  expected_request.set_type(Request::TYPE_HEART_BEAT);
  EXPECT_EQ(client.SendMessage(expected_request), 1);
}

TEST(ReplicaCommunicatorTest, Lonnconnection) {
  std::promise<bool> bc;
  std::future<bool> bc_done = bc.get_future();
  std::vector<ReplicaInfo> replicas;
  replicas.push_back(GenerateReplicaInfo("127.0.0.1", 1234));

  boost::asio::io_service io_service;
  auto resdb_client = std::make_unique<MockAsyncReplicaClient>(&io_service);
  MockReplicaCommunicator client(replicas, true);
  EXPECT_CALL(client, GetClientFromPool("127.0.0.1", 1234))
      .WillOnce(Invoke([&](const std::string& ip, int port) {
        bc.set_value(true);
        return resdb_client.get();
      }));

  Request expected_request;
  expected_request.set_type(Request::TYPE_HEART_BEAT);
  EXPECT_EQ(client.SendMessage(expected_request), 0);
  bc_done.get();
}

}  // namespace

}  // namespace resdb

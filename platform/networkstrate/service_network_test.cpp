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

#include "platform/networkstrate/service_network.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <future>
#include <thread>

#include "platform/common/network/tcp_socket.h"
#include "platform/networkstrate/mock_service_interface.h"

namespace resdb {

using ::testing::Invoke;
using ::testing::Return;

ResDBConfig GenerateDBConfig() {
  std::vector<ReplicaInfo> replicas;
  ReplicaInfo self_info;
  self_info.set_ip("127.0.0.1");
  self_info.set_port(1234);
  ReplicaInfo dest_info;
  dest_info.set_ip("127.0.0.1");
  dest_info.set_port(1235);
  replicas.push_back(dest_info);

  return ResDBConfig(replicas, self_info, KeyInfo(), CertificateInfo());
}

void SendData(const std::string& data) {
  TcpSocket client_socket;
  int ret = client_socket.Connect("127.0.0.1", 1234);
  ASSERT_EQ(ret, 0);
  ret = client_socket.Send(data);
  ASSERT_EQ(ret, 0);
}

TEST(ServiceNetworkTest, RecvData) {
  std::promise<bool> init;
  std::future<bool> init_done = init.get_future();

  std::promise<bool> recv;
  std::future<bool> recv_done = recv.get_future();

  std::unique_ptr<MockServiceInterface> service =
      std::make_unique<MockServiceInterface>();
  bool finished = false;
  EXPECT_CALL(*service, IsRunning).WillRepeatedly(Invoke([&]() {
    return !finished;
  }));

  EXPECT_CALL(*service, Process)
      .WillOnce(Invoke([&](std::unique_ptr<Context>,
                           std::unique_ptr<DataInfo> request_info) {
        EXPECT_EQ(
            std::string((char*)request_info->buff, request_info->data_len),
            "test");
        recv.set_value(true);
        return 0;
      }));

  std::thread svr_thead2 = std::thread([&]() {
    ServiceNetwork server(GenerateDBConfig(), std::move(service));
    init.set_value(true);
    server.Run();
  });
  init_done.get();
  SendData("test");
  recv_done.get();
  finished = true;
  svr_thead2.join();
}

TEST(ServiceNetworkTest, RunningDone) {
  std::unique_ptr<MockServiceInterface> service =
      std::make_unique<MockServiceInterface>();
  bool finished = false;
  EXPECT_CALL(*service, IsRunning).WillRepeatedly(Invoke([&]() {
    return !finished;
  }));

  EXPECT_CALL(*service, Process).Times(0);

  std::promise<bool> init;
  std::future<bool> init_done = init.get_future();

  std::thread svr_thead2 = std::thread([&]() {
    ServiceNetwork server(GenerateDBConfig(), std::move(service));
    init.set_value(true);
    server.Run();
  });
  init_done.get();
  finished = true;
  svr_thead2.join();
}

}  // namespace resdb

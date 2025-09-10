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

#include "platform/common/network/tcp_socket.h"

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <future>
#include <thread>

namespace resdb {

TEST(TcpSocket, SendAndRecv) {
  std::promise<bool> svr_done, cli_done;
  std::future<bool> svr_done_future = svr_done.get_future(),
                    cli_done_future = cli_done.get_future();

  std::thread svr_thead = std::thread([&]() {
    TcpSocket svr_socket;
    svr_socket.Listen("127.0.0.1", 1234);
    svr_done.set_value(true);
    auto client_socket = svr_socket.Accept();
    char *buf = nullptr;
    size_t len = 0;
    int ret = client_socket->Recv((void **)&buf, &len);
    EXPECT_EQ(ret, 4);
    EXPECT_EQ(std::string(buf, len), "test");
    free(buf);

    ret = client_socket->Send("recv_suc");
    EXPECT_EQ(ret, 0);

    cli_done_future.get();
  });

  svr_done_future.get();
  // Sever is ready, start the client.

  TcpSocket client_socket;
  int ret = client_socket.Connect("127.0.0.1", 1234);
  ASSERT_EQ(ret, 0);
  ret = client_socket.Send("test");
  ASSERT_EQ(ret, 0);

  char *buf = nullptr;
  size_t len = 0;
  ret = client_socket.Recv((void **)&buf, &len);
  EXPECT_EQ(ret, 8);
  EXPECT_EQ(std::string(buf, len), "recv_suc");
  free(buf);

  cli_done.set_value(true);
  svr_thead.join();
}

TEST(TcpSocket, SendAfterClose) {
  std::promise<bool> svr_done, cli_done;
  std::future<bool> svr_done_future = svr_done.get_future(),
                    cli_done_future = cli_done.get_future();

  std::thread svr_thead = std::thread([&]() {
    TcpSocket svr_socket;
    svr_socket.Listen("127.0.0.1", 1234);
    svr_done.set_value(true);
    cli_done_future.get();
  });

  svr_done_future.get();
  TcpSocket client_socket;
  client_socket.Connect("127.0.0.1", 1234);
  client_socket.Close();
  EXPECT_EQ(client_socket.Send("test"), -2);
  cli_done.set_value(true);
  svr_thead.join();
}

}  // namespace resdb

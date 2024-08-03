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

#include "platform/networkstrate/async_acceptor.h"

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <future>

#include "platform/common/network/tcp_socket.h"

namespace resdb {
namespace {

TEST(AsyncAcceptorTest, RecvMessage) {
  std::promise<bool> bc;
  std::future<bool> bc_done = bc.get_future();
  AsyncAcceptor acceptor(
      "127.0.0.1", 1234, 1,
      [&](const char* buff, size_t data_len) { bc.set_value(true); });

  acceptor.StartAccept();

  TcpSocket client_socket;
  int ret = client_socket.Connect("127.0.0.1", 1234);
  ASSERT_EQ(ret, 0);
  ret = client_socket.Send("test");
  ASSERT_EQ(ret, 0);
  bc_done.get();
}

TEST(AsyncAcceptorTest, RecvMessageAndClose) {
  std::promise<bool> bc;
  std::future<bool> bc_done = bc.get_future();
  AsyncAcceptor acceptor(
      "127.0.0.1", 1234, 1,
      [&](const char* buff, size_t data_len) { bc.set_value(true); });

  acceptor.StartAccept();

  TcpSocket client_socket;
  int ret = client_socket.Connect("127.0.0.1", 1234);
  ASSERT_EQ(ret, 0);
  ret = client_socket.Send("test");
  client_socket.Close();
  ASSERT_EQ(ret, 0);
  bc_done.get();
}

TEST(AsyncAcceptorTest, MultiAcceptor) {
  std::promise<bool> bc;
  std::future<bool> bc_done = bc.get_future();
  int a = 0;
  AsyncAcceptor acceptor("127.0.0.1", 1234, 2,
                         [&](const char* buff, size_t data_len) {
                           a++;
                           if (a == 3) bc.set_value(true);
                         });

  acceptor.StartAccept();

  for (int i = 0; i < 3; ++i) {
    TcpSocket client_socket;
    int ret = client_socket.Connect("127.0.0.1", 1234);
    ASSERT_EQ(ret, 0);
    ret = client_socket.Send("test");
    client_socket.Close();
    ASSERT_EQ(ret, 0);
  }
  bc_done.get();
}

TEST(AsyncAcceptorTest, MultiAcceptorError) {
  std::promise<bool> bc;
  std::future<bool> bc_done = bc.get_future();
  AsyncAcceptor acceptor("127.0.0.1", 1234, 2,
                         [&](const char* buff, size_t data_len) {});

  acceptor.StartAccept();

  for (int i = 0; i < 3; ++i) {
    TcpSocket client_socket;
    int ret = client_socket.Connect("127.0.0.1", 1234);
    ret = client_socket.Send("test");
    ASSERT_EQ(ret, 0);
    client_socket.Close();
  }
}

TEST(AsyncAcceptorTest, RecvClose) {
  std::promise<bool> bc;
  std::future<bool> bc_done = bc.get_future();
  AsyncAcceptor acceptor(
      "127.0.0.1", 1234, 1,
      [&](const char* buff, size_t data_len) { bc.set_value(true); });

  acceptor.StartAccept();

  TcpSocket client_socket;
  int ret = client_socket.Connect("127.0.0.1", 1234);
  ASSERT_EQ(ret, 0);
  client_socket.Close();

  {
    TcpSocket client_socket;
    int ret = client_socket.Connect("127.0.0.1", 1234);
    ASSERT_EQ(ret, 0);
    ret = client_socket.Send("test");
    ASSERT_EQ(ret, 0);
    client_socket.Close();
  }

  bc_done.get();
}

}  // namespace

}  // namespace resdb

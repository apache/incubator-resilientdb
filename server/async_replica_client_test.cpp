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

#include "server/async_replica_client.h"

#include <gtest/gtest.h>

#include <future>

#include "common/network/tcp_socket.h"

namespace resdb {
namespace {

TEST(AsyncReplicaClientTest, SendMessage) {
  std::promise<bool> bc;
  std::future<bool> bc_done = bc.get_future();

  std::thread svr_thead = std::thread([&]() {
    TcpSocket svr_socket;
    svr_socket.Listen("127.0.0.1", 1234);
    auto client_socket = svr_socket.Accept();
    char *buf = nullptr;
    size_t len = 0;
    int ret = client_socket->Recv((void **)&buf, &len);
    EXPECT_EQ(ret, 4);
    EXPECT_EQ(std::string(buf, len), "test");
    bc.set_value(true);
    free(buf);
    client_socket->Close();
    svr_socket.Close();
  });

  boost::asio::io_service io_service;
  std::thread t([&]() { io_service.run(); });

  boost::asio::io_service::work *work =
      new boost::asio::io_service::work(io_service);
  AsyncReplicaClient client(&io_service, "127.0.0.1", 1234);
  EXPECT_EQ(client.SendMessage("test"), 0);
  bc_done.get();
  delete work;
  work = nullptr;
  svr_thead.join();
  t.join();
}

TEST(AsyncReplicaClientTest, SendLargeMessage) {
  std::promise<bool> bc;
  std::future<bool> bc_done = bc.get_future();

  std::thread svr_thead = std::thread([&]() {
    TcpSocket svr_socket;
    svr_socket.Listen("127.0.0.1", 1234);
    auto client_socket = svr_socket.Accept();
    char *buf = nullptr;
    size_t len = 0;
    int ret = client_socket->Recv((void **)&buf, &len);
    EXPECT_EQ(ret, 1000000);
    EXPECT_EQ(std::string(buf, len), std::string(1000000, 't'));
    bc.set_value(true);
    free(buf);
    client_socket->Close();
    svr_socket.Close();
  });

  boost::asio::io_service io_service;
  std::thread t([&]() { io_service.run(); });

  boost::asio::io_service::work *work =
      new boost::asio::io_service::work(io_service);
  AsyncReplicaClient client(&io_service, "127.0.0.1", 1234);
  EXPECT_EQ(client.SendMessage(std::string(1000000, 't')), 0);
  bc_done.get();
  delete work;
  work = nullptr;
  svr_thead.join();
  t.join();
}

TEST(AsyncReplicaClientTest, MultiSendMessage) {
  std::promise<bool> bc;
  std::future<bool> bc_done = bc.get_future();

  std::thread svr_thead = std::thread([&]() {
    TcpSocket svr_TcpSocket;
    svr_TcpSocket.Listen("127.0.0.1", 1234);
    auto client_socket = svr_TcpSocket.Accept();
    char *buf = nullptr;
    size_t len = 0;
    for (int i = 0; i < 10; ++i) {
      int ret = client_socket->Recv((void **)&buf, &len);
      EXPECT_EQ(ret, 4);
      EXPECT_EQ(std::string(buf, len), "test");
    }
    bc.set_value(true);
    free(buf);
  });

  boost::asio::io_service io_service;
  std::thread t([&]() { io_service.run(); });
  boost::asio::io_service::work *work =
      new boost::asio::io_service::work(io_service);
  AsyncReplicaClient client(&io_service, "127.0.0.1", 1234);
  for (int i = 0; i < 10; ++i) {
    EXPECT_EQ(client.SendMessage("test"), 0);
  }
  bc_done.get();
  delete work;
  work = nullptr;
  svr_thead.join();
  t.join();
}

TEST(AsyncReplicaClientTest, Reconnect) {
  std::promise<bool> bc1, bc2;
  std::future<bool> bc1_done = bc1.get_future(), bc2_done = bc2.get_future();

  std::thread svr_thead = std::thread([&]() {
    TcpSocket svr_TcpSocket;
    svr_TcpSocket.Listen("127.0.0.1", 1234);
    for (int j = 0; j < 2; ++j) {
      auto client_socket = svr_TcpSocket.Accept();
      char *buf = nullptr;
      size_t len = 0;
      int ret = client_socket->Recv((void **)&buf, &len);
      EXPECT_EQ(ret, 4);
      EXPECT_EQ(std::string(buf, len), "test");
      client_socket->Close();
      free(buf);
      if (j == 0) bc2.set_value(true);
    }
    bc1.set_value(true);
  });

  boost::asio::io_service io_service;
  std::thread t([&]() { io_service.run(); });
  boost::asio::io_service::work *work =
      new boost::asio::io_service::work(io_service);
  AsyncReplicaClient client(&io_service, "127.0.0.1", 1234);
  for (int i = 0; i < 2; ++i) {
    EXPECT_EQ(client.SendMessage("test"), 0);
    if (i == 0) bc2_done.get();
  }
  bc1_done.get();
  delete work;
  work = nullptr;
  svr_thead.join();
  t.join();
}

}  // namespace

}  // namespace resdb

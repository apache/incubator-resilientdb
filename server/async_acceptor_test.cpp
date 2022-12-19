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

#include "server/async_acceptor.h"

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <future>

#include "common/network/tcp_socket.h"

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

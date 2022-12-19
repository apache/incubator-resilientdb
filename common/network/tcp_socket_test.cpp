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

#include "common/network/tcp_socket.h"

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

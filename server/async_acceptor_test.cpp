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

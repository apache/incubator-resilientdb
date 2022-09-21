#include "server/resdb_server.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <future>
#include <thread>

#include "common/network/tcp_socket.h"
#include "server/mock_resdb_service.h"

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

TEST(ResDBServerTest, RecvData) {
  std::promise<bool> init;
  std::future<bool> init_done = init.get_future();

  std::promise<bool> recv;
  std::future<bool> recv_done = recv.get_future();

  std::unique_ptr<MockResDBService> service =
      std::make_unique<MockResDBService>();
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
    ResDBServer server(GenerateDBConfig(), std::move(service));
    init.set_value(true);
    server.Run();
  });
  init_done.get();
  SendData("test");
  recv_done.get();
  finished = true;
  svr_thead2.join();
}

TEST(ResDBServerTest, RunningDone) {
  std::unique_ptr<MockResDBService> service =
      std::make_unique<MockResDBService>();
  bool finished = false;
  EXPECT_CALL(*service, IsRunning).WillRepeatedly(Invoke([&]() {
    return !finished;
  }));

  EXPECT_CALL(*service, Process).Times(0);

  std::promise<bool> init;
  std::future<bool> init_done = init.get_future();

  std::thread svr_thead2 = std::thread([&]() {
    ResDBServer server(GenerateDBConfig(), std::move(service));
    init.set_value(true);
    server.Run();
  });
  init_done.get();
  finished = true;
  svr_thead2.join();
}

}  // namespace resdb

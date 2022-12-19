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

#include "server/consensus_service.h"

#include <gtest/gtest.h>

#include <future>

#include "common/test/test_macros.h"
#include "server/mock_resdb_replica_client.h"

namespace resdb {
namespace {

using ::resdb::testing::EqualsProto;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::Invoke;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::Test;

ReplicaInfo GenerateReplicaInfo(const std::string& ip, int port) {
  ReplicaInfo info;
  info.set_ip(ip);
  info.set_port(port);
  return info;
}

class MockConsensusService : public ConsensusService {
 public:
  MockConsensusService(const ResDBConfig& config) : ConsensusService(config) {}

  MOCK_METHOD(std::unique_ptr<ResDBReplicaClient>, GetReplicaClient,
              (const std::vector<ReplicaInfo>&, bool), (override));
  MOCK_METHOD(int, ConsensusCommit,
              (std::unique_ptr<Context>, std::unique_ptr<Request>), (override));
  MOCK_METHOD(std::vector<ReplicaInfo>, GetReplicas, (), (override));
  MOCK_METHOD(std::vector<ReplicaInfo>, GetClientReplicas, (), (override));

 public:
  void BroadCast(const Request& request) {
    return ConsensusService::BroadCast(request);
  }
  void UpdateBroadCastClient() {
    return ConsensusService::UpdateBroadCastClient();
  }

  ResDBReplicaClient* GetBroadCastClient() {
    return ConsensusService::GetBroadCastClient();
  }

 public:
  int Dispatch(std::unique_ptr<Context> context,
               std::unique_ptr<Request> request) {
    return ConsensusService::Dispatch(std::move(context), std::move(request));
  }
};

class ConsensusServiceTest : public Test {
 public:
  ConsensusServiceTest()
      : replicas_({GenerateReplicaInfo("127.0.0.1", 1234),
                   GenerateReplicaInfo("127.0.0.1", 1235)}),
        self_info_(GenerateReplicaInfo("127.0.0.1", 10000)),
        config_(replicas_, self_info_, KeyInfo(), CertificateInfo()) {
    ResConfigData data;
    auto region = data.add_region();
    region->set_region_id(1);
    for (ReplicaInfo info : config_.GetReplicaInfos()) {
      *region->add_replica_info() = info;
    }
    config_.SetConfigData(data);

    Stats::GetGlobalStats(1);

    impl_ = std::make_unique<MockConsensusService>(config_);

    ON_CALL(*impl_, GetReplicas).WillByDefault(Return(replicas_));
  }

  ~ConsensusServiceTest() { impl_->Stop(); }

 protected:
  std::vector<ReplicaInfo> replicas_;
  ReplicaInfo self_info_;
  ResDBConfig config_;
  std::unique_ptr<MockConsensusService> impl_;
};

TEST_F(ConsensusServiceTest, SendHB) {
  std::promise<bool> hb;
  std::future<bool> hb_done = hb.get_future();
  EXPECT_CALL(*impl_, GetReplicaClient)
      .Times(AtLeast(1))
      .WillRepeatedly(
          Invoke([&](const std::vector<ReplicaInfo>& replicas, bool) {
            auto client = std::make_unique<MockResDBReplicaClient>(replicas);
            EXPECT_CALL(*client, SendHeartBeat)
                .WillRepeatedly(Invoke([&](const Request& hb_info) {
                  hb.set_value(true);
                  return 0;
                }));
            return client;
          }));
  impl_->Start();
  hb_done.get();
}

TEST_F(ConsensusServiceTest, SendHBToClient) {
  std::promise<bool> hb;
  std::future<bool> hb_done = hb.get_future();

  EXPECT_CALL(*impl_, GetClientReplicas)
      .Times(AtLeast(1))
      .WillRepeatedly(Invoke([&]() {
        ReplicaInfo client_info;
        client_info.set_ip("127.0.0.1");
        client_info.set_port(54321);
        return std::vector<ReplicaInfo>({client_info});
      }));

  EXPECT_CALL(*impl_, GetReplicaClient)
      .Times(AtLeast(1))
      .WillRepeatedly(
          Invoke([&](const std::vector<ReplicaInfo>& replicas, bool) {
            EXPECT_EQ(replicas.size(), 3);
            bool find = false;
            for (auto& info : replicas) {
              if (info.ip() == "127.0.0.1" && info.port() == 54321) {
                find = true;
              }
            }
            EXPECT_TRUE(find);
            auto client = std::make_unique<MockResDBReplicaClient>(replicas);
            EXPECT_CALL(*client, SendHeartBeat)
                .WillRepeatedly(Invoke([&](const Request& hb_info) {
                  hb.set_value(true);
                  return 0;
                }));
            return client;
          }));
  impl_->Start();
  hb_done.get();
}

TEST_F(ConsensusServiceTest, SendHBToClientWithTworegion) {
  ResConfigData data;
  auto region = data.add_region();
  region->set_region_id(1);
  *region->add_replica_info() = GenerateReplicaInfo("127.0.0.1", 1234);
  *region->add_replica_info() = GenerateReplicaInfo("127.0.0.1", 1235);

  region = data.add_region();
  region->set_region_id(2);
  *region->add_replica_info() = GenerateReplicaInfo("127.0.0.1", 1234);
  *region->add_replica_info() = GenerateReplicaInfo("127.0.0.1", 1235);
  config_.SetConfigData(data);
  impl_ = std::make_unique<MockConsensusService>(config_);

  std::promise<bool> hb;
  std::future<bool> hb_done = hb.get_future();

  EXPECT_CALL(*impl_, GetClientReplicas)
      .Times(AtLeast(1))
      .WillRepeatedly(Invoke([&]() {
        ReplicaInfo client_info;
        client_info.set_ip("127.0.0.1");
        client_info.set_port(54321);
        return std::vector<ReplicaInfo>({client_info});
      }));

  EXPECT_CALL(*impl_, GetReplicaClient)
      .Times(AtLeast(1))
      .WillRepeatedly(
          Invoke([&](const std::vector<ReplicaInfo>& replicas, bool) {
            EXPECT_EQ(replicas.size(), 5);
            bool find = false;
            for (auto& info : replicas) {
              if (info.ip() == "127.0.0.1" && info.port() == 54321) {
                find = true;
              }
            }
            EXPECT_TRUE(find);
            auto client = std::make_unique<MockResDBReplicaClient>(replicas);
            EXPECT_CALL(*client, SendHeartBeat)
                .WillRepeatedly(Invoke([&](const Request& hb_info) {
                  hb.set_value(true);
                  return 0;
                }));
            return client;
          }));
  impl_->Start();
  hb_done.get();
}

TEST_F(ConsensusServiceTest, BroadCast) {
  std::promise<bool> bc;
  std::future<bool> bc_done = bc.get_future();
  EXPECT_CALL(*impl_, GetReplicaClient)
      .WillRepeatedly(
          Invoke([&](const std::vector<ReplicaInfo>& replicas, bool long_conn) {
            EXPECT_EQ(replicas.size(), replicas_.size());
            auto client = std::make_unique<MockResDBReplicaClient>(replicas);
            EXPECT_CALL(*client, SendMessage(_))
                .WillRepeatedly(
                    Invoke([&](const google::protobuf::Message& request) {
                      try {
                        bc.set_value(true);
                      } catch (...) {
                      }
                      return 0;
                    }));
            return client;
          }));

  impl_->Start();
  impl_->UpdateBroadCastClient();
  impl_->BroadCast(Request());
  bc_done.get();
}

TEST_F(ConsensusServiceTest, DiscoverNewClient) {
  config_.SetSignatureVerifierEnabled(false);
  impl_ = std::make_unique<MockConsensusService>(config_);
  impl_->Start();

  EXPECT_CALL(*impl_, GetReplicaClient)
      .WillRepeatedly(
          Invoke([&](const std::vector<ReplicaInfo>& replicas, bool long_conn) {
            EXPECT_EQ(replicas.size(), replicas_.size());
            auto client = std::make_unique<MockResDBReplicaClient>(replicas);
            return client;
          }));

  HeartBeatInfo hb;
  auto* public_key = hb.add_public_keys();

  public_key->mutable_public_key_info()->set_ip("127.0.0.1");
  public_key->mutable_public_key_info()->set_port(12345);
  public_key->mutable_public_key_info()->set_node_id(5);
  public_key->mutable_public_key_info()->set_type(CertificateKeyInfo::CLIENT);

  auto request = std::make_unique<Request>();
  request->set_type(Request::TYPE_HEART_BEAT);
  hb.SerializeToString(request->mutable_data());

  EXPECT_EQ(impl_->Dispatch(std::make_unique<Context>(), std::move(request)),
            0);

  ResDBReplicaClient* bc_client = impl_->GetBroadCastClient();
  EXPECT_NE(bc_client, nullptr);

  std::vector<ReplicaInfo> client_infos = bc_client->GetClientReplicas();
  EXPECT_EQ(client_infos.size(), 1);
}

TEST_F(ConsensusServiceTest, DispatchOK) {
  Request expected_request;
  expected_request.set_type(Request::TYPE_CLIENT_REQUEST);
  EXPECT_CALL(*impl_,
              ConsensusCommit(_, Pointee(EqualsProto(expected_request))))
      .Times(1);

  impl_->Start();
  {
    auto request = std::make_unique<Request>();
    request->set_type(Request::TYPE_HEART_BEAT);
    auto replica_info = GenerateReplicaInfo("127.0.0.1", 1234);
    replica_info.SerializeToString(request->mutable_data());

    EXPECT_EQ(impl_->Dispatch(std::make_unique<Context>(), std::move(request)),
              0);
  }

  {
    auto request = std::make_unique<Request>();
    request->set_type(Request::TYPE_CLIENT_REQUEST);
    EXPECT_EQ(impl_->Dispatch(std::make_unique<Context>(), std::move(request)),
              0);
  }
}

}  // namespace

}  // namespace resdb

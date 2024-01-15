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

#include "interface/common/resdb_state_accessor.h"

#include <glog/logging.h>
#include <gtest/gtest.h>

#include "common/test/test_macros.h"
#include "interface/rdbc/mock_net_channel.h"

namespace resdb {
namespace {

using ::google::protobuf::util::MessageDifferencer;
using ::resdb::testing::EqualsProto;
using ::testing::ElementsAre;
using ::testing::Invoke;
using ::testing::Test;

void AddReplicaToList(const std::string& ip, int port,
                      std::vector<ReplicaInfo>* replica) {
  ReplicaInfo info;
  info.set_ip(ip);
  info.set_port(port);
  replica->push_back(info);
}

class MockResDBStateAccessor : public ResDBStateAccessor {
 public:
  MockResDBStateAccessor(const ResDBConfig& config)
      : ResDBStateAccessor(config) {}

  MOCK_METHOD(std::unique_ptr<NetChannel>, GetNetChannel,
              (const std::string&, int), (override));
};

class StateClientTest : public Test {
 public:
  StateClientTest() {
    self_info_.set_ip("127.0.0.1");
    self_info_.set_port(1234);

    AddReplicaToList("127.0.0.1", 1235, &replicas_);
    AddReplicaToList("127.0.0.1", 1236, &replicas_);
    AddReplicaToList("127.0.0.1", 1237, &replicas_);
    AddReplicaToList("127.0.0.1", 1238, &replicas_);

    KeyInfo private_key;
    private_key.set_key("private_key");
    config_ = std::make_unique<ResDBConfig>(replicas_, self_info_, private_key,
                                            CertificateInfo());
  }

 protected:
  ReplicaInfo self_info_;
  std::vector<ReplicaInfo> replicas_;
  std::unique_ptr<ResDBConfig> config_;
};

TEST_F(StateClientTest, GetAllReplicaState) {
  MockResDBStateAccessor client(*config_);

  ReplicaState state;
  auto region = state.mutable_replica_config()->add_region();

  for (auto replica : replicas_) {
    *region->add_replica_info() = replica;
  }

  std::atomic<int> idx = 0;
  EXPECT_CALL(client, GetNetChannel)
      .Times(1)
      .WillRepeatedly(Invoke([&](const std::string& ip, int port) {
        auto client = std::make_unique<MockNetChannel>(ip, port);
        EXPECT_CALL(*client, RecvRawMessage)
            .WillRepeatedly(Invoke([&](google::protobuf::Message* message) {
              *reinterpret_cast<ReplicaState*>(message) = state;
              return 0;
            }));
        return client;
      }));
  auto ret = client.GetReplicaState();
  EXPECT_TRUE(ret.ok());
  EXPECT_THAT(state, EqualsProto(*ret));
}

}  // namespace

}  // namespace resdb

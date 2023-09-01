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

#include "interface/common/resdb_state_accessor.h"

#include <glog/logging.h>
#include <gtest/gtest.h>

#include "common/test/test_macros.h"
#include "interface/rdbc/mock_net_channel.h"

namespace resdb {
namespace {

using ::google::protobuf::util::MessageDifferencer;
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
  auto ret = client.GetReplicaStates();
  EXPECT_TRUE(ret.ok());
  std::set<int> results;
  for (auto& state : *ret) {
    auto it = std::find_if(
        replicas_.begin(), replicas_.end(), [&](const ReplicaInfo& info) {
          return MessageDifferencer::Equals(info, state.replica_info());
        });
    EXPECT_TRUE(it != replicas_.end());
    results.insert(it - replicas_.begin());
  }
  EXPECT_EQ(results.size(), 4);
}

}  // namespace

}  // namespace resdb

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

#include "config/resdb_config.h"

#include <google/protobuf/util/message_differencer.h>
#include <gtest/gtest.h>

#include <thread>

#include "common/test/test_macros.h"
#include "gmock/gmock.h"

namespace resdb {
namespace {

using ::google::protobuf::util::MessageDifferencer;
using ::resdb::testing::EqualsProto;

MATCHER_P(EqualsReplicas, replicas, "") {
  if (arg.size() != replicas.size()) {
    return false;
  }
  for (size_t i = 0; i < replicas.size(); ++i) {
    if (!MessageDifferencer::Equals(replicas[i], arg[i])) {
      return false;
    }
  }
  return true;
}

ReplicaInfo GenerateReplicaInfo(const std::string& ip, int port) {
  ReplicaInfo info;
  info.set_ip(ip);
  info.set_port(port);
  return info;
}

TEST(TcpSocket, ResDBConfig) {
  ReplicaInfo self_info = GenerateReplicaInfo("127.0.0.1", 1234);

  std::vector<ReplicaInfo> replicas;
  replicas.push_back(GenerateReplicaInfo("127.0.0,1", 1235));
  replicas.push_back(GenerateReplicaInfo("127.0.0,1", 1236));
  replicas.push_back(GenerateReplicaInfo("127.0.0,1", 1237));
  replicas.push_back(GenerateReplicaInfo("127.0.0,1", 1238));

  ResDBConfig config(replicas, self_info);

  EXPECT_THAT(config.GetReplicaInfos(), EqualsReplicas(replicas));
  EXPECT_THAT(config.GetSelfInfo(), EqualsProto(self_info));
  EXPECT_EQ(config.GetReplicaNum(), replicas.size());
  EXPECT_EQ(config.GetMinDataReceiveNum(), 3);
}

TEST(TcpSocket, ResDBConfigWith5Replicas) {
  ReplicaInfo self_info = GenerateReplicaInfo("127.0.0.1", 1234);

  std::vector<ReplicaInfo> replicas;
  replicas.push_back(GenerateReplicaInfo("127.0.0,1", 1235));
  replicas.push_back(GenerateReplicaInfo("127.0.0,1", 1236));
  replicas.push_back(GenerateReplicaInfo("127.0.0,1", 1237));
  replicas.push_back(GenerateReplicaInfo("127.0.0,1", 1238));
  replicas.push_back(GenerateReplicaInfo("127.0.0,1", 1239));

  ResDBConfig config(replicas, self_info);

  EXPECT_THAT(config.GetReplicaInfos(), EqualsReplicas(replicas));
  EXPECT_THAT(config.GetSelfInfo(), EqualsProto(self_info));
  EXPECT_EQ(config.GetReplicaNum(), replicas.size());
  EXPECT_EQ(config.GetMinDataReceiveNum(), 3);
}

TEST(TcpSocket, ResDBConfigWith6Replicas) {
  ReplicaInfo self_info = GenerateReplicaInfo("127.0.0.1", 1234);

  std::vector<ReplicaInfo> replicas;
  replicas.push_back(GenerateReplicaInfo("127.0.0,1", 1235));
  replicas.push_back(GenerateReplicaInfo("127.0.0,1", 1236));
  replicas.push_back(GenerateReplicaInfo("127.0.0,1", 1237));
  replicas.push_back(GenerateReplicaInfo("127.0.0,1", 1238));
  replicas.push_back(GenerateReplicaInfo("127.0.0,1", 1239));
  replicas.push_back(GenerateReplicaInfo("127.0.0,1", 1240));

  ResDBConfig config(replicas, self_info);

  EXPECT_THAT(config.GetReplicaInfos(), EqualsReplicas(replicas));
  EXPECT_THAT(config.GetSelfInfo(), EqualsProto(self_info));
  EXPECT_EQ(config.GetReplicaNum(), replicas.size());
  EXPECT_EQ(config.GetMinDataReceiveNum(), 3);
}

TEST(TcpSocket, ResDBConfigWith2Replicas) {
  ReplicaInfo self_info = GenerateReplicaInfo("127.0.0.1", 1234);

  std::vector<ReplicaInfo> replicas;
  replicas.push_back(GenerateReplicaInfo("127.0.0,1", 1235));
  replicas.push_back(GenerateReplicaInfo("127.0.0,1", 1236));

  ResDBConfig config(replicas, self_info);

  EXPECT_THAT(config.GetReplicaInfos(), EqualsReplicas(replicas));
  EXPECT_THAT(config.GetSelfInfo(), EqualsProto(self_info));
  EXPECT_EQ(config.GetReplicaNum(), replicas.size());
  EXPECT_EQ(config.GetMinDataReceiveNum(), 1);
}

}  // namespace

}  // namespace resdb

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

#include "platform/config/resdb_config.h"

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

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

#include "platform/consensus/ordering/raft/raft_rpc.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "platform/networkstrate/mock_replica_communicator.h"

namespace resdb {
namespace {

ResDBConfig MakeTestConfig() {
  ReplicaInfo self;
  self.set_id(1);
  self.set_ip("127.0.0.1");
  self.set_port(9000);
  std::vector<ReplicaInfo> replicas = {self};
  return ResDBConfig(replicas, self, ResConfigData());
}

TEST(RaftRpcTest, BroadcastWrapsRequest) {
  ResDBConfig config = MakeTestConfig();
  MockReplicaCommunicator communicator(config.GetReplicaInfos());
  RaftRpc rpc(config, &communicator);

  raft::AppendEntriesRequest append_entries;
  append_entries.set_term(5);
  append_entries.set_leader_id(1);

  EXPECT_CALL(communicator, BroadCast(testing::_))
      .WillOnce(testing::Invoke(
          [&](const google::protobuf::Message& message) {
            const Request& req = static_cast<const Request&>(message);
            EXPECT_EQ(req.type(), Request::TYPE_CUSTOM_CONSENSUS);
            EXPECT_EQ(req.user_type(),
                      raft::RAFT_APPEND_ENTRIES_REQUEST);
            raft::AppendEntriesRequest inner;
            ASSERT_TRUE(inner.ParseFromString(req.data()));
            EXPECT_EQ(inner.term(), 5);
          }));
  EXPECT_TRUE(rpc.BroadcastAppendEntries(append_entries).ok());
}

TEST(RaftRpcTest, SendRequestVoteUsesReplica) {
  ResDBConfig config = MakeTestConfig();
  MockReplicaCommunicator communicator(config.GetReplicaInfos());
  RaftRpc rpc(config, &communicator);

  ReplicaInfo target;
  target.set_id(2);
  target.set_ip("10.0.0.2");
  target.set_port(9001);

  raft::RequestVoteRequest vote;
  vote.set_term(2);
  vote.set_candidate_id(1);

  testing::Matcher<const ReplicaInfo&> replica_matcher =
      testing::Truly([&](const ReplicaInfo& info) {
        return info.id() == target.id();
      });

  EXPECT_CALL(communicator, SendMessage(testing::_, replica_matcher))
      .WillOnce(testing::Invoke(
          [&](const google::protobuf::Message& message,
              const ReplicaInfo&) {
            const Request& req = static_cast<const Request&>(message);
            EXPECT_EQ(req.user_type(),
                      raft::RAFT_REQUEST_VOTE_REQUEST);
            return 0;
          }));
  EXPECT_TRUE(rpc.SendRequestVote(vote, target).ok());
}

}  // namespace
}  // namespace resdb

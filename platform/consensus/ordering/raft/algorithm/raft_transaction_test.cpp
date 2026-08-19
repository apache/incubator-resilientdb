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

#include "platform/consensus/ordering/raft/algorithm/raft_tests.h"

namespace resdb {
namespace raft {

InFlightMsg MakeInFlightMsg(uint64_t prev_log_index_sent,
                            uint64_t last_index_of_segment_sent,
                            std::chrono::steady_clock::time_point timestamp =
                                std::chrono::steady_clock::now()) {
  InFlightMsg in_flight_msg;
  in_flight_msg.time_sent = timestamp;
  in_flight_msg.prev_log_index_sent = prev_log_index_sent;
  in_flight_msg.last_index_of_segment_sent = last_index_of_segment_sent;
  return in_flight_msg;
}

// Test 1: A follower receiving a client transaction should reject it.
TEST_F(RaftTest, FollowerRejectsClientTransaction) {
  EXPECT_CALL(mock_call, Call(MessageType::AppendEntriesResponseMsg, _, _))
      .WillOnce(Invoke(
          [&](int, const google::protobuf::Message& msg, int) { return 0; }));
  EXPECT_CALL(mock_call, Call(MessageType::DirectToLeaderMsg, _, _))
      .WillOnce(Invoke([&](int, const google::protobuf::Message& msg, int) {
        const auto& dtl = dynamic_cast<const DirectToLeader&>(msg);
        EXPECT_EQ(dtl.leader_id(), 2);
        return 0;
      }));
  EXPECT_CALL(*leader_election_manager_, OnHeartbeat()).Times(1);
  EXPECT_CALL(mock_broadcast, Broadcast(_, _)).Times(0);

  raft_->SetStateForTest({
      .current_term = 0,
      .role = Role::FOLLOWER,
      .log = CreateLogEntries({}, true),
  });

  // Set up the client so it can send the DirectToLeader message.
  ReplicaInfo client;
  client.set_ip("127.0.0.1");
  client.set_port(1236);
  client.set_id(3);
  replica_communicator_->UpdateClientReplicas({client});

  // Receive an AppendEntries heartbeat to set current_leader_.
  auto ae_fields = CreateAeFields(
      /*term=*/0,
      /*leader_id=*/2,
      /*prev_log_index=*/0,
      /*prev_log_term=*/0,
      /*entries=*/
      CreateLogEntries({}),
      /*leader_commit=*/0,
      /*follower_id=*/1);
  auto ae_message = CreateAeMessage(ae_fields);

  bool ae_success = raft_->ReceiveAppendEntries(
      std::make_unique<AppendEntries>(std::move(ae_message)));
  EXPECT_TRUE(ae_success);

  auto req = std::make_unique<Request>();
  req->set_seq(1);
  bool success = raft_->ReceiveTransaction(std::move(req));
  EXPECT_FALSE(success);
}

// Test 2: A leader receiving a client transaction should send an AppendEntries
// to all other replicas.
TEST_F(RaftTest, LeaderSendsAppendEntriesUponClientTransaction) {
  EXPECT_CALL(mock_call, Call(_, _, _)).Times(3);
  EXPECT_CALL(*leader_election_manager_, OnAeBroadcast()).Times(1);

  auto req = std::make_unique<Request>();
  req->set_seq(1);
  raft_->SetStateForTest({.current_term = 0,
                          .role = Role::LEADER,
                          .log = CreateLogEntries({}, true),
                          .enable_batching = false});

  bool success = raft_->ReceiveTransaction(std::move(req));
  EXPECT_TRUE(success);
}

// Test 3: Sent AppendEntries should be based on the follower's next_index.
TEST_F(RaftTest, LeaderSendsAppendEntriesBasedOnNextIndex) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 2);
            EXPECT_EQ(ae.prev_log_index(), 2);
            EXPECT_EQ(ae.entries().size(), 3);
            return 0;
          }))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 3);
            EXPECT_EQ(ae.prev_log_index(), 1);
            EXPECT_EQ(ae.entries().size(), 4);
            return 0;
          }))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 4);
            EXPECT_EQ(ae.prev_log_index(), 0);
            EXPECT_EQ(ae.entries().size(), 5);
            return 0;
          }));
  EXPECT_CALL(*leader_election_manager_, OnAeBroadcast()).Times(1);

  raft_->SetStateForTest(
      {.current_term = 0,
       .role = Role::LEADER,
       .log = CreateLogEntries(
           {
               {0, "Term 0 Transaction 1"},
               {0, "Term 0 Transaction 2"},
               {0, "Term 0 Transaction 3"},
               {0, "Term 0 Transaction 4"},
           },
           true),
       CreateProgressPatch({
           .next_index = std::vector<uint64_t>{1, 4, 3, 2, 1},
       }),
       .enable_batching = false});

  auto req = std::make_unique<Request>();
  req->set_seq(5);

  bool success = raft_->ReceiveTransaction(std::move(req));
  EXPECT_TRUE(success);
}

// Test 4: Leader does not send entries to followers at the in-flight limit.
TEST_F(RaftTest, LeaderDoesNotExceedInFlightLimit) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 2);
            EXPECT_EQ(ae.prev_log_index(), 2);
            EXPECT_EQ(ae.entries().size(), 3);
            return 0;
          }))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 3);
            EXPECT_EQ(ae.prev_log_index(), 1);
            EXPECT_EQ(ae.entries().size(), 0);
            return 0;
          }))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 4);
            EXPECT_EQ(ae.prev_log_index(), 0);
            EXPECT_EQ(ae.entries().size(), 5);
            return 0;
          }));
  EXPECT_CALL(*leader_election_manager_, OnAeBroadcast()).Times(1);
  raft_->SetStateForTest(
      {.current_term = 0,
       .role = Role::LEADER,
       .log = CreateLogEntries(
           {
               {0, "Term 0 Transaction 1"},
               {0, "Term 0 Transaction 2"},
               {0, "Term 0 Transaction 3"},
               {0, "Term 0 Transaction 4"},
           },
           true),
       .progress = CreateProgressPatch({
           .next_index = std::vector<uint64_t>{1, 4, 3, 2, 1},
           .in_flight =
               std::vector<std::vector<InFlightMsg>>{
                   {},
                   {},
                   {},
                   {MakeInFlightMsg(1, 2), MakeInFlightMsg(2, 3)},
                   {},
               },
       }),
       .enable_batching = false,
       .max_in_flight_per_follower = 2});
  auto req = std::make_unique<Request>();
  req->set_seq(5);
  bool success = raft_->ReceiveTransaction(std::move(req));
  EXPECT_TRUE(success);
}

// Test 5: Leader Prunes expired in-flight messages.
// When this happens, it sends a probe with the prev_log_index and term of its
// match_index for that follower. Since the probe contains no entries, the
// follower will not truncate its log, and will respond with its last_log_index.
// If the follower's next_index is behind the truncation_last_index, a snapshot
// will be queued.
TEST_F(RaftTest, LeaderPrunesInFlightLimit) {
  EXPECT_CALL(mock_call, Call(_, _, _))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 2);
            EXPECT_EQ(ae.prev_log_index(), 2);
            EXPECT_EQ(ae.entries().size(), 0);
            return 0;
          }))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 3);
            EXPECT_EQ(ae.prev_log_index(), 3);
            EXPECT_EQ(ae.entries().size(), 0);
            return 0;
          }))
      .WillOnce(::testing::Invoke(
          [](int type, const google::protobuf::Message& msg, int node_id) {
            const auto& ae = dynamic_cast<const AppendEntries&>(msg);
            EXPECT_EQ(node_id, 4);
            // Send from Next index to what was received.
            EXPECT_EQ(ae.prev_log_index(), 3);
            EXPECT_EQ(ae.entries().size(), 5);
            return 0;
          }));
  EXPECT_CALL(*leader_election_manager_, OnAeBroadcast()).Times(1);
  raft_->SetStateForTest(
      {.current_term = 0,
       .role = Role::LEADER,
       .snapshot_last_index = 4,
       .snapshot_last_term = 0,
       .truncated_last_index = 3,
       .truncated_last_term = 0,
       .log = CreateLogEntries(
           {
               {0, "Term 0 Transaction 4"},
               {0, "Term 0 Transaction 5"},
               {0, "Term 0 Transaction 6"},
               {0, "Term 0 Transaction 7"},
           },
           true),
       .progress = CreateProgressPatch(
           {.next_index = std::vector<uint64_t>{1, 4, 4, 3, 4},
            .match_index = std::vector<uint64_t>{0, 3, 2, 3, 3},
            .in_flight =
                std::vector<std::vector<InFlightMsg>>{
                    {},
                    {},
                    {MakeInFlightMsg(2, 3,
                                     std::chrono::steady_clock::now() -
                                         raft_->GetAEResponseDeadline()),
                     MakeInFlightMsg(3, 4)},
                    {MakeInFlightMsg(3, 4,
                                     std::chrono::steady_clock::now() -
                                         raft_->GetAEResponseDeadline()),
                     MakeInFlightMsg(4, 5)},
                    {}},
            .states = std::vector<ProgressState>{ProgressState::PROBE,
                                                 ProgressState::REPLICATE,
                                                 ProgressState::REPLICATE,
                                                 ProgressState::REPLICATE,
                                                 ProgressState::REPLICATE}}),
       .enable_batching = false,
       .max_in_flight_per_follower = 2});

  auto req = std::make_unique<Request>();
  req->set_seq(5);
  bool success = raft_->ReceiveTransaction(std::move(req));
  EXPECT_TRUE(success);
  // The first in flight message should be pruned, leaving room for the second
  // one to be sent and added.
  const auto& follower_progress = raft_->GetFollowerProgress();
  EXPECT_EQ(follower_progress[3].state, ProgressState::PROBE);
  EXPECT_EQ(follower_progress[2].state, ProgressState::SNAPSHOT);
}

}  // namespace raft
}  // namespace resdb

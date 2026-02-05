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

#pragma once

#include <gmock/gmock.h>

#include "platform/consensus/ordering/raft/algorithm/raft.h"

namespace resdb {
namespace raft {

// Mock Raft class to test LeaderElectionManager interactions
class MockRaft : public Raft {
 public:
  MockRaft(int id, int f, int total_num, SignatureVerifier* verifier,
           LeaderElectionManager* leaderelection_manager, 
           ReplicaCommunicator* replica_communicator)
      : Raft(id, f, total_num, verifier, leaderelection_manager, replica_communicator){}

  MOCK_METHOD(void, SendHeartBeat, (), ());
  MOCK_METHOD(void, StartElection, (), ());
  MOCK_METHOD(int, Broadcast, (int msg_type, const google::protobuf::Message& msg), (override));
  MOCK_METHOD(int, SendMessage, (int msg_type,
                              const google::protobuf::Message& msg,
                              int node_id), (override));
};

}  // namespace raft
}  // namespace resdb

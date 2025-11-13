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

#include <vector>

#include "absl/status/status.h"
#include "platform/config/resdb_config.h"
#include "platform/consensus/ordering/raft/proto/raft.pb.h"
#include "platform/consensus/ordering/raft/raft_message_type.h"
#include "platform/networkstrate/replica_communicator.h"
#include "platform/proto/resdb.pb.h"

namespace resdb {

class RaftRpc {
 public:
  RaftRpc(const ResDBConfig& config, ReplicaCommunicator* communicator);

  absl::Status BroadcastAppendEntries(const raft::AppendEntriesRequest& request);
  absl::Status SendAppendEntries(const raft::AppendEntriesRequest& request,
                                 const ReplicaInfo& replica);

  absl::Status BroadcastRequestVote(const raft::RequestVoteRequest& request);
  absl::Status SendRequestVote(const raft::RequestVoteRequest& request,
                               const ReplicaInfo& replica);

  absl::Status BroadcastInstallSnapshot(
      const raft::InstallSnapshotRequest& request);
  absl::Status SendInstallSnapshot(
      const raft::InstallSnapshotRequest& request,
      const ReplicaInfo& replica);

 private:
  absl::Status Send(const google::protobuf::Message& payload, int user_type,
                    const ReplicaInfo* replica);

  Request BuildEnvelope(int user_type,
                        const google::protobuf::Message& payload) const;

 private:
  const ResDBConfig& config_;
  ReplicaCommunicator* communicator_;
};

}  // namespace resdb


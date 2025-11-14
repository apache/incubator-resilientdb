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

#include "glog/logging.h"

namespace resdb {

RaftRpc::RaftRpc(const ResDBConfig& config,
                 ReplicaCommunicator* communicator)
    : config_(config), communicator_(communicator) {}

absl::Status RaftRpc::BroadcastAppendEntries(
    const raft::AppendEntriesRequest& request) {
  return Send(request, raft::RAFT_APPEND_ENTRIES_REQUEST, nullptr);
}

absl::Status RaftRpc::SendAppendEntries(
    const raft::AppendEntriesRequest& request, const ReplicaInfo& replica) {
  return Send(request, raft::RAFT_APPEND_ENTRIES_REQUEST, &replica);
}

absl::Status RaftRpc::SendAppendEntriesResponse(
    const raft::AppendEntriesResponse& response, const ReplicaInfo& replica) {
  return Send(response, raft::RAFT_APPEND_ENTRIES_RESPONSE, &replica);
}

absl::Status RaftRpc::BroadcastRequestVote(
    const raft::RequestVoteRequest& request) {
  return Send(request, raft::RAFT_REQUEST_VOTE_REQUEST, nullptr);
}

absl::Status RaftRpc::SendRequestVote(
    const raft::RequestVoteRequest& request, const ReplicaInfo& replica) {
  return Send(request, raft::RAFT_REQUEST_VOTE_REQUEST, &replica);
}

absl::Status RaftRpc::SendRequestVoteResponse(
    const raft::RequestVoteResponse& response, const ReplicaInfo& replica) {
  return Send(response, raft::RAFT_REQUEST_VOTE_RESPONSE, &replica);
}

absl::Status RaftRpc::BroadcastInstallSnapshot(
    const raft::InstallSnapshotRequest& request) {
  return Send(request, raft::RAFT_INSTALL_SNAPSHOT_REQUEST, nullptr);
}

absl::Status RaftRpc::SendInstallSnapshot(
    const raft::InstallSnapshotRequest& request, const ReplicaInfo& replica) {
  return Send(request, raft::RAFT_INSTALL_SNAPSHOT_REQUEST, &replica);
}

absl::Status RaftRpc::SendInstallSnapshotResponse(
    const raft::InstallSnapshotResponse& response,
    const ReplicaInfo& replica) {
  return Send(response, raft::RAFT_INSTALL_SNAPSHOT_RESPONSE, &replica);
}

absl::Status RaftRpc::Send(const google::protobuf::Message& payload,
                           int user_type, const ReplicaInfo* replica) {
  if (communicator_ == nullptr) {
    return absl::FailedPreconditionError("communicator is null");
  }
  Request envelope = BuildEnvelope(user_type, payload);
  int rc = 0;
  if (replica == nullptr) {
    communicator_->BroadCast(envelope);
  } else {
    rc = communicator_->SendMessage(envelope, *replica);
  }
  if (rc != 0) {
    return absl::InternalError("failed to send RAFT RPC");
  }
  return absl::OkStatus();
}

Request RaftRpc::BuildEnvelope(int user_type,
                               const google::protobuf::Message& payload) const {
  Request request;
  request.set_type(Request::TYPE_CUSTOM_CONSENSUS);
  request.set_user_type(user_type);
  request.set_sender_id(config_.GetSelfInfo().id());
  request.mutable_region_info()->set_region_id(
      config_.GetConfigData().self_region_id());
  payload.SerializeToString(request.mutable_data());
  return request;
}

}  // namespace resdb

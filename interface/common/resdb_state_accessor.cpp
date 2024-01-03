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

#include <future>
#include <thread>

namespace resdb {

ResDBStateAccessor::ResDBStateAccessor(const ResDBConfig& config)
    : config_(config) {}

std::unique_ptr<NetChannel> ResDBStateAccessor::GetNetChannel(
    const std::string& ip, int port) {
  return std::make_unique<NetChannel>(ip, port);
}

// Obtain ReplicaState of each replica.
absl::StatusOr<std::vector<ReplicaState>>
ResDBStateAccessor::GetReplicaStates() {
  const auto& client_info = config_.GetReplicaInfos()[0];

  Request request;
  std::unique_ptr<NetChannel> client =
      GetNetChannel(client_info.ip(), client_info.port());
  client->SetRecvTimeout(1000000);  // 1s for recv timeout.
	
  LOG(ERROR)<<"send request";
  int ret = client->SendRequest(request, Request::TYPE_REPLICA_STATE);
  if (ret) {
    LOG(ERROR)<<"recv request fail:"<<ret;
    return absl::InternalError("send data fail.");
  }

  std::unique_ptr<ReplicaState> state = std::make_unique<ReplicaState>();
  LOG(ERROR)<<"recv request";
  ret = client->RecvRawMessage(state.get());
  if (ret < 0) {
    return absl::InternalError("recv data fail.");
  }
  if (state == nullptr) {
    return absl::InternalError("recv data fail.");
  }
  LOG(ERROR)<<"recv state:"<<state->DebugString();

  std::vector<ReplicaState> resp;
  for (const auto& region : state->replica_config().region()) {
    for (const auto& info : region.replica_info()) {
      ReplicaState new_state;
      LOG(ERROR)<<"push new state:"<<new_state.DebugString();
      *new_state.mutable_replica_info() = info;
      resp.push_back(new_state);
    }
  }
  return resp;
}

}  // namespace resdb

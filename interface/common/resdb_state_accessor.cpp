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

  int ret = client->SendRequest(request, Request::TYPE_REPLICA_STATE);
  if (ret) {
    return absl::InternalError("send data fail.");
  }

  std::unique_ptr<ReplicaState> state = std::make_unique<ReplicaState>();
  ret = client->RecvRawMessage(state.get());
  if (ret < 0) {
    return absl::InternalError("recv data fail.");
  }
  if (state == nullptr) {
    return absl::InternalError("recv data fail.");
  }

  std::vector<ReplicaState> resp;
  for (const auto& region : state->replica_config().region()) {
    for (const auto& info : region.replica_info()) {
      ReplicaState new_state;
      *new_state.mutable_replica_info() = info;
      resp.push_back(new_state);
    }
  }
  return resp;
}

}  // namespace resdb

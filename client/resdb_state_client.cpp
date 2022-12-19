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

#include "client/resdb_state_client.h"

#include <glog/logging.h>

#include <future>
#include <thread>

namespace resdb {

ResDBStateClient::ResDBStateClient(const ResDBConfig& config)
    : config_(config) {}

std::unique_ptr<ResDBClient> ResDBStateClient::GetResDBClient(
    const std::string& ip, int port) {
  return std::make_unique<ResDBClient>(ip, port);
}

// Obtain ReplicaState of each replica.
absl::StatusOr<std::vector<ReplicaState>> ResDBStateClient::GetReplicaStates() {
  std::vector<std::thread> ths;
  Request request;
  std::vector<std::future<std::unique_ptr<ReplicaState>>> resp_future;
  std::vector<ReplicaState> resp;

  for (const auto& replica : config_.GetReplicaInfos()) {
    std::promise<std::unique_ptr<ReplicaState>> state_prom;
    resp_future.push_back(state_prom.get_future());
    ths.push_back(std::thread(
        [&](std::promise<std::unique_ptr<ReplicaState>> state_prom) {
          std::unique_ptr<ResDBClient> client =
              GetResDBClient(replica.ip(), replica.port());
          client->SetRecvTimeout(1000000);  // 1s for recv timeout.

          int ret = client->SendRequest(request, Request::TYPE_REPLICA_STATE);
          if (ret) {
            state_prom.set_value(nullptr);
            return;
          }

          std::unique_ptr<ReplicaState> state =
              std::make_unique<ReplicaState>();
          ret = client->RecvRawMessage(state.get());
          if (ret < 0) {
            state_prom.set_value(nullptr);
            return;
          }
          state_prom.set_value(std::move(state));
          return;
        },
        std::move(state_prom)));
  }

  for (auto& future : resp_future) {
    auto state = future.get();
    if (state == nullptr) {
      continue;
    }
    resp.push_back(*state);
  }

  for (auto& th : ths) {
    th.join();
  }
  return resp;
}

}  // namespace resdb

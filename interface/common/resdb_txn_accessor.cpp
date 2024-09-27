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

#include "interface/common/resdb_txn_accessor.h"

#include <glog/logging.h>

#include <future>
#include <thread>

namespace resdb {

ResDBTxnAccessor::ResDBTxnAccessor(const ResDBConfig& config)
    : config_(config),
      replicas_(config.GetReplicaInfos()),
      recv_timeout_(1) /*1s*/ {}

std::unique_ptr<NetChannel> ResDBTxnAccessor::GetNetChannel(
    const std::string& ip, int port) {
  return std::make_unique<NetChannel>(ip, port);
}

// Obtain ReplicaState of each replica.
absl::StatusOr<std::vector<std::pair<uint64_t, std::string>>>
ResDBTxnAccessor::GetTxn(uint64_t min_seq, uint64_t max_seq) {
  QueryRequest request;
  request.set_min_seq(min_seq);
  request.set_max_seq(max_seq);

  std::vector<std::unique_ptr<NetChannel>> clients;
  std::vector<std::thread> ths;
  std::string final_str;
  std::mutex mtx;
  std::condition_variable resp_cv;
  bool success = false;
  std::map<std::string, int> recv_count;
  for (const auto& replica : replicas_) {
    std::unique_ptr<NetChannel> client =
        GetNetChannel(replica.ip(), replica.port());
    NetChannel* client_ptr = client.get();
    clients.push_back(std::move(client));

    ths.push_back(std::thread(
        [&](NetChannel* client) {
          std::string response_str;
          int ret = client->SendRequest(request, Request::TYPE_QUERY);
          if (ret) {
            return;
          }
          client->SetRecvTimeout(1000);
          ret = client->RecvRawMessageStr(&response_str);
          if (ret == 0) {
            std::unique_lock<std::mutex> lck(mtx);
            recv_count[response_str]++;
            // receive f+1 count.
            if (recv_count[response_str] == config_.GetMinClientReceiveNum()) {
              final_str = response_str;
              success = true;
              // notify the main thread.
              resp_cv.notify_all();
            }
          }
          return;
        },
        client_ptr));
  }

  {
    std::unique_lock<std::mutex> lck(mtx);
    resp_cv.wait_for(lck, std::chrono::seconds(recv_timeout_));
    // Time out or done, close all the client.
    for (auto& client : clients) {
      client->Close();
    }
  }

  // wait for all theads done.
  for (auto& th : ths) {
    if (th.joinable()) {
      th.join();
    }
  }

  std::vector<std::pair<uint64_t, std::string>> txn_resp;
  QueryResponse resp;
  if (success && final_str.empty()) {
    return txn_resp;
  }

  if (final_str.empty() || !resp.ParseFromString(final_str)) {
    LOG(ERROR) << "parse fail len:" << final_str.size();
    return absl::InternalError("recv data fail.");
  }
  for (auto& transaction : resp.transactions()) {
    txn_resp.push_back(std::make_pair(transaction.seq(), transaction.data()));
  }
  return txn_resp;
}

absl::StatusOr<std::vector<Request>> ResDBTxnAccessor::GetRequestFromReplica(
    uint64_t min_seq, uint64_t max_seq, const ReplicaInfo& replica) {
  QueryRequest request;
  request.set_min_seq(min_seq);
  request.set_max_seq(max_seq);

  std::unique_ptr<NetChannel> client =
      GetNetChannel(replica.ip(), replica.port());

  std::string response_str;
  int ret = client->SendRequest(request, Request::TYPE_QUERY);
  if (ret) {
    return absl::InternalError("send data fail.");
  }
  client->SetRecvTimeout(1000);
  ret = client->RecvRawMessageStr(&response_str);
  if (ret) {
    return absl::InternalError("recv data fail.");
  }

  QueryResponse resp;

  if (!resp.ParseFromString(response_str)) {
    LOG(ERROR) << "parse fail len:" << response_str.size();
    return absl::InternalError("recv data fail.");
  }

  std::vector<Request> txn_resp;
  for (auto& transaction : resp.transactions()) {
    txn_resp.push_back(transaction);
  }
  return txn_resp;
}

absl::StatusOr<uint64_t> ResDBTxnAccessor::GetBlockNumbers() {
  QueryRequest request;
  request.set_min_seq(0);
  request.set_max_seq(0);

  std::vector<std::unique_ptr<NetChannel>> clients;
  std::vector<std::thread> ths;
  std::string final_str;
  std::mutex mtx;
  std::condition_variable resp_cv;
  bool success = false;

  std::unique_ptr<NetChannel> client =
      GetNetChannel(replicas_[0].ip(), replicas_[0].port());

  LOG(INFO) << "ip:" << replicas_[0].ip() << " port:" << replicas_[0].port();

  std::string response_str;
  int ret = 0;
  for (int i = 0; i < 5; ++i) {
    ret = client->SendRequest(request, Request::TYPE_QUERY);
    if (ret) {
      continue;
    }
    client->SetRecvTimeout(100000);
    ret = client->RecvRawMessageStr(&response_str);
    LOG(INFO) << "receive str:" << ret << " len:" << response_str.size();
    if (ret != 0) {
      continue;
    }
    break;
  }

  QueryResponse resp;
  if (response_str.empty() || !resp.ParseFromString(response_str)) {
    LOG(ERROR) << "parse fail len:" << final_str.size();
    return absl::InternalError("recv data fail.");
  }
  return resp.max_seq();
}

}  // namespace resdb

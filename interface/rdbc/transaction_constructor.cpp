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

#include "interface/rdbc/transaction_constructor.h"

#include <glog/logging.h>

namespace resdb {

TransactionConstructor::TransactionConstructor(const ResDBConfig& config)
    : NetChannel("", 0),
      config_(config),
      timeout_ms_(
          config.GetClientTimeoutMs()) {  // default 2s for process timeout
  socket_->SetRecvTimeout(timeout_ms_);
}

absl::StatusOr<std::string> TransactionConstructor::GetResponseData(
    const Response& response) {
  std::string hash_;
  std::set<int64_t> hash_counter;
  std::string resp_str;
  for (const auto& each_resp : response.resp()) {
    // Check signature
    std::string hash = SignatureVerifier::CalculateHash(each_resp.data());

    if (!hash_.empty() && hash != hash_) {
      LOG(ERROR) << "hash not the same";
      return absl::InvalidArgumentError("hash not match");
    }
    if (hash_.empty()) {
      hash_ = hash;
      resp_str = each_resp.data();
    }
    hash_counter.insert(each_resp.signature().node_id());
  }
  // LOG(INFO) << "recv hash:" << hash_counter.size()
  //         << " need:" << config_.GetMinClientReceiveNum()
  //        << " data len:" << resp_str.size();
  if (hash_counter.size() >=
      static_cast<size_t>(config_.GetMinClientReceiveNum())) {
    return resp_str;
  }
  return absl::InvalidArgumentError("data not enough");
}

int TransactionConstructor::SendRequest(
    const google::protobuf::Message& message, Request::Type type) {
  // Use the replica obtained from the server.
  NetChannel::SetDestReplicaInfo(config_.GetReplicaInfos()[0]);
  return NetChannel::SendRequest(message, type, false);
}

int TransactionConstructor::SendRequest(
    const google::protobuf::Message& message,
    google::protobuf::Message* response, Request::Type type) {
  NetChannel::SetDestReplicaInfo(config_.GetReplicaInfos()[0]);
  int ret = NetChannel::SendRequest(message, type, true);
  if (ret == 0) {
    std::string resp_str;
    int ret = NetChannel::RecvRawMessageData(&resp_str);
    if (ret >= 0) {
      if (!response->ParseFromString(resp_str)) {
        LOG(ERROR) << "parse response fail:" << resp_str.size();
        return -2;
      }
      return 0;
    }
  }
  return -1;
}

}  // namespace resdb

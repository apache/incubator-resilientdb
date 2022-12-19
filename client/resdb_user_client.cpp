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

#include "client/resdb_user_client.h"

#include <glog/logging.h>

namespace resdb {

ResDBUserClient::ResDBUserClient(const ResDBConfig& config)
    : ResDBClient("", 0),
      config_(config),
      timeout_ms_(
          config.GetClientTimeoutMs()),  // default 2s for process timeout
      is_check_signature_(true) {
  socket_->SetRecvTimeout(timeout_ms_);
}

void ResDBUserClient::DisableSignatureCheck() { is_check_signature_ = false; }

absl::StatusOr<std::string> ResDBUserClient::GetResponseData(
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

int ResDBUserClient::SendRequest(const google::protobuf::Message& message,
                                 Request::Type type) {
  // Use the replica obtained from the server.
  ResDBClient::SetDestReplicaInfo(config_.GetReplicaInfos()[0]);
  return ResDBClient::SendRequest(message, type, false);
}

int ResDBUserClient::SendRequest(const google::protobuf::Message& message,
                                 google::protobuf::Message* response,
                                 Request::Type type) {
  ResDBClient::SetDestReplicaInfo(config_.GetReplicaInfos()[0]);
  int ret = ResDBClient::SendRequest(message, type, true);
  if (ret == 0) {
    std::string resp_str;
    int ret = ResDBClient::RecvRawMessageData(&resp_str);
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

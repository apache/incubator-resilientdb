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

#include "platform/interface/resdb_transaction_proposer.h"

#include <glog/logging.h>

namespace resdb {

ResDBTransactionProposer::ResDBTransactionProposer(const ResDBConfig& config)
    : ResDBNetChannel("", 0),
      config_(config),
      timeout_ms_(
          config.GetClientTimeoutMs()) {  // default 2s for process timeout
  socket_->SetRecvTimeout(timeout_ms_);
}

int ResDBTransactionProposer::SendRequest(
    const google::protobuf::Message& message, Request::Type type) {
  // Use the replica obtained from the server.
  ResDBNetChannel::SetDestReplicaInfo(config_.GetReplicaInfos()[0]);
  return ResDBNetChannel::SendRequest(message, type, false);
}

int ResDBTransactionProposer::SendRequest(
    const google::protobuf::Message& message,
    google::protobuf::Message* response, Request::Type type) {
  ResDBNetChannel::SetDestReplicaInfo(config_.GetReplicaInfos()[0]);
  int ret = ResDBNetChannel::SendRequest(message, type, true);
  if (ret == 0) {
    std::string resp_str;
    int ret = ResDBNetChannel::RecvRawMessageData(&resp_str);
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

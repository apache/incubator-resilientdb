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

#pragma once

#include "platform/config/resdb_config.h"
#include "platform/networkstrate/replica_communicator.h"
#include "platform/networkstrate/server_comm.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace common {

class RequestHandler {
 public:
  RequestHandler() {}

  virtual std::unique_ptr<Request> ConvertToUserRequest(
      const BatchUserRequest& batch_request) = 0;
};

template <class RequestType>
class RequestSerializer : public RequestHandler {
 public:
  RequestSerializer(SignatureVerifier* verifier, int node_id,
                    int new_request_type)
      : verifier_(verifier),
        node_id_(node_id),
        new_request_type_(new_request_type) {}

  std::unique_ptr<Request> ConvertToUserRequest(
      const BatchUserRequest& batch_request) override {
    std::unique_ptr<RequestType> request = std::make_unique<RequestType>();
    batch_request.SerializeToString(request->mutable_data());
    request->set_proxy_id(node_id_);
    request->set_hash(SignatureVerifier::CalculateHash(request->data()));
    if (verifier_) {
      auto signature_or = verifier_->SignMessage(request->data());
      if (!signature_or.ok()) {
        LOG(ERROR) << "Sign message fail";
        return nullptr;
      }
      *request->mutable_data_signature() = *signature_or;
    }
    return NewRequest(std::move(request));
  }

  std::unique_ptr<Request> NewRequest(
      std::unique_ptr<RequestType> user_request) {
    user_request->set_type(new_request_type_);
    user_request->set_sender_id(node_id_);
    auto ret = std::make_unique<Request>();
    user_request->SerializeToString(ret->mutable_data());
    return ret;
  }

 private:
  SignatureVerifier* verifier_;
  int node_id_;
  int new_request_type_;
};

}  // namespace common
}  // namespace resdb

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

#include "absl/status/statusor.h"
#include "client/resdb_client.h"
#include "config/resdb_config.h"

namespace resdb {

// ResDBUserClient to send data to one server located in the config replicas
// and receive data from it.
// Inside ResDBUserClient, it does two things:
// 1. Broadcast the public key obtained from the config
//    and receive the public keys of each replicas and the up-to-date replica
//    list.
// 2. Send the messages to one server with the certificate signed by the
//    private key and receive the response if needed. Each response contains
//    at least f+1 valid sub response from distinct replicas (can be verified
//    by the public keys obtained from 1).
class ResDBUserClient : public ResDBClient {
 public:
  ResDBUserClient(const ResDBConfig& config);
  virtual ~ResDBUserClient() = default;

  // Send request with a command.
  int SendRequest(const google::protobuf::Message& message,
                  Request::Type type = Request::TYPE_CLIENT_REQUEST);
  // Send request with a command and wait for a response.
  int SendRequest(const google::protobuf::Message& message,
                  google::protobuf::Message* response,
                  Request::Type type = Request::TYPE_CLIENT_REQUEST);

  // Only For test.
  void DisableSignatureCheck();

 private:
  absl::StatusOr<std::string> GetResponseData(const Response& response);

 private:
  ResDBConfig config_;
  int64_t timeout_ms_;  // microsecond for timeout.
  bool is_check_signature_;
};

}  // namespace resdb

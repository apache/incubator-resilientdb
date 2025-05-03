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

#pragma once

#include "absl/status/statusor.h"
#include "interface/rdbc/net_channel.h"
#include "platform/config/resdb_config.h"

namespace resdb {

// TransactionConstructor is a tool to access NesDB to send data and receive
// data Inside TransactionConstructor, it does two things:
class TransactionConstructor : public NetChannel {
 public:
  TransactionConstructor(const ResDBConfig& config);
  virtual ~TransactionConstructor() = default;

  // Send request with a command.
  int SendRequest(const google::protobuf::Message& message,
                  Request::Type type = Request::TYPE_CLIENT_REQUEST);
  // Send request with a command and wait for a response.
  int SendRequest(const google::protobuf::Message& message,
                  google::protobuf::Message* response,
                  Request::Type type = Request::TYPE_CLIENT_REQUEST);

 private:
  absl::StatusOr<std::string> GetResponseData(const Response& response);

 private:
  ResDBConfig config_;
  int64_t timeout_ms_;  // microsecond for timeout.
};

}  // namespace resdb

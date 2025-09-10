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

#include "gmock/gmock.h"
#include "interface/rdbc/net_channel.h"

namespace resdb {

// A mock class for NetChannel.
class MockNetChannel : public NetChannel {
 public:
  MockNetChannel(const std::string& ip, int port) : NetChannel(ip, port) {}

  MOCK_METHOD(int, SendRequest,
              (const google::protobuf::Message&, Request::Type, bool),
              (override));
  MOCK_METHOD(int, SendRawMessage, (const google::protobuf::Message&),
              (override));
  MOCK_METHOD(int, SendRawMessageData, (const std::string&), (override));
  MOCK_METHOD(int, RecvRawMessageStr, (std::string*), (override));
  MOCK_METHOD(int, RecvRawMessage, (google::protobuf::Message*), (override));
};

}  // namespace resdb

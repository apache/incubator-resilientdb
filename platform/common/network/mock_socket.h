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

#include <string>

#include "gmock/gmock.h"
#include "platform/common/network/socket.h"

namespace resdb {

class MockSocket : public Socket {
 public:
  MOCK_METHOD(int, Connect, (const std::string&, int), (override));
  MOCK_METHOD(int, Listen, (const std::string&, int), (override));
  MOCK_METHOD(void, ReInit, (), (override));
  MOCK_METHOD(void, Close, (), (override));
  MOCK_METHOD(std::unique_ptr<Socket>, Accept, (), (override));
  MOCK_METHOD(int, Send, (const std::string&), (override));
  MOCK_METHOD(int, Recv, (void**, size_t*), (override));
  MOCK_METHOD(int, GetBindingPort, (), (override));

  MOCK_METHOD(int, SetAsync, (bool), (override));
};

}  // namespace resdb

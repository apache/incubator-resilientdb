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

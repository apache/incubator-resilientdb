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

#include <string>

#include "common/network/socket.h"
#include "gmock/gmock.h"

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

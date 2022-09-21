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

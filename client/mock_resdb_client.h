#pragma once

#include "client/resdb_client.h"
#include "gmock/gmock.h"

namespace resdb {

// A mock class for ResDBClient.
class MockResDBClient : public ResDBClient {
 public:
  MockResDBClient(const std::string& ip, int port) : ResDBClient(ip, port) {}

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

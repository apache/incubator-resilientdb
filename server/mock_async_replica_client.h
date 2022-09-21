#pragma once

#include <gmock/gmock.h>

#include "server/async_replica_client.h"

namespace resdb {

class MockAsyncReplicaClient : public AsyncReplicaClient {
 public:
  MockAsyncReplicaClient(boost::asio::io_service *io_service)
      : AsyncReplicaClient(io_service, "127.0.0.1", 0) {}
  MOCK_METHOD(int, SendMessage, (const std::string &), (override));
};

}  // namespace resdb

#pragma once

#include "client/resdb_txn_client.h"
#include "gmock/gmock.h"

namespace resdb {

// A mock class for ResDBClient.
class MockResDBTxnClient : public ResDBTxnClient {
 public:
  MockResDBTxnClient(const ResDBConfig& config) : ResDBTxnClient(config) {}

  MOCK_METHOD((absl::StatusOr<std::vector<std::pair<uint64_t, std::string>>>),
              GetTxn, (uint64_t, uint64_t), (override));
};

}  // namespace resdb

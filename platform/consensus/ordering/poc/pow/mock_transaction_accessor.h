#pragma once

#include "gmock/gmock.h"
#include "platform/consensus/ordering/poc/pow/transaction_accessor.h"

namespace resdb {

class MockTransactionAccessor : public TransactionAccessor {
 public:
  MockTransactionAccessor(const ResDBPoCConfig& config)
      : TransactionAccessor(config, false) {}
  MOCK_METHOD(std::unique_ptr<BatchClientTransactions>, ConsumeTransactions,
              (uint64_t seq), (override));
};

}  // namespace resdb

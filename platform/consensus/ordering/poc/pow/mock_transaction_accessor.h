#pragma once

#include "platform/consensus/ordering/poc/pow/transaction_accessor.h"
#include "gmock/gmock.h"

namespace resdb {

class MockTransactionAccessor : public TransactionAccessor {
 public:
	 MockTransactionAccessor(const ResDBPoCConfig& config):TransactionAccessor(config, false){}
  MOCK_METHOD(std::unique_ptr<BatchClientTransactions>, ConsumeTransactions, (uint64_t seq),
              (override));
};

}  // namespace resdb

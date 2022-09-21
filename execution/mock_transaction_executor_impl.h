#pragma once

#include "execution/transaction_executor.h"
#include "gmock/gmock.h"

namespace resdb {

class MockTransactionExecutorDataImpl : public TransactionExecutorImpl {
 public:
  MOCK_METHOD(std::unique_ptr<std::string>, ExecuteData, (const std::string&),
              (override));
};

class MockTransactionExecutorImpl : public MockTransactionExecutorDataImpl {
 public:
  MOCK_METHOD(std::unique_ptr<BatchClientResponse>, ExecuteBatch,
              (const BatchClientRequest&), (override));
};

}  // namespace resdb

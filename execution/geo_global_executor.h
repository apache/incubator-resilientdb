#pragma once
#include "execution/transaction_executor_impl.h"

namespace resdb {

class GeoGlobalExecutor {
 public:
  GeoGlobalExecutor(std::unique_ptr<TransactionExecutorImpl> geo_executor_impl);
  virtual ~GeoGlobalExecutor() = default;
  virtual int Execute(std::unique_ptr<Request> request);

 private:
  std::unique_ptr<TransactionExecutorImpl> geo_executor_impl_ = nullptr;
};
}  // namespace resdb

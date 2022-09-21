#pragma once

#include <memory>

#include "proto/resdb.pb.h"

namespace resdb {

class TransactionExecutorImpl {
 public:
  TransactionExecutorImpl() = default;
  virtual ~TransactionExecutorImpl() = default;

  virtual std::unique_ptr<BatchClientResponse> ExecuteBatch(
      const BatchClientRequest& request);

  virtual std::unique_ptr<std::string> ExecuteData(const std::string& request);
};
}  // namespace resdb

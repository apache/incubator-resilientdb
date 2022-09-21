#pragma once

#include "geo_global_executor.h"
#include "gmock/gmock.h"

namespace resdb {

class MockGeoGlobalExecutor : public GeoGlobalExecutor {
 public:
  MockGeoGlobalExecutor(
      std::unique_ptr<TransactionExecutorImpl> geo_executor_impl)
      : GeoGlobalExecutor(std::move(geo_executor_impl)){};
  MOCK_METHOD(int, Execute, (std::unique_ptr<Request>), (override));
};

}  // namespace resdb
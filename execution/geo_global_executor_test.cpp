#include "geo_global_executor.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <future>

#include "mock_transaction_executor_impl.h"

namespace resdb {
namespace {
TEST(GlobalExecutorTest, ExecuteSuccess) {
  auto mock_executor = std::make_unique<MockTransactionExecutorDataImpl>();

  Request request;
  request.set_seq(1);
  BatchClientRequest batch_request;
  batch_request.add_client_requests()->mutable_request()->set_data(
      "global_execute_test");
  batch_request.SerializeToString(request.mutable_data());

  EXPECT_CALL(*mock_executor, ExecuteData("global_execute_test")).Times(1);

  GeoGlobalExecutor global_executor(std::move(mock_executor));
  EXPECT_EQ(global_executor.Execute(std::make_unique<Request>(request)), 0);
}

TEST(GlobalExecutorTest, ExecuteSkiped) {
  auto mock_executor = std::make_unique<MockTransactionExecutorDataImpl>();

  Request request;
  request.set_seq(1);

  EXPECT_CALL(*mock_executor, ExecuteData).Times(0);

  GeoGlobalExecutor global_executor(std::move(mock_executor));
  EXPECT_EQ(global_executor.Execute(std::make_unique<Request>(request)), 1);
}

TEST(GlobalExecutorTest, ExecuteFailed) {
  auto mock_executor = std::make_unique<MockTransactionExecutorDataImpl>();

  Request request;
  request.set_seq(1);
  request.mutable_data()->append("append wrong data");
  EXPECT_CALL(*mock_executor, ExecuteData).Times(0);

  GeoGlobalExecutor global_executor(std::move(mock_executor));
  EXPECT_EQ(global_executor.Execute(std::make_unique<Request>(request)), -1);
}
}  // namespace

}  // namespace resdb

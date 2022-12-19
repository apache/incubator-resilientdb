/*
 * Copyright (c) 2019-2022 ExpoLab, UC Davis
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "geo_global_executor.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <future>

#include "common/test/test_macros.h"
#include "config/resdb_config_utils.h"
#include "mock_transaction_executor_impl.h"

namespace resdb {
namespace {

using ::resdb::testing::EqualsProto;
using ::testing::Test;

class GlobalExecutorTest : public Test {
 public:
  GlobalExecutorTest()
      :  // just set the monitor time to 1 second to return early.
        config_({GenerateReplicaInfo(1, "127.0.0.1", 1234),
                 GenerateReplicaInfo(2, "127.0.0.1", 1235),
                 GenerateReplicaInfo(3, "127.0.0.1", 1236),
                 GenerateReplicaInfo(4, "127.0.0.1", 1237)},
                GenerateReplicaInfo(1, "127.0.0.1", 1234)) {}

 protected:
  ResDBConfig config_;
};

TEST_F(GlobalExecutorTest, ExecuteSuccess) {
  auto mock_executor = std::make_unique<MockTransactionExecutorImpl>();

  Request request;
  request.set_seq(1);
  BatchClientRequest batch_request;
  batch_request.add_client_requests()->mutable_request()->set_data(
      "global_execute_test");
  batch_request.SerializeToString(request.mutable_data());

  EXPECT_CALL(*mock_executor, ExecuteBatch(EqualsProto(batch_request)))
      .Times(1);

  GeoGlobalExecutor global_executor(std::move(mock_executor), config_);
  global_executor.Execute(std::make_unique<Request>(request));
}

TEST_F(GlobalExecutorTest, ExecuteSkiped) {
  auto mock_executor = std::make_unique<MockTransactionExecutorImpl>();

  Request request;
  request.set_seq(1);

  EXPECT_CALL(*mock_executor, ExecuteBatch).Times(0);

  GeoGlobalExecutor global_executor(std::move(mock_executor), config_);
  global_executor.Execute(std::make_unique<Request>(request));
}

TEST_F(GlobalExecutorTest, ExecuteFailed) {
  auto mock_executor = std::make_unique<MockTransactionExecutorImpl>();

  Request request;
  request.set_seq(1);
  request.mutable_data()->append("append wrong data");
  EXPECT_CALL(*mock_executor, ExecuteBatch).Times(0);

  GeoGlobalExecutor global_executor(std::move(mock_executor), config_);
  global_executor.Execute(std::make_unique<Request>(request));
}

}  // namespace

}  // namespace resdb

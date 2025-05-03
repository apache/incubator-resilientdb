/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "platform/consensus/execution/geo_global_executor.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <future>

#include "common/test/test_macros.h"
#include "executor/common/mock_transaction_manager.h"
#include "platform/config/resdb_config_utils.h"

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
  auto mock_executor = std::make_unique<MockTransactionManager>();

  Request request;
  request.set_seq(1);
  BatchUserRequest batch_request;
  batch_request.add_user_requests()->mutable_request()->set_data(
      "global_execute_test");
  batch_request.SerializeToString(request.mutable_data());

  EXPECT_CALL(*mock_executor, ExecuteBatch(EqualsProto(batch_request)))
      .Times(1);

  GeoGlobalExecutor global_executor(std::move(mock_executor), config_);
  global_executor.Execute(std::make_unique<Request>(request));
}

TEST_F(GlobalExecutorTest, ExecuteSkiped) {
  auto mock_executor = std::make_unique<MockTransactionManager>();

  Request request;
  request.set_seq(1);

  EXPECT_CALL(*mock_executor, ExecuteBatch).Times(0);

  GeoGlobalExecutor global_executor(std::move(mock_executor), config_);
  global_executor.Execute(std::make_unique<Request>(request));
}

TEST_F(GlobalExecutorTest, ExecuteFailed) {
  auto mock_executor = std::make_unique<MockTransactionManager>();

  Request request;
  request.set_seq(1);
  request.mutable_data()->append("append wrong data");
  EXPECT_CALL(*mock_executor, ExecuteBatch).Times(0);

  GeoGlobalExecutor global_executor(std::move(mock_executor), config_);
  global_executor.Execute(std::make_unique<Request>(request));
}

}  // namespace

}  // namespace resdb

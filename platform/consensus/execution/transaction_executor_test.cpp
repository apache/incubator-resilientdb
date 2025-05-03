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

#include "platform/consensus/execution/transaction_executor.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <future>

#include "common/test/test_macros.h"
#include "executor/common/mock_transaction_manager.h"

namespace resdb {
namespace {

using ::resdb::testing::EqualsProto;
using ::testing::Invoke;

ResDBConfig GetResDBConfig() {
  return ResDBConfig({ReplicaInfo()}, ReplicaInfo());
}

TEST(TransactionExecutorTest, ExecuteOne) {
  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();

  ResDBConfig config = GetResDBConfig();
  Request request;
  request.set_seq(1);

  BatchUserRequest batch_request;
  batch_request.add_user_requests()->mutable_request()->set_data("execute_1");
  batch_request.SerializeToString(request.mutable_data());

  SystemInfo system_info(config);
  auto mock_executor = std::make_unique<MockTransactionExecutorDataImpl>();

  EXPECT_CALL(*mock_executor, ExecuteData)
      .WillOnce(Invoke([&](const std::string& input) {
        EXPECT_EQ(input, "execute_1");
        done.set_value(true);
        return nullptr;
      }));
  TransactionExecutor executor(
      config,
      [&](std::unique_ptr<Request>, std::unique_ptr<BatchUserResponse> resp) {},
      &system_info, std::move(mock_executor));

  EXPECT_EQ(executor.Commit(std::make_unique<Request>(request)), 0);

  done_future.get();
}

TEST(TransactionExecutorTest, MaxPendingExecuteSeq) {
  Stats::GetGlobalStats()->Stop();
  ResDBConfig config = GetResDBConfig();

  Request request;
  request.set_seq(1);

  BatchUserRequest batch_request;
  batch_request.add_user_requests()->mutable_request()->set_data("execute_1");
  batch_request.SerializeToString(request.mutable_data());

  std::mutex mutex;
  std::condition_variable cv;
  std::atomic<int> call_num = 0;
  SystemInfo system_info(config);
  TransactionExecutor executor(
      config,
      [&](std::unique_ptr<Request>, std::unique_ptr<BatchUserResponse> resp) {},
      &system_info, std::make_unique<MockTransactionManager>());

  executor.SetSeqUpdateNotifyFunc([&](uint64_t seq) {
    std::unique_lock<std::mutex> lk(mutex);
    cv.notify_one();
    call_num++;
  });

  EXPECT_EQ(executor.Commit(std::make_unique<Request>(request)), 0);
  {
    std::unique_lock<std::mutex> lk(mutex);
    cv.wait(lk, [&]() { return call_num >= 1; });
  }
  EXPECT_LE(executor.GetMaxPendingExecutedSeq(), 1);

  request.set_seq(3);
  EXPECT_EQ(executor.Commit(std::make_unique<Request>(request)), 0);
  EXPECT_EQ(executor.GetMaxPendingExecutedSeq(), 1);

  request.set_seq(2);
  EXPECT_EQ(executor.Commit(std::make_unique<Request>(request)), 0);
  {
    std::unique_lock<std::mutex> lk(mutex);
    cv.wait(lk, [&]() { return call_num == 3; });
  }
  EXPECT_EQ(executor.GetMaxPendingExecutedSeq(), 3);
}

TEST(TransactionExecutorTest, MaxPendingExecuteSeqOnOutOfOrder) {
  Stats::GetGlobalStats()->Stop();
  ResDBConfig config = GetResDBConfig();

  Request request;
  request.set_seq(1);

  BatchUserRequest batch_request;
  batch_request.add_user_requests()->mutable_request()->set_data("execute_1");
  batch_request.SerializeToString(request.mutable_data());

  std::mutex mutex;
  std::condition_variable cv;
  std::atomic<int> call_num = 0;
  SystemInfo system_info(config);
  TransactionExecutor executor(
      config,
      [&](std::unique_ptr<Request>, std::unique_ptr<BatchUserResponse> resp) {},
      &system_info, std::make_unique<MockTransactionManager>(true));

  executor.SetSeqUpdateNotifyFunc([&](uint64_t seq) {
    std::unique_lock<std::mutex> lk(mutex);
    cv.notify_one();
    call_num++;
  });

  EXPECT_EQ(executor.Commit(std::make_unique<Request>(request)), 0);
  {
    std::unique_lock<std::mutex> lk(mutex);
    cv.wait(lk, [&]() { return call_num >= 1; });
  }
  EXPECT_LE(executor.GetMaxPendingExecutedSeq(), 1);

  request.set_seq(3);
  EXPECT_EQ(executor.Commit(std::make_unique<Request>(request)), 0);
  EXPECT_EQ(executor.GetMaxPendingExecutedSeq(), 1);

  request.set_seq(2);
  EXPECT_EQ(executor.Commit(std::make_unique<Request>(request)), 0);
  {
    std::unique_lock<std::mutex> lk(mutex);
    cv.wait(lk, [&]() { return call_num == 3; });
  }
  EXPECT_EQ(executor.GetMaxPendingExecutedSeq(), 3);
}

TEST(TransactionExecutorTest, ExecuteWaitInOrder) {
  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();

  ResDBConfig config = GetResDBConfig();

  SystemInfo system_info(config);
  auto mock_executor = std::make_unique<MockTransactionExecutorDataImpl>();

  EXPECT_CALL(*mock_executor, ExecuteData)
      .WillOnce(Invoke([&](const std::string& input) {
        EXPECT_EQ(input, "execute_1");
        return nullptr;
      }))
      .WillOnce(Invoke([&](const std::string& input) {
        EXPECT_EQ(input, "execute_2");
        done.set_value(true);
        return nullptr;
      }));

  TransactionExecutor executor(
      config,
      [&](std::unique_ptr<Request>, std::unique_ptr<BatchUserResponse> resp) {},
      &system_info, std::move(mock_executor));

  Request request;
  request.set_seq(2);
  BatchUserRequest batch_request;
  batch_request.add_user_requests()->mutable_request()->set_data("execute_2");
  batch_request.SerializeToString(request.mutable_data());
  EXPECT_EQ(executor.Commit(std::make_unique<Request>(request)), 0);

  request.set_seq(1);
  batch_request.clear_user_requests();
  batch_request.add_user_requests()->mutable_request()->set_data("execute_1");
  batch_request.SerializeToString(request.mutable_data());
  EXPECT_EQ(executor.Commit(std::make_unique<Request>(request)), 0);

  done_future.get();
}

TEST(TransactionExecutorTest, ExecuteWaitOutOfOrder) {
  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();

  ResDBConfig config = GetResDBConfig();

  SystemInfo system_info(config);
  auto mock_executor = std::make_unique<MockTransactionManager>(true);

  EXPECT_CALL(*mock_executor, ExecuteBatch)
      .WillOnce(Invoke([&](const BatchUserRequest& request) {
        EXPECT_EQ(request.seq(), 4);
        return nullptr;
      }))
      .WillOnce(Invoke([&](const BatchUserRequest& request) {
        EXPECT_EQ(request.seq(), 1);
        done.set_value(true);
        return nullptr;
      }));

  TransactionExecutor executor(
      config,
      [&](std::unique_ptr<Request>, std::unique_ptr<BatchUserResponse> resp) {},
      &system_info, std::move(mock_executor));

  Request request;
  request.set_seq(4);
  BatchUserRequest batch_request;
  batch_request.add_user_requests()->mutable_request()->set_data("execute_2");
  batch_request.SerializeToString(request.mutable_data());
  EXPECT_EQ(executor.Commit(std::make_unique<Request>(request)), 0);

  request.set_seq(1);
  batch_request.clear_user_requests();
  batch_request.add_user_requests()->mutable_request()->set_data("execute_1");
  batch_request.SerializeToString(request.mutable_data());
  EXPECT_EQ(executor.Commit(std::make_unique<Request>(request)), 0);

  done_future.get();
}

TEST(TransactionExecutorTest, CallBack) {
  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();

  ResDBConfig config = GetResDBConfig();

  Request request;
  request.set_seq(1);
  BatchUserRequest batch_request;
  batch_request.add_user_requests()->mutable_request()->set_data("execute_1");
  batch_request.SerializeToString(request.mutable_data());

  SystemInfo system_info(config);
  auto mock_executor = std::make_unique<MockTransactionExecutorDataImpl>();

  EXPECT_CALL(*mock_executor, ExecuteData)
      .WillOnce(Invoke([&](const std::string& input) {
        EXPECT_EQ(input, "execute_1");
        return nullptr;
      }));

  TransactionExecutor executor(
      config,
      [&](std::unique_ptr<Request> call_request,
          std::unique_ptr<BatchUserResponse> resp) {
        EXPECT_EQ(call_request->seq(), 1);
        done.set_value(true);
      },
      &system_info, std::move(mock_executor));

  EXPECT_EQ(executor.Commit(std::make_unique<Request>(request)), 0);

  done_future.get();
}

}  // namespace

}  // namespace resdb

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

#include "execution/transaction_executor.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <future>

#include "common/test/test_macros.h"
#include "execution/mock_transaction_executor_impl.h"

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

  BatchClientRequest batch_request;
  batch_request.add_client_requests()->mutable_request()->set_data("execute_1");
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
      [&](std::unique_ptr<Request>, std::unique_ptr<BatchClientResponse> resp) {
      },
      &system_info, std::move(mock_executor));

  EXPECT_EQ(executor.Commit(std::make_unique<Request>(request)), 0);

  done_future.get();
}

TEST(TransactionExecutorTest, MaxPendingExecuteSeq) {
  Stats::GetGlobalStats()->Stop();
  ResDBConfig config = GetResDBConfig();

  Request request;
  request.set_seq(1);

  BatchClientRequest batch_request;
  batch_request.add_client_requests()->mutable_request()->set_data("execute_1");
  batch_request.SerializeToString(request.mutable_data());

  std::mutex mutex;
  std::condition_variable cv;
  std::atomic<int> call_num = 0;
  SystemInfo system_info(config);
  TransactionExecutor executor(
      config,
      [&](std::unique_ptr<Request>, std::unique_ptr<BatchClientResponse> resp) {
      },
      &system_info, std::make_unique<MockTransactionExecutorImpl>());

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

  BatchClientRequest batch_request;
  batch_request.add_client_requests()->mutable_request()->set_data("execute_1");
  batch_request.SerializeToString(request.mutable_data());

  std::mutex mutex;
  std::condition_variable cv;
  std::atomic<int> call_num = 0;
  SystemInfo system_info(config);
  TransactionExecutor executor(
      config,
      [&](std::unique_ptr<Request>, std::unique_ptr<BatchClientResponse> resp) {
      },
      &system_info, std::make_unique<MockTransactionExecutorImpl>(true));

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
      [&](std::unique_ptr<Request>, std::unique_ptr<BatchClientResponse> resp) {
      },
      &system_info, std::move(mock_executor));

  Request request;
  request.set_seq(2);
  BatchClientRequest batch_request;
  batch_request.add_client_requests()->mutable_request()->set_data("execute_2");
  batch_request.SerializeToString(request.mutable_data());
  EXPECT_EQ(executor.Commit(std::make_unique<Request>(request)), 0);

  request.set_seq(1);
  batch_request.clear_client_requests();
  batch_request.add_client_requests()->mutable_request()->set_data("execute_1");
  batch_request.SerializeToString(request.mutable_data());
  EXPECT_EQ(executor.Commit(std::make_unique<Request>(request)), 0);

  done_future.get();
}

TEST(TransactionExecutorTest, ExecuteWaitOutOfOrder) {
  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();

  ResDBConfig config = GetResDBConfig();

  SystemInfo system_info(config);
  auto mock_executor = std::make_unique<MockTransactionExecutorImpl>(true);

  EXPECT_CALL(*mock_executor, ExecuteBatch)
      .WillOnce(Invoke([&](const BatchClientRequest& request) {
        EXPECT_EQ(request.seq(), 4);
        return nullptr;
      }))
      .WillOnce(Invoke([&](const BatchClientRequest& request) {
        EXPECT_EQ(request.seq(), 1);
        done.set_value(true);
        return nullptr;
      }));

  TransactionExecutor executor(
      config,
      [&](std::unique_ptr<Request>, std::unique_ptr<BatchClientResponse> resp) {
      },
      &system_info, std::move(mock_executor));

  Request request;
  request.set_seq(4);
  BatchClientRequest batch_request;
  batch_request.add_client_requests()->mutable_request()->set_data("execute_2");
  batch_request.SerializeToString(request.mutable_data());
  EXPECT_EQ(executor.Commit(std::make_unique<Request>(request)), 0);

  request.set_seq(1);
  batch_request.clear_client_requests();
  batch_request.add_client_requests()->mutable_request()->set_data("execute_1");
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
  BatchClientRequest batch_request;
  batch_request.add_client_requests()->mutable_request()->set_data("execute_1");
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
          std::unique_ptr<BatchClientResponse> resp) {
        EXPECT_EQ(call_request->seq(), 1);
        done.set_value(true);
      },
      &system_info, std::move(mock_executor));

  EXPECT_EQ(executor.Commit(std::make_unique<Request>(request)), 0);

  done_future.get();
}

}  // namespace

}  // namespace resdb

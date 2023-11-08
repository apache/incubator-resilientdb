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

#include "executor/kv/quecc_executor.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <future>

#include "chain/storage/mock_storage.h"
#include "platform/config/resdb_config_utils.h"
#include "proto/kv/kv.pb.h"
namespace resdb {
namespace {

using ::testing::Invoke;
using ::testing::Return;
using ::testing::Test;

class QueccTest : public Test {
 public:
  QueccTest() {
    auto mock_storage = std::make_unique<MockStorage>();
    mock_storage_ptr_ = mock_storage.get();
    impl_ = std::make_unique<QueccExecutor>(
        std::make_unique<ChainState>(std::move(mock_storage)));
  }

  int Set(const std::string& key, const std::string& value) {
    KVRequest request;
    request.set_cmd(KVRequest::SET);
    request.set_key(key);
    request.set_value(value);

    std::string str;
    if (!request.SerializeToString(&str)) {
      return -1;
    }

    impl_->ExecuteData(str);
    return 0;
  }

  std::string Get(const std::string& key) {
    KVRequest request;
    request.set_cmd(KVRequest::GET);
    request.set_key(key);

    std::string str;
    if (!request.SerializeToString(&str)) {
      return "";
    }
    auto resp = impl_->ExecuteData(str);
    if (resp == nullptr) {
      return "";
    }
    KVResponse kv_response;
    if (!kv_response.ParseFromString(*resp)) {
      return "";
    }
    return kv_response.value();
  }

  std::string GetAllValues() {
    KVRequest request;
    request.set_cmd(KVRequest::GETALLVALUES);

    std::string str;
    if (!request.SerializeToString(&str)) {
      return "";
    }
    auto resp = impl_->ExecuteData(str);
    if (resp == nullptr) {
      return "";
    }
    KVResponse kv_response;
    if (!kv_response.ParseFromString(*resp)) {
      return "";
    }
    return kv_response.value();
  }

  std::string GetRange(const std::string& min_key, const std::string& max_key) {
    KVRequest request;
    request.set_cmd(KVRequest::GETRANGE);
    request.set_key(min_key);
    request.set_value(max_key);

    std::string str;
    if (!request.SerializeToString(&str)) {
      return "";
    }
    auto resp = impl_->ExecuteData(str);
    if (resp == nullptr) {
      return "";
    }
    KVResponse kv_response;
    if (!kv_response.ParseFromString(*resp)) {
      return "";
    }
    return kv_response.value();
  }

 protected:
  MockStorage* mock_storage_ptr_;

 private:
  std::unique_ptr<QueccExecutor> impl_;
};

TEST_F(QueccTest, SetValue) {
  std::map<std::string, std::string> data;

  EXPECT_CALL(*mock_storage_ptr_, SetValue("test_key", "test_value"))
      .WillOnce(Invoke([&](const std::string& key, const std::string& value) {
        data[key] = value;
        return 0;
      }));

  EXPECT_CALL(*mock_storage_ptr_, GetValue("test_key"))
      .WillOnce(Invoke([&](const std::string& key) {
        std::string ret = data[key];
        return ret;
      }));

  EXPECT_CALL(*mock_storage_ptr_, GetAllValues())
      .WillOnce(Return("[]"))
      .WillOnce(Return("[test_value]"));

  EXPECT_CALL(*mock_storage_ptr_, GetRange("a", "z"))
      .WillOnce(Return("[test_value]"));

  EXPECT_EQ(GetAllValues(), "[]");
  EXPECT_EQ(Set("test_key", "test_value"), 0);
  EXPECT_EQ(Get("test_key"), "test_value");

  // GetAllValues and GetRange may be out of order for in-memory, so we test up to
  // 1 key-value pair
  EXPECT_EQ(GetAllValues(), "[test_value]");
  EXPECT_EQ(GetRange("a", "z"), "[test_value]");
}

TEST_F(QueccTest, ExecuteOne) {
  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();

  KVRequest request;
  request.set_cmd(KVRequest::SET);
  request.set_key("test");
  request.set_value("1234");

  BatchUserRequest batch_request;
  std::string str;
  if (!request.SerializeToString(&str)) {
    ADD_FAILURE();
  }
  batch_request.add_user_requests()->mutable_request()->set_data(str);
  auto mock_storage = std::make_unique<MockStorage>();
  QueccExecutor executor(std::make_unique<ChainState>(std::move(mock_storage)));

  // Test expects ExecuteData call, checks if batch_user_response has a response
  // for the one txn
  EXPECT_EQ(executor.ExecuteBatch(batch_request).get()->response_size(), 1);
  done.set_value(true);
  done_future.get();
}

TEST_F(QueccTest, ExecuteMany) {
  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();
  BatchUserRequest batch_request;

  vector<string> keys = {"test1", "test2", "test3", "test4", "test5"};

  for (int i = 0; i < 10; i++) {
    KVRequest request;
    request.set_cmd(KVRequest::SET);
    request.set_key(keys[i % 5]);
    request.set_value(to_string(i));

    std::string str;
    if (!request.SerializeToString(&str)) {
      ADD_FAILURE();
    }
    batch_request.add_user_requests()->mutable_request()->set_data(str);
  }

  auto mock_storage = std::make_unique<MockStorage>();

  QueccExecutor executor(std::make_unique<ChainState>(std::move(mock_storage)));

  // Test expects ExecuteData call, checks if batch_user_response has a response
  // for the one txn
  EXPECT_EQ(executor.ExecuteBatch(batch_request).get()->response_size(), 10);
  done.set_value(true);
  done_future.get();
}

TEST_F(QueccTest, ExecuteManyEqualSplitBenchmark) {
  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();
  BatchUserRequest batch_request;

  vector<string> keys = {"test1", "test2", "test3", "test4", "test5"};

  for (int i = 0; i < 1000; i++) {
    KVRequest request;
    request.set_cmd(KVRequest::SET);
    request.set_key(keys[i % 5]);
    request.set_value(to_string(i));

    std::string str;
    if (!request.SerializeToString(&str)) {
      ADD_FAILURE();
    }
    batch_request.add_user_requests()->mutable_request()->set_data(str);
  }

  auto mock_storage = std::make_unique<MockStorage>();

  QueccExecutor executor(std::make_unique<ChainState>(std::move(mock_storage)));

  // Test expects ExecuteData call, checks if batch_user_response has a response
  // for the one txn
  auto start_time = std::chrono::high_resolution_clock::now();
  EXPECT_EQ(executor.ExecuteBatch(batch_request).get()->response_size(), 1000);
  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
      end_time - start_time);
  LOG(ERROR) << "Time Taken: " << duration.count();
  done.set_value(true);
  done_future.get();
}

TEST_F(QueccTest, ExecuteManySameKeyBenchmark) {
  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();
  BatchUserRequest batch_request;

  vector<string> keys = {"test1", "test2", "test3", "test4", "test5"};

  for (int i = 0; i < 1000; i++) {
    KVRequest request;
    request.set_cmd(KVRequest::SET);
    request.set_key(keys[0]);
    request.set_value(to_string(i));

    std::string str;
    if (!request.SerializeToString(&str)) {
      ADD_FAILURE();
    }
    batch_request.add_user_requests()->mutable_request()->set_data(str);
  }

  auto mock_storage = std::make_unique<MockStorage>();

  QueccExecutor executor(std::make_unique<ChainState>(std::move(mock_storage)));

  // Test expects ExecuteData call, checks if batch_user_response has a response
  // for the one txn
  auto start_time = std::chrono::high_resolution_clock::now();
  EXPECT_EQ(executor.ExecuteBatch(batch_request).get()->response_size(), 1000);
  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
      end_time - start_time);
  LOG(ERROR) << "Time Taken: " << duration.count();
  done.set_value(true);
  done_future.get();
}

TEST_F(QueccTest, TwoBatches) {
  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();
  BatchUserRequest batch_request;

  vector<string> keys = {"test1", "test2", "test3", "test4", "test5"};

  for (int i = 0; i < 10; i++) {
    KVRequest request;
    request.set_cmd(KVRequest::SET);
    request.set_key(keys[i % 5]);
    request.set_value(to_string(i));

    std::string str;
    if (!request.SerializeToString(&str)) {
      ADD_FAILURE();
    }
    batch_request.add_user_requests()->mutable_request()->set_data(str);
  }

  auto mock_storage = std::make_unique<MockStorage>();

  QueccExecutor executor(std::make_unique<ChainState>(std::move(mock_storage)));

  // Test expects ExecuteData call, checks if batch_user_response has a response
  // for the one txn
  EXPECT_EQ(executor.ExecuteBatch(batch_request).get()->response_size(), 10);
  BatchUserRequest batch_request2;

  for (int i = 0; i < 10; i++) {
    KVRequest request;
    request.set_cmd(KVRequest::GET);
    request.set_key(keys[i % 5]);
    request.set_value(to_string(i));

    std::string str;
    if (!request.SerializeToString(&str)) {
      ADD_FAILURE();
    }
    batch_request2.add_user_requests()->mutable_request()->set_data(str);
  }
  EXPECT_EQ(executor.ExecuteBatch(batch_request2).get()->response_size(), 10);

  done.set_value(true);
  done_future.get();
}

}  // namespace
}  // namespace resdb

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

#include "executor/kv/kv_executor.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "platform/config/resdb_config_utils.h"
#include "proto/kv/kv.pb.h"
#include "storage/mock_storage.h"

namespace resdb {
namespace {

using ::testing::Return;
using ::testing::Test;

class KVExecutorTest : public Test {
 public:
  KVExecutorTest() {
    auto mock_storage = std::make_unique<MockStorage>();
    mock_storage_ptr_ = mock_storage.get();
    impl_ = std::make_unique<KVExecutor>(std::move(mock_storage));
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

  std::string GetValues() {
    KVRequest request;
    request.set_cmd(KVRequest::GETVALUES);

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
  std::unique_ptr<KVExecutor> impl_;
};

TEST_F(KVExecutorTest, SetValue) {
  EXPECT_CALL(*mock_storage_ptr_, SetValue("test_key", "test_value"))
      .WillOnce(Return(0));
  EXPECT_CALL(*mock_storage_ptr_, GetValue("test_key"))
      .WillOnce(Return("test_value"));
  EXPECT_CALL(*mock_storage_ptr_, GetAllValues())
      .WillOnce(Return("[]"))
      .WillOnce(Return("[test_value]"));

  EXPECT_CALL(*mock_storage_ptr_, GetRange("a", "z"))
      .WillOnce(Return("[test_value]"));

  EXPECT_EQ(GetValues(), "[]");
  EXPECT_EQ(Set("test_key", "test_value"), 0);
  EXPECT_EQ(Get("test_key"), "test_value");

  // GetValues and GetRange may be out of order for in-memory, so we test up to
  // 1 key-value pair
  EXPECT_EQ(GetValues(), "[test_value]");
  EXPECT_EQ(GetRange("a", "z"), "[test_value]");
}

}  // namespace

}  // namespace resdb

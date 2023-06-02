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

#include "chain/storage/res_leveldb.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>

namespace resdb {

namespace {

using ::testing::Test;

class ResLevelDBDurableTest : public Test {
 public:
  ResLevelDBDurableTest() { Reset(); }
  ~ResLevelDBDurableTest() {}
  int Set(const std::string& key, const std::string& value) {
    resdb::ResConfigData config_data;
    config_data.mutable_leveldb_info()->set_path(path_);
    return NewResLevelDB(NULL, config_data)->SetValue(key, value);
  }

  std::string Get(const std::string& key) {
    resdb::ResConfigData config_data;
    config_data.mutable_leveldb_info()->set_path(path_);
    return NewResLevelDB(NULL, config_data)->GetValue(key);
  }

  std::string GetAllValues() {
    resdb::ResConfigData config_data;
    config_data.mutable_leveldb_info()->set_path(path_);
    return NewResLevelDB(NULL, config_data)->GetAllValues();
  }

  std::string GetRange(const std::string& min_key, const std::string& max_key) {
    resdb::ResConfigData config_data;
    config_data.mutable_leveldb_info()->set_path(path_);
    return NewResLevelDB(NULL, config_data)->GetRange(min_key, max_key);
  }

  void Reset() { std::filesystem::remove_all(path_.c_str()); }

 private:
  std::string path_ = "/tmp/leveldb_test";
};

TEST_F(ResLevelDBDurableTest, GetEmptyValue) {
  EXPECT_EQ(Get("empty_key"), "");
}

TEST_F(ResLevelDBDurableTest, SetValue) {
  EXPECT_EQ(Set("test_key", "test_value"), 0);
}

TEST_F(ResLevelDBDurableTest, GetValue) {
  EXPECT_EQ(Set("test_key", "test_value"), 0);
  EXPECT_EQ(Get("test_key"), "test_value");
}

TEST_F(ResLevelDBDurableTest, SetNewValue) {
  EXPECT_EQ(Set("test_key", "new_value"), 0);
}

TEST_F(ResLevelDBDurableTest, GetNewValue) { EXPECT_EQ(Get("test_key"), ""); }

TEST_F(ResLevelDBDurableTest, GetAllValues) {
  EXPECT_EQ(Set("a", "a"), 0);
  EXPECT_EQ(Set("b", "b"), 0);
  EXPECT_EQ(Set("c", "c"), 0);
  EXPECT_EQ(GetAllValues(), "[a,b,c]");
}

TEST_F(ResLevelDBDurableTest, GetRange) {
  EXPECT_EQ(Set("key1", "value1"), 0);
  EXPECT_EQ(Set("key2", "value2"), 0);
  EXPECT_EQ(Set("key3", "value3"), 0);
  EXPECT_EQ(GetRange("key1", "key3"), "[value1,value2,value3]");
  EXPECT_EQ(GetRange("key1", "key2"), "[value1,value2]");
  EXPECT_EQ(GetRange("key4", "key5"), "[]");
}

}  // namespace

}  // namespace resdb

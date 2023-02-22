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

#include "durable_layer/rocksdb_durable.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {

using ::testing::Test;

class RocksDBDurableTest : public Test {
 public:
  int Set(const std::string& key, const std::string& value) {
    ResConfigData config_data;
    config_data.mutable_rocksdb_info()->set_path(path_);
    RocksDurable rocksdb_layer(NULL, config_data);

    rocksdb_layer.SetValue(key, value);
    return 0;
  }

  std::string Get(const std::string& key) {
    ResConfigData config_data;
    config_data.mutable_rocksdb_info()->set_path(path_);
    RocksDurable rocksdb_layer(NULL, config_data);

    std::string value = rocksdb_layer.GetValue(key);
    return value;
  }

  std::string GetAllValues() {
    ResConfigData config_data;
    config_data.mutable_rocksdb_info()->set_path(path_);
    RocksDurable rocksdb_layer(NULL, config_data);

    std::string values = rocksdb_layer.GetAllValues();
    return values;
  }

  std::string GetRange(const std::string& min_key, const std::string& max_key) {
    ResConfigData config_data;
    config_data.mutable_rocksdb_info()->set_path(path_);
    RocksDurable rocksdb_layer(NULL, config_data);

    std::string values = rocksdb_layer.GetRange(min_key, max_key);
    return values;
  }

 private:
  std::string path_ = "/tmp/rocksdb_test";
};

TEST_F(RocksDBDurableTest, GetEmptyValue) { EXPECT_EQ(Get("empty_key"), ""); }

TEST_F(RocksDBDurableTest, SetValue) {
  EXPECT_EQ(Set("test_key", "test_value"), 0);
}

TEST_F(RocksDBDurableTest, GetValue) {
  EXPECT_EQ(Get("test_key"), "test_value");
}

TEST_F(RocksDBDurableTest, SetNewValue) {
  EXPECT_EQ(Set("test_key", "new_value"), 0);
}

TEST_F(RocksDBDurableTest, GetNewValue) {
  EXPECT_EQ(Get("test_key"), "new_value");
}

TEST_F(RocksDBDurableTest, GetAllValues) {
  EXPECT_EQ(Set("a", "a"), 0);
  EXPECT_EQ(Set("b", "b"), 0);
  EXPECT_EQ(Set("c", "c"), 0);
  EXPECT_EQ(GetAllValues(), "[a,b,c,new_value]");
}

TEST_F(RocksDBDurableTest, GetRange) {
  EXPECT_EQ(Set("key1", "value1"), 0);
  EXPECT_EQ(Set("key2", "value2"), 0);
  EXPECT_EQ(Set("key3", "value3"), 0);
  EXPECT_EQ(GetRange("key1", "key3"), "[value1,value2,value3]");
  EXPECT_EQ(GetRange("key1", "key2"), "[value1,value2]");
  EXPECT_EQ(GetRange("key4", "key5"), "[]");
}

}  // namespace

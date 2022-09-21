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

    rocksdb_layer.setDurable(key, value);
    return 0;
  }

  std::string Get(const std::string& key) {
    ResConfigData config_data;
    config_data.mutable_rocksdb_info()->set_path(path_);
    RocksDurable rocksdb_layer(NULL, config_data);

    std::string value = rocksdb_layer.getDurable(key);
    return value;
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

}  // namespace

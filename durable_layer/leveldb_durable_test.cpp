#include "durable_layer/leveldb_durable.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {

using ::testing::Test;

class LevelDBDurableTest : public Test {
 public:
  int Set(const std::string& key, const std::string& value) {
    ResConfigData config_data;
    config_data.mutable_leveldb_info()->set_path(path_);
    LevelDurable leveldb_layer(NULL, config_data);

    leveldb_layer.setDurable(key, value);
    return 0;
  }

  std::string Get(const std::string& key) {
    ResConfigData config_data;
    config_data.mutable_leveldb_info()->set_path(path_);
    LevelDurable leveldb_layer(NULL, config_data);

    std::string value = leveldb_layer.getDurable(key);
    return value;
  }

 private:
  std::string path_ = "/tmp/leveldb_test";
};

TEST_F(LevelDBDurableTest, GetEmptyValue) { EXPECT_EQ(Get("empty_key"), ""); }

TEST_F(LevelDBDurableTest, SetValue) {
  EXPECT_EQ(Set("test_key", "test_value"), 0);
}

TEST_F(LevelDBDurableTest, GetValue) {
  EXPECT_EQ(Get("test_key"), "test_value");
}

TEST_F(LevelDBDurableTest, SetNewValue) {
  EXPECT_EQ(Set("test_key", "new_value"), 0);
}

TEST_F(LevelDBDurableTest, GetNewValue) {
  EXPECT_EQ(Get("test_key"), "new_value");
}

}  // namespace

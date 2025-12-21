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

#include "chain/storage/leveldb.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>

namespace resdb {
namespace storage {
namespace {

enum class CacheConfig { DISABLED, ENABLED };

class TestableResLevelDB : public ResLevelDB {
 public:
  using ResLevelDB::block_cache_;
  using ResLevelDB::global_stats_;
  using ResLevelDB::ResLevelDB;
};

class LevelDBTest : public ::testing::TestWithParam<CacheConfig> {
 protected:
  LevelDBTest() {
    LevelDBInfo config;
    if (GetParam() == CacheConfig::ENABLED) {
      config.set_enable_block_cache(true);
      config.set_block_cache_capacity(1000);
    } else {
      Reset();
    }
    storage = std::make_unique<TestableResLevelDB>(config);
  }

 protected:
  std::unique_ptr<TestableResLevelDB> storage;
  std::string path_ = "/tmp/leveldb_test";

 private:
  void Reset() { std::filesystem::remove_all(path_.c_str()); }
};

TEST_P(LevelDBTest, BlockCacheEnabled) {
  if (GetParam() == CacheConfig::ENABLED) {
    EXPECT_TRUE(storage->block_cache_ != nullptr);
    EXPECT_TRUE(storage->block_cache_->GetCapacity() == 1000);
  } else {
    EXPECT_TRUE(storage->block_cache_ == nullptr);
    EXPECT_FALSE(storage->UpdateMetrics());
  }
}

TEST_P(LevelDBTest, AddValueAndCheckCache) {
  if (GetParam() == CacheConfig::ENABLED) {
    // Add a value
    std::string key = "test_key";
    std::string value = "test_value";
    EXPECT_EQ(storage->SetValue(key, value), 0);

    // Check if CacheHit is incremented in the Stats class
    EXPECT_TRUE(storage->block_cache_->Get(key) == "test_value");
    EXPECT_EQ(storage->block_cache_->GetCacheHits(), 1);
  }
}

TEST_P(LevelDBTest, CacheEvictionPolicy) {
  if (GetParam() == CacheConfig::ENABLED) {
    // Insert 1000 values
    for (int i = 1; i <= 1000; ++i) {
      std::string key = "key_" + std::to_string(i);
      std::string value = "value_" + std::to_string(i);
      EXPECT_EQ(storage->SetValue(key, value), 0);
    }

    // Insert the 1001st value
    std::string key_1001 = "key_1001";
    std::string value_1001 = "value_1001";
    EXPECT_EQ(storage->SetValue(key_1001, value_1001), 0);

    // Check that the 1001st value is not present in the cache
    EXPECT_TRUE(storage->GetValue("key_1") == "value_1");
    EXPECT_EQ(storage->block_cache_->GetCacheMisses(), 1);

    // Expect key_2 to be present in cache and hence a cache hit
    EXPECT_TRUE(storage->GetValue("key_2") == "value_2");
    EXPECT_EQ(storage->block_cache_->GetCacheHits(), 1);

    EXPECT_TRUE(storage->UpdateMetrics());
  }
}

INSTANTIATE_TEST_CASE_P(LevelDBTest, LevelDBTest,
                        ::testing::Values(CacheConfig::ENABLED,
                                          CacheConfig::DISABLED));

}  // namespace
}  // namespace storage
}  // namespace resdb
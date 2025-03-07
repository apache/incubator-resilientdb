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

#include <cstddef>
#include <filesystem>

namespace resdb {
namespace storage {
namespace {

class TestableResLevelDB : public ResLevelDB {
 public:
  using ResLevelDB::block_cache_;
  using ResLevelDB::ResLevelDB;
};

class LevelDBBlockCacheTest : public ::testing::Test {
 protected:
  std::unique_ptr<TestableResLevelDB> storage;
  std::string path_ = "/tmp/leveldb_test";

  void TearDown() override { std::filesystem::remove_all(path_.c_str()); }
};

TEST_F(LevelDBBlockCacheTest, BlockCacheEnabled) {
  LevelDBInfo config;
  config.set_enable_block_cache(true);
  auto storage = std::make_unique<TestableResLevelDB>(config);
  EXPECT_TRUE(storage->block_cache_ != nullptr);
}

}  // namespace
}  // namespace storage
}  // namespace resdb
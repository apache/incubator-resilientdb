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

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <filesystem>

#include "chain/storage/leveldb.h"
#include "chain/storage/memory_db.h"
#include "chain/storage/ipfs_client.h"
#include "chain/storage/tiered_storage.h"

namespace resdb {
namespace storage {
namespace {

class StubIPFSClient : public IPFSClient {
 public:
  StubIPFSClient(bool enabled) : enabled_(enabled) {}

  std::string Add(const std::string& data) override {
    return data.empty() ? "" : "stub_cid_" + data.substr(0, 10);
  }
  std::string AddDAG(const std::string& json_data) override {
    return "stub_dag_cid";
  }
  std::string Cat(const std::string& cid) override {
    return "fetched_" + cid;
  }
  std::string GetDAG(const std::string& cid) override {
    return "dag_" + cid;
  }
  bool Exists(const std::string& cid) override {
    return !cid.empty();
  }
  bool IsEnabled() const override {
    return enabled_;
  }

 private:
  bool enabled_;
};

class TieredStorageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    std::filesystem::remove_all(leveldb_path_.c_str());
  }

  void TearDown() override {
    std::filesystem::remove_all(leveldb_path_.c_str());
  }

  std::string leveldb_path_ = "/tmp/tiered_storage_test";
};

TEST_F(TieredStorageTest, CreateDisabled) {
  TieredStorageConfig config;
  config.set_enabled(false);

  auto warm = NewResLevelDB(leveldb_path_);
  auto hot = NewMemoryDB();
  std::unique_ptr<IPFSClient> ipfs(new StubIPFSClient(false));

  auto storage = std::make_unique<TieredStorage>(
      std::move(hot), std::move(warm), std::move(ipfs), config);

  EXPECT_FALSE(storage->IsTiered());
}

TEST_F(TieredStorageTest, CreateEnabled) {
  TieredStorageConfig config;
  config.set_enabled(true);
  config.set_cold_threshold_checkpoint(2);

  auto warm = NewResLevelDB(leveldb_path_);
  auto hot = NewMemoryDB();
  std::unique_ptr<IPFSClient> ipfs(new StubIPFSClient(true));

  auto storage = std::make_unique<TieredStorage>(
      std::move(hot), std::move(warm), std::move(ipfs), config);

  EXPECT_TRUE(storage->IsTiered());
  EXPECT_TRUE(storage->IsColdEnabled());
}

TEST_F(TieredStorageTest, SetValueHotStorage) {
  TieredStorageConfig config;
  config.set_enabled(false);

  auto warm = NewResLevelDB(leveldb_path_);
  auto hot = NewMemoryDB();
  std::unique_ptr<IPFSClient> ipfs(new StubIPFSClient(false));

  auto storage = std::make_unique<TieredStorage>(
      std::move(hot), std::move(warm), std::move(ipfs), config);

  EXPECT_EQ(storage->SetValue("test_key", "test_value"), 0);
  EXPECT_EQ(storage->GetValue("test_key"), "test_value");
}

TEST_F(TieredStorageTest, FallbackToWarmStorage) {
  TieredStorageConfig config;
  config.set_enabled(false);

  auto warm = NewResLevelDB(leveldb_path_);
  auto hot = NewMemoryDB();
  warm->SetValue("warm_key", "warm_value");
  std::unique_ptr<IPFSClient> ipfs(new StubIPFSClient(false));

  auto storage = std::make_unique<TieredStorage>(
      std::move(hot), std::move(warm), std::move(ipfs), config);

  EXPECT_EQ(storage->GetValue("warm_key"), "warm_value");
}

TEST_F(TieredStorageTest, GetColdThreshold) {
  TieredStorageConfig config;
  config.set_enabled(true);
  config.set_cold_threshold_checkpoint(5);

  auto warm = NewResLevelDB(leveldb_path_);
  auto hot = NewMemoryDB();
  std::unique_ptr<IPFSClient> ipfs(new StubIPFSClient(true));

  auto storage = std::make_unique<TieredStorage>(
      std::move(hot), std::move(warm), std::move(ipfs), config);

  EXPECT_EQ(storage->GetColdThreshold(), 5);
  storage->SetColdThreshold(10);
  EXPECT_EQ(storage->GetColdThreshold(), 10);
}

TEST_F(TieredStorageTest, GetLastCheckpointFromWarm) {
  TieredStorageConfig config;
  config.set_enabled(false);

  auto warm = NewResLevelDB(leveldb_path_);
  auto hot = NewMemoryDB();
  std::unique_ptr<IPFSClient> ipfs(new StubIPFSClient(false));

  auto storage = std::make_unique<TieredStorage>(
      std::move(hot), std::move(warm), std::move(ipfs), config);

  warm->SetValueWithSeq("key1", "value1", 100);
  
  EXPECT_EQ(storage->GetLastCheckpoint(), 100);
}

TEST_F(TieredStorageTest, GetValueWithSeq) {
  TieredStorageConfig config;
  config.set_enabled(false);

  auto warm = NewResLevelDB(leveldb_path_);
  auto hot = NewMemoryDB();
  std::unique_ptr<IPFSClient> ipfs(new StubIPFSClient(false));

  auto storage = std::make_unique<TieredStorage>(
      std::move(hot), std::move(warm), std::move(ipfs), config);

  hot->SetValueWithSeq("key1", "value1", 1);
  warm->SetValueWithSeq("key2", "value2", 2);
  
  auto result1 = storage->GetValueWithSeq("key1", 0);
  EXPECT_EQ(result1.first, "value1");
  EXPECT_EQ(result1.second, 1);
  
  auto result2 = storage->GetValueWithSeq("key2", 0);
  EXPECT_EQ(result2.first, "value2");
  EXPECT_EQ(result2.second, 2);
}

TEST_F(TieredStorageTest, SetValueWithVersion) {
  TieredStorageConfig config;
  config.set_enabled(false);

  auto warm = NewResLevelDB(leveldb_path_);
  auto hot = NewMemoryDB();
  std::unique_ptr<IPFSClient> ipfs(new StubIPFSClient(false));

  auto storage = std::make_unique<TieredStorage>(
      std::move(hot), std::move(warm), std::move(ipfs), config);

  EXPECT_EQ(storage->SetValueWithVersion("key1", "value1", 1), 0);
  
  auto result = storage->GetValueWithVersion("key1", 1);
  EXPECT_EQ(result.first, "value1");
  EXPECT_EQ(result.second, 1);
}

}  // namespace
}  // namespace storage
}  // namespace resdb
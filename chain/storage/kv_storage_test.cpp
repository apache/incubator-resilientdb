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
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>

#include "chain/storage/leveldb.h"
#include "chain/storage/memory_db.h"
#include "chain/storage/rocksdb.h"



namespace resdb {
namespace storage {
namespace {

enum StorageType { MEM = 0, LEVELDB = 1, LEVELDB_WITH_BLOCK_CACHE = 2, ROCKSDB = 3 };

class KVStorageTest : public ::testing::TestWithParam<StorageType> {
 protected:
  KVStorageTest() {
    StorageType t = GetParam();
    LevelDBInfo config;
    switch (t) {
      case MEM:
        storage = NewMemoryDB();
        break;
      case LEVELDB:
        Reset();
        storage = NewResLevelDB(path_);
        break;
      case LEVELDB_WITH_BLOCK_CACHE:
        Reset();
        config.set_enable_block_cache(true);
        storage = NewResLevelDB(path_, config);
        break;
      case ROCKSDB:
        Reset();
        storage = NewResRocksDB(path_);
        break;
    }
  }

 private:
  void Reset() { std::filesystem::remove_all(path_.c_str()); }

 protected:
  std::unique_ptr<Storage> storage;
  std::string path_ = "/tmp/leveldb_test";
};

TEST_P(KVStorageTest, SetValue) {
  EXPECT_EQ(storage->SetValue("test_key", "test_value"), 0);
  EXPECT_EQ(storage->GetValue("test_key"), "test_value");
}

TEST_P(KVStorageTest, GetValue) {
  EXPECT_EQ(storage->GetValue("test_key"), "");
}

TEST_P(KVStorageTest, GetEmptyValueWithVersion) {
  EXPECT_EQ(storage->GetValueWithVersion("test_key", 0),
            std::make_pair(std::string(""), 0));
}

TEST_P(KVStorageTest, SetValueWithVersion) {
  EXPECT_EQ(storage->SetValueWithVersion("test_key", "test_value", 1), -2);

  EXPECT_EQ(storage->SetValueWithVersion("test_key", "test_value", 0), 0);

  EXPECT_EQ(storage->GetValueWithVersion("test_key", 0),
            std::make_pair(std::string("test_value"), 1));
  EXPECT_EQ(storage->GetValueWithVersion("test_key", 1),
            std::make_pair(std::string("test_value"), 1));

  EXPECT_EQ(storage->SetValueWithVersion("test_key", "test_value_v2", 2), -2);
  EXPECT_EQ(storage->SetValueWithVersion("test_key", "test_value_v2", 1), 0);

  EXPECT_EQ(storage->GetValueWithVersion("test_key", 0),
            std::make_pair(std::string("test_value_v2"), 2));

  EXPECT_EQ(storage->GetValueWithVersion("test_key", 1),
            std::make_pair(std::string("test_value"), 1));

  EXPECT_EQ(storage->GetValueWithVersion("test_key", 2),
            std::make_pair(std::string("test_value_v2"), 2));

  EXPECT_EQ(storage->GetValueWithVersion("test_key", 3),
            std::make_pair(std::string("test_value_v2"), 2));
}

TEST_P(KVStorageTest, GetAllValueWithVersion) {
  {
    std::map<std::string, std::pair<std::string, int> > expected_list{
        std::make_pair("test_key", std::make_pair("test_value", 1))};

    EXPECT_EQ(storage->SetValueWithVersion("test_key", "test_value", 0), 0);
    EXPECT_EQ(storage->GetAllItems(), expected_list);
  }

  {
    std::map<std::string, std::pair<std::string, int> > expected_list{
        std::make_pair("test_key", std::make_pair("test_value_v2", 2))};
    EXPECT_EQ(storage->SetValueWithVersion("test_key", "test_value_v2", 1), 0);
    EXPECT_EQ(storage->GetAllItems(), expected_list);
  }

  {
    std::map<std::string, std::pair<std::string, int> > expected_list{
        std::make_pair("test_key_v1", std::make_pair("test_value1", 1)),
        std::make_pair("test_key", std::make_pair("test_value_v2", 2))};
    EXPECT_EQ(storage->SetValueWithVersion("test_key_v1", "test_value1", 0), 0);
    EXPECT_EQ(storage->GetAllItems(), expected_list);
  }
}

TEST_P(KVStorageTest, GetKeyRange) {
  EXPECT_EQ(storage->SetValueWithVersion("1", "value1", 0), 0);
  EXPECT_EQ(storage->SetValueWithVersion("2", "value2", 0), 0);
  EXPECT_EQ(storage->SetValueWithVersion("3", "value3", 0), 0);
  EXPECT_EQ(storage->SetValueWithVersion("4", "value4", 0), 0);

  {
    std::map<std::string, std::pair<std::string, int> > expected_list{
        std::make_pair("1", std::make_pair("value1", 1)),
        std::make_pair("2", std::make_pair("value2", 1)),
        std::make_pair("3", std::make_pair("value3", 1)),
        std::make_pair("4", std::make_pair("value4", 1))};
    EXPECT_EQ(storage->GetKeyRange("1", "4"), expected_list);
  }

  EXPECT_EQ(storage->SetValueWithVersion("3", "value3_1", 1), 0);
  {
    std::map<std::string, std::pair<std::string, int> > expected_list{
        std::make_pair("1", std::make_pair("value1", 1)),
        std::make_pair("2", std::make_pair("value2", 1)),
        std::make_pair("3", std::make_pair("value3_1", 2)),
        std::make_pair("4", std::make_pair("value4", 1))};
    EXPECT_EQ(storage->GetKeyRange("1", "4"), expected_list);
  }
  {
    std::map<std::string, std::pair<std::string, int> > expected_list{
        std::make_pair("1", std::make_pair("value1", 1)),
        std::make_pair("2", std::make_pair("value2", 1)),
        std::make_pair("3", std::make_pair("value3_1", 2)),
    };
    EXPECT_EQ(storage->GetKeyRange("1", "3"), expected_list);
  }
  {
    std::map<std::string, std::pair<std::string, int> > expected_list{
        std::make_pair("2", std::make_pair("value2", 1)),
        std::make_pair("3", std::make_pair("value3_1", 2)),
        std::make_pair("4", std::make_pair("value4", 1))};
    EXPECT_EQ(storage->GetKeyRange("2", "4"), expected_list);
  }
  {
    std::map<std::string, std::pair<std::string, int> > expected_list{
        std::make_pair("1", std::make_pair("value1", 1)),
        std::make_pair("2", std::make_pair("value2", 1)),
        std::make_pair("3", std::make_pair("value3_1", 2)),
        std::make_pair("4", std::make_pair("value4", 1))};
    EXPECT_EQ(storage->GetKeyRange("0", "5"), expected_list);
  }
  {
    std::map<std::string, std::pair<std::string, int> > expected_list{
        std::make_pair("2", std::make_pair("value2", 1)),
        std::make_pair("3", std::make_pair("value3_1", 2)),
    };
    EXPECT_EQ(storage->GetKeyRange("2", "3"), expected_list);
  }
}

TEST_P(KVStorageTest, GetHistory) {
  {
    std::vector<std::pair<std::string, int> > expected_list{};
    EXPECT_EQ(storage->GetHistory("1", 1, 5), expected_list);
  }
  {
    std::vector<std::pair<std::string, int> > expected_list{
        std::make_pair("value3", 3), std::make_pair("value2", 2),
        std::make_pair("value1", 1)};

    EXPECT_EQ(storage->SetValueWithVersion("1", "value1", 0), 0);
    EXPECT_EQ(storage->SetValueWithVersion("1", "value2", 1), 0);
    EXPECT_EQ(storage->SetValueWithVersion("1", "value3", 2), 0);

    EXPECT_EQ(storage->GetHistory("1", 1, 5), expected_list);
  }

  {
    std::vector<std::pair<std::string, int> > expected_list{
        std::make_pair("value5", 5), std::make_pair("value4", 4),
        std::make_pair("value3", 3), std::make_pair("value2", 2),
        std::make_pair("value1", 1)};

    EXPECT_EQ(storage->SetValueWithVersion("1", "value4", 3), 0);
    EXPECT_EQ(storage->SetValueWithVersion("1", "value5", 4), 0);
    EXPECT_EQ(storage->SetValueWithVersion("1", "value6", 5), 0);
    EXPECT_EQ(storage->SetValueWithVersion("1", "value7", 6), 0);

    EXPECT_EQ(storage->GetHistory("1", 1, 5), expected_list);
  }

  {
    std::vector<std::pair<std::string, int> > expected_list{
        std::make_pair("value7", 7), std::make_pair("value6", 6)};

    EXPECT_EQ(storage->GetTopHistory("1", 2), expected_list);
  }
}

TEST_P(KVStorageTest, BlockCacheSpecificTest) {
  if (GetParam() == LEVELDB_WITH_BLOCK_CACHE) {
    std::cout << "Running BlockCacheSpecificTest for LEVELDB_WITH_BLOCK_CACHE"
              << std::endl;
  }
}

INSTANTIATE_TEST_CASE_P(KVStorageTest, KVStorageTest,
                        ::testing::Values(MEM, LEVELDB,
                                          LEVELDB_WITH_BLOCK_CACHE, ROCKSDB));

}  // namespace
}  // namespace storage
}  // namespace resdb

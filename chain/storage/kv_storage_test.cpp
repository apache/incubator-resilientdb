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

#include "chain/storage/composite_key_codec.h"
#include "chain/storage/leveldb.h"
#include "chain/storage/memory_db.h"

namespace resdb {
namespace storage {
namespace {

enum StorageType { MEM = 0, LEVELDB = 1, LEVELDB_WITH_BLOCK_CACHE = 2 };

class KVStorageTest : public ::testing::TestWithParam<StorageType> {
 protected:
  KVStorageTest() {
    StorageType t = GetParam();
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
        LevelDBInfo config;
        config.set_enable_block_cache(true);
        storage = NewResLevelDB(path_, config);
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
    std::map<std::string, std::pair<std::string, int>> expected_list{
        std::make_pair("test_key", std::make_pair("test_value", 1))};

    EXPECT_EQ(storage->SetValueWithVersion("test_key", "test_value", 0), 0);
    EXPECT_EQ(storage->GetAllItems(), expected_list);
  }

  {
    std::map<std::string, std::pair<std::string, int>> expected_list{
        std::make_pair("test_key", std::make_pair("test_value_v2", 2))};
    EXPECT_EQ(storage->SetValueWithVersion("test_key", "test_value_v2", 1), 0);
    EXPECT_EQ(storage->GetAllItems(), expected_list);
  }

  {
    std::map<std::string, std::pair<std::string, int>> expected_list{
        std::make_pair("test_key_v1", std::make_pair("test_value1", 1)),
        std::make_pair("test_key", std::make_pair("test_value_v2", 2))};
    EXPECT_EQ(storage->SetValueWithVersion("test_key_v1", "test_value1", 0), 0);
    EXPECT_EQ(storage->GetAllItems(), expected_list);
  }
}

TEST_P(KVStorageTest, SetValueWithSeq) {
  EXPECT_EQ(storage->SetValueWithSeq("test_key", "test_value", 2), 0);

  EXPECT_EQ(storage->SetValueWithSeq("test_key", "test_value", 1), -2);

  EXPECT_EQ(storage->GetValueWithSeq("test_key", static_cast<uint64_t>(1)),
            std::make_pair(std::string(""), static_cast<uint64_t>(0)));
  EXPECT_EQ(
      storage->GetValueWithSeq("test_key", 2),
      std::make_pair(std::string("test_value"), static_cast<uint64_t>(2)));

  EXPECT_EQ(storage->SetValueWithSeq("test_key", "test_value_v2", 3), 0);

  EXPECT_EQ(storage->GetValueWithSeq("test_key", 1),
            std::make_pair(std::string(""), static_cast<uint64_t>(0)));

  EXPECT_EQ(
      storage->GetValueWithSeq("test_key", 2),
      std::make_pair(std::string("test_value"), static_cast<uint64_t>(2)));

  EXPECT_EQ(
      storage->GetValueWithSeq("test_key", 3),
      std::make_pair(std::string("test_value_v2"), static_cast<uint64_t>(3)));

  EXPECT_EQ(storage->GetValueWithSeq("test_key", 4),
            std::make_pair(std::string(""), static_cast<uint64_t>(0)));

  EXPECT_EQ(
      storage->GetValueWithSeq("test_key", 0),
      std::make_pair(std::string("test_value_v2"), static_cast<uint64_t>(3)));
}

TEST_P(KVStorageTest, GetAllValueWithSeq) {
  typedef std::vector<std::pair<std::string, uint64_t>> List;
  {
    std::map<std::string, std::vector<std::pair<std::string, uint64_t>>>
        expected_list{
            std::make_pair("test_key", List{std::make_pair("test_value", 1)})};

    EXPECT_EQ(storage->SetValueWithSeq("test_key", "test_value", 1), 0);
    EXPECT_EQ(storage->GetAllItemsWithSeq(), expected_list);
  }

  {
    std::map<std::string, std::vector<std::pair<std::string, uint64_t>>>
        expected_list{std::make_pair("test_key",
                                     List{std::make_pair("test_value", 1),
                                          std::make_pair("test_value_v2", 2)})};
    EXPECT_EQ(storage->SetValueWithSeq("test_key", "test_value_v2", 2), 0);
    EXPECT_EQ(storage->GetAllItemsWithSeq(), expected_list);
  }

  {
    std::map<std::string, std::vector<std::pair<std::string, uint64_t>>>
        expected_list{std::make_pair("test_key_v1",
                                     List{std::make_pair("test_value1", 1)}),
                      std::make_pair("test_key",
                                     List{std::make_pair("test_value", 1),
                                          std::make_pair("test_value_v2", 2)})};
    EXPECT_EQ(storage->SetValueWithSeq("test_key_v1", "test_value1", 1), 0);
    EXPECT_EQ(storage->GetAllItemsWithSeq(), expected_list);
  }

  {
    storage->SetMaxHistoryNum(2);
    std::map<std::string, std::vector<std::pair<std::string, uint64_t>>>
        expected_list{
            std::make_pair("test_key_v1",
                           List{std::make_pair("test_value1", 1)}),
            std::make_pair("test_key", List{std::make_pair("test_value3", 3),
                                            std::make_pair("test_value4", 4)})};
    EXPECT_EQ(storage->SetValueWithSeq("test_key", "test_value3", 3), 0);
    EXPECT_EQ(storage->SetValueWithSeq("test_key", "test_value4", 4), 0);
    EXPECT_EQ(storage->GetAllItemsWithSeq(), expected_list);
  }
}

TEST_P(KVStorageTest, GetKeyRange) {
  EXPECT_EQ(storage->SetValueWithVersion("1", "value1", 0), 0);
  EXPECT_EQ(storage->SetValueWithVersion("2", "value2", 0), 0);
  EXPECT_EQ(storage->SetValueWithVersion("3", "value3", 0), 0);
  EXPECT_EQ(storage->SetValueWithVersion("4", "value4", 0), 0);

  {
    std::map<std::string, std::pair<std::string, int>> expected_list{
        std::make_pair("1", std::make_pair("value1", 1)),
        std::make_pair("2", std::make_pair("value2", 1)),
        std::make_pair("3", std::make_pair("value3", 1)),
        std::make_pair("4", std::make_pair("value4", 1))};
    EXPECT_EQ(storage->GetKeyRange("1", "4"), expected_list);
  }

  EXPECT_EQ(storage->SetValueWithVersion("3", "value3_1", 1), 0);
  {
    std::map<std::string, std::pair<std::string, int>> expected_list{
        std::make_pair("1", std::make_pair("value1", 1)),
        std::make_pair("2", std::make_pair("value2", 1)),
        std::make_pair("3", std::make_pair("value3_1", 2)),
        std::make_pair("4", std::make_pair("value4", 1))};
    EXPECT_EQ(storage->GetKeyRange("1", "4"), expected_list);
  }
  {
    std::map<std::string, std::pair<std::string, int>> expected_list{
        std::make_pair("1", std::make_pair("value1", 1)),
        std::make_pair("2", std::make_pair("value2", 1)),
        std::make_pair("3", std::make_pair("value3_1", 2)),
    };
    EXPECT_EQ(storage->GetKeyRange("1", "3"), expected_list);
  }
  {
    std::map<std::string, std::pair<std::string, int>> expected_list{
        std::make_pair("2", std::make_pair("value2", 1)),
        std::make_pair("3", std::make_pair("value3_1", 2)),
        std::make_pair("4", std::make_pair("value4", 1))};
    EXPECT_EQ(storage->GetKeyRange("2", "4"), expected_list);
  }
  {
    std::map<std::string, std::pair<std::string, int>> expected_list{
        std::make_pair("1", std::make_pair("value1", 1)),
        std::make_pair("2", std::make_pair("value2", 1)),
        std::make_pair("3", std::make_pair("value3_1", 2)),
        std::make_pair("4", std::make_pair("value4", 1))};
    EXPECT_EQ(storage->GetKeyRange("0", "5"), expected_list);
  }
  {
    std::map<std::string, std::pair<std::string, int>> expected_list{
        std::make_pair("2", std::make_pair("value2", 1)),
        std::make_pair("3", std::make_pair("value3_1", 2)),
    };
    EXPECT_EQ(storage->GetKeyRange("2", "3"), expected_list);
  }
}

TEST_P(KVStorageTest, GetHistory) {
  {
    std::vector<std::pair<std::string, int>> expected_list{};
    EXPECT_EQ(storage->GetHistory("1", 1, 5), expected_list);
  }
  {
    std::vector<std::pair<std::string, int>> expected_list{
        std::make_pair("value3", 3), std::make_pair("value2", 2),
        std::make_pair("value1", 1)};

    EXPECT_EQ(storage->SetValueWithVersion("1", "value1", 0), 0);
    EXPECT_EQ(storage->SetValueWithVersion("1", "value2", 1), 0);
    EXPECT_EQ(storage->SetValueWithVersion("1", "value3", 2), 0);

    EXPECT_EQ(storage->GetHistory("1", 1, 5), expected_list);
  }

  {
    std::vector<std::pair<std::string, int>> expected_list{
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
    std::vector<std::pair<std::string, int>> expected_list{
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

// Composite key tests run against all three backends. Keys are built via the
// codec because C-string literals would truncate at the embedded '\0'.

TEST_P(KVStorageTest, CreateAndRetrieveCompositeKey) {
  std::string ck1 = EncodeCompositeKey("byCity", {"Davis"}, "user:1");
  std::string ck2 = EncodeCompositeKey("byCity", {"Davis"}, "user:2");
  std::string ck3 = EncodeCompositeKey("byCity", {"Davis"}, "user:3");

  EXPECT_EQ(storage->CreateCompositeKey(ck1), 0);
  EXPECT_EQ(storage->CreateCompositeKey(ck2), 0);
  EXPECT_EQ(storage->CreateCompositeKey(ck3), 0);

  std::string davis_prefix = EncodeCompositeKeyPrefix("byCity", {"Davis"});
  auto results = storage->GetByCompositeKeyPrefix(davis_prefix);

  EXPECT_EQ(results.size(), 3u);
  EXPECT_EQ(results[0], ck1);
  EXPECT_EQ(results[1], ck2);
  EXPECT_EQ(results[2], ck3);
}

TEST_P(KVStorageTest, PrefixScanOrdering) {
  // Inserted out of order; backend must return them sorted within a prefix.
  std::string sf_ck = EncodeCompositeKey("byCity", {"SF"}, "user:3");
  std::string davis_ck1 = EncodeCompositeKey("byCity", {"Davis"}, "user:1");
  std::string davis_ck2 = EncodeCompositeKey("byCity", {"Davis"}, "user:2");
  std::string nyc_ck = EncodeCompositeKey("byCity", {"NYC"}, "user:4");

  EXPECT_EQ(storage->CreateCompositeKey(sf_ck), 0);
  EXPECT_EQ(storage->CreateCompositeKey(davis_ck1), 0);
  EXPECT_EQ(storage->CreateCompositeKey(davis_ck2), 0);
  EXPECT_EQ(storage->CreateCompositeKey(nyc_ck), 0);

  std::string davis_prefix = EncodeCompositeKeyPrefix("byCity", {"Davis"});
  auto davis_results = storage->GetByCompositeKeyPrefix(davis_prefix);
  EXPECT_EQ(davis_results.size(), 2u);
  EXPECT_EQ(davis_results[0], davis_ck1);
  EXPECT_EQ(davis_results[1], davis_ck2);

  std::string sf_prefix = EncodeCompositeKeyPrefix("byCity", {"SF"});
  auto sf_results = storage->GetByCompositeKeyPrefix(sf_prefix);
  EXPECT_EQ(sf_results.size(), 1u);
  EXPECT_EQ(sf_results[0], sf_ck);
}

TEST_P(KVStorageTest, DeleteRemovesEntry) {
  std::string ck = EncodeCompositeKey("byCity", {"Davis"}, "user:1");
  std::string davis_prefix = EncodeCompositeKeyPrefix("byCity", {"Davis"});

  EXPECT_EQ(storage->CreateCompositeKey(ck), 0);
  EXPECT_EQ(storage->GetByCompositeKeyPrefix(davis_prefix).size(), 1u);

  EXPECT_EQ(storage->DeleteCompositeKey(ck), 0);
  EXPECT_EQ(storage->GetByCompositeKeyPrefix(davis_prefix).size(), 0u);
}

TEST_P(KVStorageTest, UpdateIsAtomic) {
  std::string old_ck = EncodeCompositeKey("byCity", {"Davis"}, "user:1");
  std::string new_ck = EncodeCompositeKey("byCity", {"SF"}, "user:1");
  std::string davis_prefix = EncodeCompositeKeyPrefix("byCity", {"Davis"});
  std::string sf_prefix = EncodeCompositeKeyPrefix("byCity", {"SF"});

  EXPECT_EQ(storage->CreateCompositeKey(old_ck), 0);
  EXPECT_EQ(storage->GetByCompositeKeyPrefix(davis_prefix).size(), 1u);

  EXPECT_EQ(storage->UpdateCompositeKey(old_ck, new_ck), 0);

  EXPECT_EQ(storage->GetByCompositeKeyPrefix(davis_prefix).size(), 0u);
  auto sf_results = storage->GetByCompositeKeyPrefix(sf_prefix);
  EXPECT_EQ(sf_results.size(), 1u);
  EXPECT_EQ(sf_results[0], new_ck);
}

TEST_P(KVStorageTest, EmptyPrefixScanReturnsNothing) {
  EXPECT_EQ(storage->CreateCompositeKey(
                EncodeCompositeKey("byCity", {"Davis"}, "user:1")),
            0);
  EXPECT_EQ(storage->CreateCompositeKey(
                EncodeCompositeKey("byCity", {"Davis"}, "user:2")),
            0);

  std::string nyc_prefix = EncodeCompositeKeyPrefix("byCity", {"NYC"});
  auto results = storage->GetByCompositeKeyPrefix(nyc_prefix);
  EXPECT_EQ(results.size(), 0u);
}

// GetAllItems must not return composite-key markers. Also checks that the
// filter is exact ("ck\0...") and doesn't drop user keys like "ck_balance".
TEST_P(KVStorageTest, GetAllItemsExcludesCompositeKeys) {
  EXPECT_EQ(storage->SetValueWithVersion("user:1", "alice_data", 0), 0);
  EXPECT_EQ(storage->SetValueWithVersion("user:2", "bob_data", 0), 0);
  EXPECT_EQ(storage->SetValueWithVersion("ck_balance", "real_user_data", 0), 0);

  EXPECT_EQ(storage->CreateCompositeKey(
                EncodeCompositeKey("byCity", {"Davis"}, "user:1")),
            0);
  EXPECT_EQ(storage->CreateCompositeKey(
                EncodeCompositeKey("byCity", {"SF"}, "user:2")),
            0);

  auto all = storage->GetAllItems();
  EXPECT_EQ(all.size(), 3u);
  EXPECT_TRUE(all.count("user:1"));
  EXPECT_TRUE(all.count("user:2"));
  EXPECT_TRUE(all.count("ck_balance"));

  // Markers are hidden, not deleted.
  auto davis = storage->GetByCompositeKeyPrefix(
      EncodeCompositeKeyPrefix("byCity", {"Davis"}));
  EXPECT_EQ(davis.size(), 1u);
}

INSTANTIATE_TEST_CASE_P(KVStorageTest, KVStorageTest,
                        ::testing::Values(MEM, LEVELDB,
                                          LEVELDB_WITH_BLOCK_CACHE));

}  // namespace
}  // namespace storage
}  // namespace resdb

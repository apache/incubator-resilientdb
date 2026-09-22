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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <tuple>
#include <vector>

#include "chain/storage/leveldb.h"
#include "chain/storage/memory_db.h"
#include "executor/kv/kv_executor.h"
#include "proto/kv/kv.pb.h"

namespace resdb {
namespace {

using ::testing::Combine;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Values;

enum Backend { MEMORY_DB, LEVEL_DB };
// ExecuteData and ExecuteRequest each dispatch commands; test both.
enum EntryPoint { EXECUTE_DATA, EXECUTE_REQUEST };

class KVExecutorIndexTest
    : public ::testing::TestWithParam<std::tuple<Backend, EntryPoint>> {
 protected:
  KVExecutorIndexTest() {
    std::unique_ptr<Storage> storage;
    if (std::get<0>(GetParam()) == LEVEL_DB) {
      db_path_ = ::testing::TempDir() + "kv_executor_index_test_db";
      std::filesystem::remove_all(db_path_);
      storage = storage::NewResLevelDB(db_path_);
    } else {
      storage = std::make_unique<storage::MemoryDB>();
    }
    executor_ = std::make_unique<KVExecutor>(std::move(storage));
  }

  ~KVExecutorIndexTest() override {
    executor_.reset();  // close LevelDB before deleting its folder
    if (!db_path_.empty()) {
      std::filesystem::remove_all(db_path_);
    }
  }

  KVResponse Execute(const KVRequest& request) {
    std::unique_ptr<std::string> raw;
    if (std::get<1>(GetParam()) == EXECUTE_DATA) {
      std::string data;
      request.SerializeToString(&data);
      raw = executor_->ExecuteData(data);
    } else {
      raw = executor_->ExecuteRequest(request);
    }
    KVResponse response;
    if (raw != nullptr) {
      response.ParseFromString(*raw);
    }
    return response;
  }

  void Set(const std::string& key, const std::string& value) {
    KVRequest request;
    request.set_cmd(KVRequest::SET);
    request.set_key(key);
    request.set_value(value);
    Execute(request);
  }

  void SetWithVersion(const std::string& key, const std::string& value) {
    KVRequest request;
    request.set_cmd(KVRequest::SET_WITH_VERSION);
    request.set_key(key);
    request.set_value(value);
    request.set_version(0);
    Execute(request);
  }

  std::string Get(const std::string& key) {
    KVRequest request;
    request.set_cmd(KVRequest::GET);
    request.set_key(key);
    return Execute(request).value();
  }

  bool IndexCommand(KVRequest::CMD cmd, const std::string& index,
                    const std::vector<std::string>& attributes,
                    const std::string& primary_key,
                    const std::vector<std::string>& new_attributes = {}) {
    KVRequest request;
    request.set_cmd(cmd);
    request.set_index_name(index);
    for (const auto& a : attributes) request.add_attributes(a);
    for (const auto& a : new_attributes) request.add_new_attributes(a);
    request.set_primary_key(primary_key);
    return Execute(request).success();
  }

  bool Create(const std::string& index, const std::vector<std::string>& attrs,
              const std::string& pk) {
    return IndexCommand(KVRequest::CREATE_INDEX_ENTRY, index, attrs, pk);
  }

  bool Delete(const std::string& index, const std::vector<std::string>& attrs,
              const std::string& pk) {
    return IndexCommand(KVRequest::DELETE_INDEX_ENTRY, index, attrs, pk);
  }

  bool Update(const std::string& index, const std::vector<std::string>& from,
              const std::vector<std::string>& to, const std::string& pk) {
    return IndexCommand(KVRequest::UPDATE_INDEX_ENTRY, index, from, pk, to);
  }

  std::vector<std::string> Query(const std::string& index,
                                 const std::vector<std::string>& attrs) {
    KVRequest request;
    request.set_cmd(KVRequest::QUERY_BY_INDEX);
    request.set_index_name(index);
    for (const auto& a : attrs) request.add_attributes(a);
    // Keep the response in a variable: looping over Execute(...).items()
    // directly would read from a temporary that's already been destroyed.
    KVResponse response = Execute(request);
    std::vector<std::string> keys;
    for (const Item& item : response.items().item()) {
      keys.push_back(item.key());
    }
    return keys;
  }

  std::string db_path_;
  std::unique_ptr<KVExecutor> executor_;
};

// A string holding a real zero byte (a plain "a\0b" literal stops at the \0).
std::string WithNul(const std::string& a, const std::string& b) {
  return a + std::string(1, '\0') + b;
}

TEST_P(KVExecutorIndexTest, CreateThenQuery) {
  Set("user:1", "Ana");
  Set("user:2", "Bo");
  Set("user:3", "Cy");
  EXPECT_TRUE(Create("by_city", {"Davis"}, "user:1"));
  EXPECT_TRUE(Create("by_city", {"Davis"}, "user:2"));
  EXPECT_TRUE(Create("by_city", {"Sacramento"}, "user:3"));

  EXPECT_THAT(Query("by_city", {"Davis"}), ElementsAre("user:1", "user:2"));
  EXPECT_THAT(Query("by_city", {"Sacramento"}), ElementsAre("user:3"));
  EXPECT_THAT(Query("by_city", {"Chico"}), IsEmpty());
}

TEST_P(KVExecutorIndexTest, CreateFailsWhenRecordDoesNotExist) {
  EXPECT_FALSE(Create("by_city", {"Davis"}, "user:99"));
  EXPECT_THAT(Query("by_city", {"Davis"}), IsEmpty());
}

// MemoryDB keeps SET_WITH_VERSION records in a separate map from SET records.
TEST_P(KVExecutorIndexTest, CreateWorksForRecordWrittenWithVersion) {
  SetWithVersion("user:5", "Eve");
  EXPECT_TRUE(Create("by_city", {"Davis"}, "user:5"));
  EXPECT_THAT(Query("by_city", {"Davis"}), ElementsAre("user:5"));
}

TEST_P(KVExecutorIndexTest, CreateRejectsBadInput) {
  Set("user:1", "Ana");
  EXPECT_FALSE(Create("", {"Davis"}, "user:1"));
  EXPECT_FALSE(Create("by_city", {"Davis"}, ""));
  EXPECT_FALSE(Create("by_city", {WithNul("Da", "vis")}, "user:1"));
  EXPECT_FALSE(Create(WithNul("by", "city"), {"Davis"}, "user:1"));
  EXPECT_THAT(Query("by_city", {"Davis"}), IsEmpty());
}

TEST_P(KVExecutorIndexTest, CreatingTwiceKeepsOneEntry) {
  Set("user:1", "Ana");
  EXPECT_TRUE(Create("by_city", {"Davis"}, "user:1"));
  EXPECT_TRUE(Create("by_city", {"Davis"}, "user:1"));
  EXPECT_THAT(Query("by_city", {"Davis"}), ElementsAre("user:1"));
}

// "Davis" must not match "Davisville".
TEST_P(KVExecutorIndexTest, QueryMatchesWholeAttribute) {
  Set("user:1", "Ana");
  Set("user:2", "Bo");
  EXPECT_TRUE(Create("by_city", {"Davis"}, "user:1"));
  EXPECT_TRUE(Create("by_city", {"Davisville"}, "user:2"));
  EXPECT_THAT(Query("by_city", {"Davis"}), ElementsAre("user:1"));
}

// Same attribute, different index: queries must not mix them.
TEST_P(KVExecutorIndexTest, QueryOnlyReadsItsOwnIndex) {
  Set("user:1", "Ana");
  EXPECT_TRUE(Create("by_city", {"Davis"}, "user:1"));
  EXPECT_THAT(Query("by_birthplace", {"Davis"}), IsEmpty());
  EXPECT_THAT(Query("by_cit", {"Davis"}), IsEmpty());
}

TEST_P(KVExecutorIndexTest, QueryWithLeadingAttributes) {
  Set("user:1", "Ana");
  Set("user:2", "Bo");
  EXPECT_TRUE(Create("by_city_status", {"Davis", "active"}, "user:1"));
  EXPECT_TRUE(Create("by_city_status", {"Davis", "inactive"}, "user:2"));
  EXPECT_THAT(Query("by_city_status", {"Davis"}),
              ElementsAre("user:1", "user:2"));
  EXPECT_THAT(Query("by_city_status", {"Davis", "active"}),
              ElementsAre("user:1"));
}

// No attributes = every entry in the index.
TEST_P(KVExecutorIndexTest, QueryWithNoAttributesReturnsWholeIndex) {
  Set("user:1", "Ana");
  Set("user:2", "Bo");
  EXPECT_TRUE(Create("by_city", {"Davis"}, "user:1"));
  EXPECT_TRUE(Create("by_city", {"Sacramento"}, "user:2"));
  EXPECT_THAT(Query("by_city", {}), ElementsAre("user:1", "user:2"));
}

TEST_P(KVExecutorIndexTest, QueryRejectsBadInput) {
  Set("user:1", "Ana");
  EXPECT_TRUE(Create("by_city", {"Davis"}, "user:1"));
  EXPECT_THAT(Query("", {"Davis"}), IsEmpty());
  EXPECT_THAT(Query("by_city", {WithNul("Da", "vis")}), IsEmpty());
}

TEST_P(KVExecutorIndexTest, DeleteRemovesOnlyThatEntry) {
  Set("user:1", "Ana");
  Set("user:10", "Jo");
  EXPECT_TRUE(Create("by_city", {"Davis"}, "user:1"));
  EXPECT_TRUE(Create("by_city", {"Davis"}, "user:10"));

  EXPECT_TRUE(Delete("by_city", {"Davis"}, "user:1"));
  EXPECT_THAT(Query("by_city", {"Davis"}), ElementsAre("user:10"));
  // user:1's entry is gone; "user:10" must not count as a match for it.
  EXPECT_FALSE(Delete("by_city", {"Davis"}, "user:1"));
}

TEST_P(KVExecutorIndexTest, DeleteFailsWhenEntryDoesNotExist) {
  Set("user:1", "Ana");
  EXPECT_TRUE(Create("by_city", {"Davis"}, "user:1"));
  EXPECT_FALSE(Delete("by_city", {"Sacramento"}, "user:1"));
  EXPECT_THAT(Query("by_city", {"Davis"}), ElementsAre("user:1"));
}

TEST_P(KVExecutorIndexTest, UpdateMovesEntry) {
  Set("user:1", "Ana");
  Set("user:2", "Bo");
  EXPECT_TRUE(Create("by_city", {"Davis"}, "user:1"));
  EXPECT_TRUE(Create("by_city", {"Davis"}, "user:2"));

  EXPECT_TRUE(Update("by_city", {"Davis"}, {"Sacramento"}, "user:2"));
  EXPECT_THAT(Query("by_city", {"Davis"}), ElementsAre("user:1"));
  EXPECT_THAT(Query("by_city", {"Sacramento"}), ElementsAre("user:2"));
}

// A wrong "from" must change nothing, and must not create the new entry.
TEST_P(KVExecutorIndexTest, UpdateFailsWhenEntryDoesNotExist) {
  Set("user:1", "Ana");
  EXPECT_TRUE(Create("by_city", {"Davis"}, "user:1"));
  EXPECT_FALSE(Update("by_city", {"Chico"}, {"Sacramento"}, "user:1"));
  EXPECT_THAT(Query("by_city", {"Davis"}), ElementsAre("user:1"));
  EXPECT_THAT(Query("by_city", {"Sacramento"}), IsEmpty());
}

TEST_P(KVExecutorIndexTest, UpdateRejectsBadInput) {
  Set("user:1", "Ana");
  EXPECT_TRUE(Create("by_city", {"Davis"}, "user:1"));
  EXPECT_FALSE(Update("by_city", {"Davis"}, {WithNul("Sac", "ramento")},
                      "user:1"));
  EXPECT_THAT(Query("by_city", {"Davis"}), ElementsAre("user:1"));
}

// SET and SET_WITH_VERSION must not be able to plant a fake index entry.
TEST_P(KVExecutorIndexTest, SetCannotWriteIndexEntries) {
  const std::string forged = std::string("ck") + '\0' + "by_city" + '\0' +
                             "Davis" + '\0' + "user:666";
  Set(forged, "x");
  SetWithVersion(forged, "x");
  EXPECT_THAT(Query("by_city", {"Davis"}), IsEmpty());
  EXPECT_EQ(Get(forged), "");
}

// Keys that merely start with "ck" (no \0) are ordinary user keys.
TEST_P(KVExecutorIndexTest, SetStillAllowsKeysStartingWithCk) {
  Set("ck_balance", "100");
  EXPECT_EQ(Get("ck_balance"), "100");
}

// user:1 has two matching entries; it must come back once.
TEST_P(KVExecutorIndexTest, QueryReturnsEachRecordOnce) {
  Set("user:1", "Ana");
  EXPECT_TRUE(Create("by_city_status", {"Davis", "active"}, "user:1"));
  EXPECT_TRUE(Create("by_city_status", {"Davis", "inactive"}, "user:1"));
  EXPECT_THAT(Query("by_city_status", {"Davis"}), ElementsAre("user:1"));
}

// Stored order is by attributes ("active" < "inactive"), so user:2 comes
// first internally; results must still be sorted by primary key.
TEST_P(KVExecutorIndexTest, QueryResultsAreSortedByPrimaryKey) {
  Set("user:1", "Ana");
  Set("user:2", "Bo");
  EXPECT_TRUE(Create("by_city_status", {"Davis", "inactive"}, "user:1"));
  EXPECT_TRUE(Create("by_city_status", {"Davis", "active"}, "user:2"));
  EXPECT_THAT(Query("by_city_status", {"Davis"}),
              ElementsAre("user:1", "user:2"));
}

TEST_P(KVExecutorIndexTest, UpdateToSameAttributesKeepsEntry) {
  Set("user:1", "Ana");
  EXPECT_TRUE(Create("by_city", {"Davis"}, "user:1"));
  EXPECT_TRUE(Update("by_city", {"Davis"}, {"Davis"}, "user:1"));
  EXPECT_THAT(Query("by_city", {"Davis"}), ElementsAre("user:1"));
}

TEST_P(KVExecutorIndexTest, UpdateOntoExistingEntry) {
  Set("user:1", "Ana");
  EXPECT_TRUE(Create("by_city", {"Davis"}, "user:1"));
  EXPECT_TRUE(Create("by_city", {"Sacramento"}, "user:1"));
  EXPECT_TRUE(Update("by_city", {"Davis"}, {"Sacramento"}, "user:1"));
  EXPECT_THAT(Query("by_city", {"Davis"}), IsEmpty());
  EXPECT_THAT(Query("by_city", {"Sacramento"}), ElementsAre("user:1"));
}

INSTANTIATE_TEST_SUITE_P(AllBackendsAndEntryPoints, KVExecutorIndexTest,
                         Combine(Values(MEMORY_DB, LEVEL_DB),
                                 Values(EXECUTE_DATA, EXECUTE_REQUEST)));

}  // namespace
}  // namespace resdb

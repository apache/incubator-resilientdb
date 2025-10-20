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



#include <filesystem>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "chain/storage/leveldb.h"
#include "chain/storage/rocksdb.h"
#include "chain/storage/memory_db.h"
#include "chain/storage/storage.h"
#include "common/test/test_macros.h"

#include "proto/kv/kv.pb.h"
#include "executor/kv/kv_executor.h"

namespace resdb {
namespace {

using ::resdb::testing::EqualsProto;
using ::testing::TestWithParam;

enum StorageType { MEM = 0, LEVELDB = 1, LEVELDB_WITH_BLOCK_CACHE = 2, ROCKSDB = 3 };

class KVExecutorTest : public TestWithParam<StorageType> {
 public:
  KVExecutorTest() {
    Reset();
    StorageType t = GetParam();
    storage::LevelDBInfo config;
    std::unique_ptr<Storage> storage;

    switch (t) {
      case MEM:
        storage = storage::NewMemoryDB();
        break;
      case LEVELDB:
        storage = storage::NewResLevelDB(std::nullopt);
        break;
      case LEVELDB_WITH_BLOCK_CACHE:
        config.set_enable_block_cache(true);
        storage = storage::NewResLevelDB(config);
        break;
      case ROCKSDB:
        storage = storage::NewResRocksDB(std::nullopt);
        break;
    }
    storage_ptr_ = storage.get();
    impl_ = std::make_unique<KVExecutor>(std::move(storage));
  }

  ~KVExecutorTest() {
    Reset(); 
  }

 protected:
  void Reset() { 
    impl_.reset();  // Release the executor first
    storage_ptr_ = nullptr;
    std::filesystem::remove_all("/tmp/nexres-leveldb");
    std::filesystem::remove_all("/tmp/nexres-rocksdb");
  }

  int Set(const std::string& key, const std::string& value) {
    KVRequest request;
    request.set_cmd(KVRequest::SET);
    request.set_key(key);
    request.set_value(value);

    std::string str;
    if (!request.SerializeToString(&str)) {
      return -1;
    }

    impl_->ExecuteData(str);
    return 0;
  }

  std::string Get(const std::string& key) {
    KVRequest request;
    request.set_cmd(KVRequest::GET);
    request.set_key(key);

    std::string str;
    if (!request.SerializeToString(&str)) {
      return "";
    }
    auto resp = impl_->ExecuteData(str);
    if (resp == nullptr) {
      return "";
    }
    KVResponse kv_response;
    if (!kv_response.ParseFromString(*resp)) {
      return "";
    }
    return kv_response.value();
  }

  std::string GetAllValues() {
    KVRequest request;
    request.set_cmd(KVRequest::GETALLVALUES);

    std::string str;
    if (!request.SerializeToString(&str)) {
      return "";
    }
    auto resp = impl_->ExecuteData(str);
    if (resp == nullptr) {
      return "";
    }
    KVResponse kv_response;
    if (!kv_response.ParseFromString(*resp)) {
      return "";
    }
    return kv_response.value();
  }

  std::string GetRange(const std::string& min_key, const std::string& max_key) {
    KVRequest request;
    request.set_cmd(KVRequest::GETRANGE);
    request.set_key(min_key);
    request.set_value(max_key);

    std::string str;
    if (!request.SerializeToString(&str)) {
      return "";
    }
    auto resp = impl_->ExecuteData(str);
    if (resp == nullptr) {
      return "";
    }
    KVResponse kv_response;
    if (!kv_response.ParseFromString(*resp)) {
      return "";
    }
    return kv_response.value();
  }

  int Set(const std::string& key, const std::string& value, int version) {
    KVRequest request;
    request.set_cmd(KVRequest::SET_WITH_VERSION);
    request.set_key(key);
    request.set_value(value);
    request.set_version(version);

    std::string str;
    if (!request.SerializeToString(&str)) {
      return -1;
    }

    impl_->ExecuteData(str);
    return 0;
  }

  ValueInfo Get(const std::string& key, int version) {
    KVRequest request;
    request.set_cmd(KVRequest::GET_WITH_VERSION);
    request.set_key(key);
    request.set_version(version);

    std::string str;
    if (!request.SerializeToString(&str)) {
      return ValueInfo();
    }

    auto resp = impl_->ExecuteData(str);
    if (resp == nullptr) {
      return ValueInfo();
    }
    KVResponse kv_response;
    if (!kv_response.ParseFromString(*resp)) {
      return ValueInfo();
    }

    return kv_response.value_info();
  }

  Items GetAllItems() {
    KVRequest request;
    request.set_cmd(KVRequest::GET_ALL_ITEMS);

    std::string str;
    if (!request.SerializeToString(&str)) {
      return Items();
    }

    auto resp = impl_->ExecuteData(str);
    if (resp == nullptr) {
      return Items();
    }
    KVResponse kv_response;
    if (!kv_response.ParseFromString(*resp)) {
      return Items();
    }

    return kv_response.items();
  }

  Items GetKeyRange(const std::string& min_key, const std::string& max_key) {
    KVRequest request;
    request.set_cmd(KVRequest::GET_KEY_RANGE);
    request.set_min_key(min_key);
    request.set_max_key(max_key);

    std::string str;
    if (!request.SerializeToString(&str)) {
      return Items();
    }

    auto resp = impl_->ExecuteData(str);
    if (resp == nullptr) {
      return Items();
    }
    KVResponse kv_response;
    if (!kv_response.ParseFromString(*resp)) {
      return Items();
    }

    return kv_response.items();
  }

  Items GetHistory(const std::string& key, int min_version, int max_version) {
    KVRequest request;
    request.set_cmd(KVRequest::GET_HISTORY);
    request.set_key(key);
    request.set_min_version(min_version);
    request.set_max_version(max_version);

    std::string str;
    if (!request.SerializeToString(&str)) {
      return Items();
    }

    auto resp = impl_->ExecuteData(str);
    if (resp == nullptr) {
      return Items();
    }
    KVResponse kv_response;
    if (!kv_response.ParseFromString(*resp)) {
      return Items();
    }

    return kv_response.items();
  }

  int CreateCompositeKey(const std::string& primary_key,
                                 const std::string& field_name,
                                 const std::string& field_value,
                                 int field_type) {
    KVRequest request;
    request.set_cmd(KVRequest::CREATE_COMPOSITE_KEY);
    request.set_key(primary_key);
    request.set_value(field_value);
    request.set_field_name(field_name);
    request.set_field_type(field_type);

    std::string str;
    if (!request.SerializeToString(&str)) {
      return -1;
    }

    auto resp = impl_->ExecuteData(str);
    if (resp == nullptr) {
      return -1;
    }

    KVResponse kv_response;
    if (!kv_response.ParseFromString(*resp)) {
      return -1;
    }

    int status = std::stoi(kv_response.value());

    return status;
  }

  int UpdateCompositeKey(const std::string& primary_key, const std::string& field_name, const std::string& old_field_value, const std::string& new_field_value, 
        int old_field_type, int new_field_type) {
    
    KVRequest request;
    request.set_cmd(KVRequest::UPDATE_COMPOSITE_KEY);
    request.set_key(primary_key);
    request.set_field_name(field_name);
    request.set_min_value(old_field_value);
    request.set_max_value(new_field_value);
    request.set_field_type(old_field_type); //old field type and new field type should be the same

    std::string str;
    if (!request.SerializeToString(&str)) {
      return -1;
    }

    auto resp = impl_->ExecuteData(str);
    if (resp == nullptr) {
      return -1;
    }

    KVResponse kv_response;
    if (!kv_response.ParseFromString(*resp)) {
      return -1;
    }

    int status = std::stoi(kv_response.value());

    return status;
  }

  int DelVal(const std::string& key) {
    KVRequest request;
    request.set_cmd(KVRequest::DEL_VAL);
    request.set_key(key);
    
    std::string str;
    if (!request.SerializeToString(&str)) {
      return -1;
    }

    auto resp = impl_->ExecuteData(str);
    if (resp == nullptr) {
      return -1;
    }

    KVResponse kv_response;
    if (!kv_response.ParseFromString(*resp)) {
      return -1;
    }

    int status = std::stoi(kv_response.value());

    return status;
  }

  Items GetByCompositeKey(const std::string& field_name,
                              const std::string& field_value, int field_type) {
    KVRequest request;
    request.set_cmd(KVRequest::GET_BY_COMPOSITE_KEY);
    request.set_value(field_value);
    request.set_field_name(field_name);
    request.set_field_type(field_type);

    std::string str;
    if (!request.SerializeToString(&str)) {
      return Items();
    }

    auto resp = impl_->ExecuteData(str);
    if (resp == nullptr) {
      return Items();
    }
    KVResponse kv_response;
    if (!kv_response.ParseFromString(*resp)) {
      return Items();
    }

    return kv_response.items();
  }

  Items GetByCompositeKeyRange(const std::string& field_name,
                               const std::string& min_value,
                               const std::string& max_value, int field_type) {
    KVRequest request;
    request.set_cmd(KVRequest::GET_COMPOSITE_KEY_RANGE);
    request.set_field_name(field_name);
    request.set_min_value(min_value);
    request.set_max_value(max_value);
    request.set_field_type(field_type);

    std::string str;
    if (!request.SerializeToString(&str)) {
      return Items();
    }

    auto resp = impl_->ExecuteData(str);
    if (resp == nullptr) {
      return Items();
    }
    KVResponse kv_response;
    if (!kv_response.ParseFromString(*resp)) {
      return Items();
    }

    return kv_response.items();
  }

  void PrintAllKeys() {
    std::cout << "=== All Keys in Database ===" << std::endl;
    Items all_items = GetAllItems();
    for (int i = 0; i < all_items.item_size(); i++) {
      std::cout << "Key: '" << all_items.item(i).key() << "' -> Value: '"
                << all_items.item(i).value_info().value() << "'" << std::endl;
    }
    std::cout << "=== End All Keys ===" << std::endl;
  }

 protected:
  Storage* storage_ptr_;

 private:
  std::unique_ptr<KVExecutor> impl_;
};

TEST_P(KVExecutorTest, SetValue) {
  std::map<std::string, std::string> data;

  EXPECT_EQ(GetAllValues(), "[]");
  EXPECT_EQ(Set("test_key", "test_value"), 0);
  EXPECT_EQ(Get("test_key"), "test_value");

  // GetAllValues and GetRange may be out of order for in-memory, so we test up
  // to 1 key-value pair
  EXPECT_EQ(GetAllValues(), "[test_value]");
  EXPECT_EQ(GetRange("a", "z"), "[test_value]");
}

TEST_P(KVExecutorTest, SetValueWithVersion) {
  std::map<std::string, std::string> data;

  {
    EXPECT_EQ(Set("test_key", "test_value", 0), 0);
    ValueInfo expected_info;
    expected_info.set_value("test_value");
    expected_info.set_version(1);
    EXPECT_THAT(Get("test_key", 1), EqualsProto(expected_info));
  }

  {
    EXPECT_EQ(Set("test_key", "test_value1", 1), 0);
    ValueInfo expected_info;
    expected_info.set_value("test_value1");
    expected_info.set_version(2);
    EXPECT_THAT(Get("test_key", 2), EqualsProto(expected_info));
  }
  {
    EXPECT_EQ(Set("test_key", "test_value1", 1), 0);
    ValueInfo expected_info;
    expected_info.set_value("test_value");
    expected_info.set_version(1);
    EXPECT_THAT(Get("test_key", 1), EqualsProto(expected_info));
  }

  {
    EXPECT_EQ(Set("test_key1", "test_key1", 0), 0);
    ValueInfo expected_info;
    expected_info.set_value("test_key1");
    expected_info.set_version(1);
    EXPECT_THAT(Get("test_key1", 1), EqualsProto(expected_info));

    ValueInfo expected_info2;
    expected_info2.set_value("test_value");
    expected_info2.set_version(1);
    EXPECT_THAT(Get("test_key", 1), EqualsProto(expected_info2));

    ValueInfo expected_info3;
    expected_info3.set_value("test_value1");
    expected_info3.set_version(2);
    EXPECT_THAT(Get("test_key", 0), EqualsProto(expected_info3));
  }

  {
    Items items;
    {
      Item* item = items.add_item();
      item->set_key("test_key");
      item->mutable_value_info()->set_value("test_value1");
      item->mutable_value_info()->set_version(2);
    }
    {
      Item* item = items.add_item();
      item->set_key("test_key1");
      item->mutable_value_info()->set_value("test_key1");
      item->mutable_value_info()->set_version(1);
    }

    EXPECT_THAT(GetAllItems(), EqualsProto(items));
  }

  {
    Items items;
    {
      Item* item = items.add_item();
      item->set_key("test_key");
      item->mutable_value_info()->set_value("test_value1");
      item->mutable_value_info()->set_version(2);
    }
    EXPECT_THAT(GetKeyRange("test_key", "test_key"), EqualsProto(items));
  }

  {
    Items items;
    {
      Item* item = items.add_item();
      item->set_key("test_key");
      item->mutable_value_info()->set_value("test_value1");
      item->mutable_value_info()->set_version(2);
    }
    {
      Item* item = items.add_item();
      item->set_key("test_key");
      item->mutable_value_info()->set_value("test_value");
      item->mutable_value_info()->set_version(1);
    }
    EXPECT_THAT(GetHistory("test_key", 0, 2), EqualsProto(items));
  }
}

TEST_P(KVExecutorTest, CompositeKeyStringField) {
  std::string user1 = "{\"name\":\"John\",\"age\":30,\"city\":\"NYC\"}";
  std::string user2 = "{\"name\":\"Jane\",\"age\":25,\"city\":\"LA\"}";
  std::string user3 = "{\"name\":\"Bob\",\"age\":35,\"city\":\"Chicago\"}";


  EXPECT_EQ(Set("user_1", user1), 0);
  EXPECT_EQ(Set("user_2", user2), 0);
  EXPECT_EQ(Set("user_3", user3), 0);


  int status1 = CreateCompositeKey("user_1", "name", "John", 0);  // STRING = 0
  int status2 = CreateCompositeKey("user_2", "name", "Jane", 0);
  int status3 = CreateCompositeKey("user_3", "name", "Bob", 0);


  EXPECT_EQ(status1, 0);
  EXPECT_EQ(status2, 0);
  EXPECT_EQ(status3, 0);


  Items results = GetByCompositeKey("name", "John", 0);
  EXPECT_EQ(results.item_size(), 1);
  EXPECT_EQ(results.item(0).value_info().value(), user1);

  Items empty_results = GetByCompositeKey("name", "Alice", 0);
  EXPECT_EQ(empty_results.item_size(), 0);
}

TEST_P(KVExecutorTest, CompositeKeyIntegerField) {
  std::string user1 = "{\"name\":\"John\",\"age\":30,\"city\":\"NYC\"}";
  std::string user2 = "{\"name\":\"Jane\",\"age\":25,\"city\":\"LA\"}";
  std::string user3 = "{\"name\":\"Bob\",\"age\":35,\"city\":\"Chicago\"}";
  std::string user4 = "{\"name\":\"Alice\",\"age\":30,\"city\":\"Boston\"}";

  // Store documents
  EXPECT_EQ(Set("user_1", user1), 0);
  EXPECT_EQ(Set("user_2", user2), 0);
  EXPECT_EQ(Set("user_3", user3), 0);
  EXPECT_EQ(Set("user_4", user4), 0);

  int status1 = CreateCompositeKey("user_1", "age", "30", 1);
  int status2 = CreateCompositeKey("user_2", "age", "25", 1);
  int status3 = CreateCompositeKey("user_3", "age", "35", 1);
  int status4 = CreateCompositeKey("user_4", "age", "30", 1);

  EXPECT_EQ(status1, 0);
  EXPECT_EQ(status2, 0);
  EXPECT_EQ(status3, 0);
  EXPECT_EQ(status4, 0);

  Items results = GetByCompositeKey("age", "30", 1);
  EXPECT_EQ(results.item_size(), 2);

  bool found_user1 = false, found_user4 = false;
  for (int i = 0; i < results.item_size(); i++) {
    std::string doc = results.item(i).value_info().value();
    if (doc == user1) found_user1 = true;
    if (doc == user4) found_user4 = true;
  }
  EXPECT_TRUE(found_user1);
  EXPECT_TRUE(found_user4);
}


TEST_P(KVExecutorTest, DeleteValue) {
  std::string user1 = "{\"name\":\"John\",\"age\":30,\"city\":\"NYC\"}";
  EXPECT_EQ(Set("user_1", user1), 0);
  EXPECT_EQ(Get("user_1"), user1);

  int status1 = DelVal("user_1");
  EXPECT_EQ(status1, 0);
  EXPECT_EQ(Get("user_1"), "");
}

TEST_P(KVExecutorTest, UpdateCompositeKey){
  std::string user1 = "{\"name\":\"John\",\"age\":30,\"city\":\"NYC\"}";
  std::string user2 = "{\"name\":\"amy\",\"age\":30,\"city\":\"LA\"}";

  EXPECT_EQ(Set("user_1", user1), 0);
  EXPECT_EQ(Set("user_2", user2), 0);

  int status1 = CreateCompositeKey("user_1", "age", "30", 1);
  int status2 = CreateCompositeKey("user_2", "age", "30", 1);
  
  EXPECT_EQ(status1, 0);
  EXPECT_EQ(status2, 0);

  std::string user_update = "{\"name\":\"John\",\"age\":31,\"city\":\"NYC\"}";
  EXPECT_EQ(Set("user_1", user_update), 0);
  int status5 = UpdateCompositeKey("user_1", "age", "30", "31", 1, 1);
  EXPECT_EQ(status5, 0);

  Items results = GetByCompositeKey("age", "31", 1);
  EXPECT_EQ(results.item_size(), 1);
  EXPECT_EQ(results.item(0).value_info().value(), user_update);

  Items empty_results = GetByCompositeKey("age", "30", 1);
  EXPECT_EQ(empty_results.item_size(), 1);
  EXPECT_EQ(empty_results.item(0).value_info().value(), user2);

}


TEST_P(KVExecutorTest, CompositeKeyBooleanField) {
  std::string user1 = "{\"name\":\"John\",\"active\":true,\"city\":\"NYC\"}";
  std::string user2 = "{\"name\":\"Jane\",\"active\":false,\"city\":\"LA\"}";
  std::string user3 = "{\"name\":\"Bob\",\"active\":true,\"city\":\"Chicago\"}";

  EXPECT_EQ(Set("user_1", user1), 0);
  EXPECT_EQ(Set("user_2", user2), 0);
  EXPECT_EQ(Set("user_3", user3), 0);

  int status1 = CreateCompositeKey("user_1", "active", "true", 2);
  int status2 = CreateCompositeKey("user_2", "active", "false", 2);
  int status3 = CreateCompositeKey("user_3", "active", "true", 2);

  EXPECT_EQ(status1, 0);
  EXPECT_EQ(status2, 0);
  EXPECT_EQ(status3, 0);

  Items active_results = GetByCompositeKey("active", "true", 2);
  EXPECT_EQ(active_results.item_size(), 2);

  Items inactive_results = GetByCompositeKey("active", "false", 2);
  EXPECT_EQ(inactive_results.item_size(), 1);
  EXPECT_EQ(inactive_results.item(0).value_info().value(), user2);
}

TEST_P(KVExecutorTest, CompositeKeyTimestampField) {
  std::string user1 =
      "{\"name\":\"John\",\"created_at\":1640995200,\"city\":\"NYC\"}";
  std::string user2 =
      "{\"name\":\"Jane\",\"created_at\":1641081600,\"city\":\"LA\"}";
  std::string user3 =
      "{\"name\":\"Bob\",\"created_at\":1641168000,\"city\":\"Chicago\"}";


  EXPECT_EQ(Set("user_1", user1), 0);
  EXPECT_EQ(Set("user_2", user2), 0);
  EXPECT_EQ(Set("user_3", user3), 0);

  int status1 = CreateCompositeKey("user_1", "created_at", "1640995200", 3);
  int status2 = CreateCompositeKey("user_2", "created_at", "1641081600", 3);
  int status3 = CreateCompositeKey("user_3", "created_at", "1641168000", 3);

  EXPECT_EQ(status1, 0);
  EXPECT_EQ(status2, 0);
  EXPECT_EQ(status3, 0);

  Items results = GetByCompositeKey("created_at", "1640995200", 3);
  EXPECT_EQ(results.item_size(), 1);
  EXPECT_EQ(results.item(0).value_info().value(), user1);
}

TEST_P(KVExecutorTest, CompositeKeyRangeQuery) {
  std::string user1 = "{\"name\":\"John\",\"age\":25,\"city\":\"NYC\"}";
  std::string user2 = "{\"name\":\"Jane\",\"age\":30,\"city\":\"LA\"}";
  std::string user3 = "{\"name\":\"Bob\",\"age\":35,\"city\":\"Chicago\"}";
  std::string user4 = "{\"name\":\"Alice\",\"age\":40,\"city\":\"Boston\"}";

  // Store documents
  EXPECT_EQ(Set("user_1", user1), 0);
  EXPECT_EQ(Set("user_2", user2), 0);
  EXPECT_EQ(Set("user_3", user3), 0);
  EXPECT_EQ(Set("user_4", user4), 0);

  // Create composite keys on 'age' field
  int status1 = CreateCompositeKey("user_1", "age", "25", 1);
  int status2 = CreateCompositeKey("user_2", "age", "30", 1);
  int status3 = CreateCompositeKey("user_3", "age", "35", 1);
  int status4 = CreateCompositeKey("user_4", "age", "40", 1);

  // Verify composite keys were created successfully
  EXPECT_EQ(status1, 0);
  EXPECT_EQ(status2, 0);
  EXPECT_EQ(status3, 0);
  EXPECT_EQ(status4, 0);

  // Test range query (age between 30 and 35 inclusive)
  Items range_results = GetByCompositeKeyRange("age", "30", "35", 1);
  EXPECT_EQ(range_results.item_size(), 2);  // Should return user_2 and user_3
}

TEST_P(KVExecutorTest, CompositeKeyMultipleFields) {
  // Create JSON documents with multiple fields
  std::string user1 =
      "{\"name\":\"John\",\"age\":30,\"city\":\"NYC\",\"active\":true}";
  std::string user2 =
      "{\"name\":\"Jane\",\"age\":25,\"city\":\"LA\",\"active\":false}";
  std::string user3 =
      "{\"name\":\"Bob\",\"age\":35,\"city\":\"Chicago\",\"active\":true}";

  // Store documents
  EXPECT_EQ(Set("user_1", user1), 0);
  EXPECT_EQ(Set("user_2", user2), 0);
  EXPECT_EQ(Set("user_3", user3), 0);

  // Create composite keys on multiple fields
  int user1_name_status = CreateCompositeKey("user_1", "name", "John", 0);
  int user1_age_status = CreateCompositeKey("user_1", "age", "30", 1);
  int user1_city_status = CreateCompositeKey("user_1", "city", "NYC", 0);
  int user1_active_status = CreateCompositeKey("user_1", "active", "true", 2);

  int user2_name_status = CreateCompositeKey("user_2", "name", "Jane", 0);
  int user2_age_status = CreateCompositeKey("user_2", "age", "25", 1);
  int user2_city_status = CreateCompositeKey("user_2", "city", "LA", 0);
  int user2_active_status = CreateCompositeKey("user_2", "active", "false", 2);

  int user3_name_status = CreateCompositeKey("user_3", "name", "Bob", 0);
  int user3_age_status = CreateCompositeKey("user_3", "age", "35", 1);
  int user3_city_status = CreateCompositeKey("user_3", "city", "Chicago", 0);
  int user3_active_status = CreateCompositeKey("user_3", "active", "true", 2);

  // Verify composite keys were created successfully
  EXPECT_EQ(user1_name_status, 0);
  EXPECT_EQ(user1_age_status, 0);
  EXPECT_EQ(user1_city_status, 0);
  EXPECT_EQ(user1_active_status, 0);
  EXPECT_EQ(user2_name_status, 0);
  EXPECT_EQ(user2_age_status, 0);
  EXPECT_EQ(user2_city_status, 0);
  EXPECT_EQ(user2_active_status, 0);
  EXPECT_EQ(user3_name_status, 0);
  EXPECT_EQ(user3_age_status, 0);
  EXPECT_EQ(user3_city_status, 0);
  EXPECT_EQ(user3_active_status, 0);

  // Test queries on different fields
  Items name_results = GetByCompositeKey("name", "John", 0);
  EXPECT_EQ(name_results.item_size(), 1);
  EXPECT_EQ(name_results.item(0).value_info().value(), user1);

  Items age_results = GetByCompositeKey("age", "25", 1);
  EXPECT_EQ(age_results.item_size(), 1);
  EXPECT_EQ(age_results.item(0).value_info().value(), user2);

  Items city_results = GetByCompositeKey("city", "Chicago", 0);
  EXPECT_EQ(city_results.item_size(), 1);
  EXPECT_EQ(city_results.item(0).value_info().value(), user3);

  Items active_results = GetByCompositeKey("active", "true", 2);
  EXPECT_EQ(active_results.item_size(), 2);  // John and Bob are active
}

INSTANTIATE_TEST_CASE_P(KVExecutorTest, KVExecutorTest,
                        ::testing::Values(LEVELDB, ROCKSDB));

}  // namespace

}  // namespace resdb

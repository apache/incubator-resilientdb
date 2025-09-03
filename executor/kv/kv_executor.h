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

#pragma once

#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

#include "chain/storage/storage.h"
#include "executor/common/transaction_manager.h"
#include "proto/kv/kv.pb.h"

namespace resdb {

enum class CompositeKeyType {
  STRING = 0,
  INTEGER = 1,  // int32_t for regular integers
  BOOLEAN = 2,
  TIMESTAMP = 3  // int64_t for Unix timestamps
};

class KVExecutor : public TransactionManager {
 public:
  KVExecutor(std::unique_ptr<Storage> storage);
  virtual ~KVExecutor() = default;

  std::unique_ptr<std::string> ExecuteData(const std::string& request) override;

  std::unique_ptr<google::protobuf::Message> ParseData(
      const std::string& request) override;
  std::unique_ptr<std::string> ExecuteRequest(
      const google::protobuf::Message& kv_request) override;

 protected:
  virtual void Set(const std::string& key, const std::string& value);
  std::string Get(const std::string& key);
  std::string GetAllValues();
  std::string GetRange(const std::string& min_key, const std::string& max_key);

  void SetWithVersion(const std::string& key, const std::string& value,
                      int version);
  void GetWithVersion(const std::string& key, int version, ValueInfo* info);
  void GetAllItems(Items* items);
  void GetKeyRange(const std::string& min_key, const std::string& max_key,
                   Items* items);
  void GetHistory(const std::string& key, int min_key, int max_key,
                  Items* items);
  void GetTopHistory(const std::string& key, int top_number, Items* items);

  // Composite key methods
  int CreateCompositeKey(const std::string& primary_key,
                           const std::string& field_name,
                           const std::string& field_value,
                           CompositeKeyType field_type);
  
  std::vector<std::string> GetByCompositeKey(const std::string& field_name,
                                             const std::string& field_value,
                                             CompositeKeyType field_type);
  
  std::vector<std::string> GetByCompositeKeyRange(const std::string& field_name,
                                                const std::string& min_value,
                                                const std::string& max_value,
                                                CompositeKeyType field_type);
  int UpdateCompositeKey(const std::string& primary_key, const std::string& field_name, const std::string& old_field_value, const std::string& new_field_value, 
                          CompositeKeyType old_field_type, CompositeKeyType new_field_type);

 private:
  // Simple encoding functions
  std::string EncodeValue(const std::string& value, CompositeKeyType field_type);
  std::string EncodeInteger(int32_t value);
  std::string EncodeBoolean(bool value);
  std::string EncodeTimestamp(int64_t value);
  
  // Helper functions
  std::string BuildCompositeKey(const std::string& field_name, 
                                const std::string& encoded_value,
                                const std::string& primary_key);
  std::string BuildCompositeKeyPrefix(const std::string& field_name, 
                                const std::string& encoded_value);
  std::vector<std::string> ExtractPrimaryKeys(const std::vector<std::string>& composite_keys);

  std::unique_ptr<Storage> storage_;
  std::unique_ptr<TransactionManager> contract_manager_;
  
  // Composite key configuration
  const std::string composite_key_separator_ = ":";
  const std::string composite_key_prefix_ = "idx";

  //TODO: add protocol versioning support
  const std::string version_ = "v1";
};

}  // namespace resdb

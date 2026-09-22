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
#include <string>
#include <unordered_map>
#include <vector>

#include "chain/storage/storage.h"
#include "executor/common/transaction_manager.h"
#include "proto/kv/kv.pb.h"

namespace resdb {

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
  std::string ExecuteSQL(const std::string& sql_query);

  // Secondary indexes (issue #180). An index entry says which attributes a
  // record has, e.g. ("by_city", {"Davis"}) -> "user:1", and is stored as the
  // key "ck\0by_city\0Davis\0user:1". Only these functions build such keys.

  // Fails if the record (primary_key) doesn't exist.
  bool CreateIndexEntry(const std::string& index_name,
                        const std::vector<std::string>& attributes,
                        const std::string& primary_key);
  // Fails if the entry doesn't exist.
  bool DeleteIndexEntry(const std::string& index_name,
                        const std::vector<std::string>& attributes,
                        const std::string& primary_key);
  // Moves an entry to new attributes in one write. Fails if it doesn't exist.
  bool UpdateIndexEntry(const std::string& index_name,
                        const std::vector<std::string>& old_attributes,
                        const std::vector<std::string>& new_attributes,
                        const std::string& primary_key);
  // Adds the primary keys of matching entries to `items`, each once, sorted.
  void QueryByIndex(const std::string& index_name,
                    const std::vector<std::string>& attribute_prefix,
                    Items* items);

 private:
  // Shared by ExecuteRequest and ExecuteData.
  void ExecuteIndexCommand(const KVRequest& request, KVResponse* response);
  bool PrimaryKeyExists(const std::string& key);
  bool IndexEntryExists(const std::string& composite_key);

  std::unique_ptr<TransactionManager> contract_manager_;
};

}  // namespace resdb

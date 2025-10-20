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

class CompositeKeyBuilderBase {
 public:
  virtual ~CompositeKeyBuilderBase() = default;
  virtual std::string Build(const std::string& field_name,
                            const std::string& encoded_value,
                            const std::string& primary_key) = 0;
  virtual std::string BuildPrefix(const std::string& field_name,
                                  const std::string& encoded_value) = 0;
  virtual std::string BuildLowerBound(const std::string& field_name,
                                      const std::string& encoded_min_value) = 0;
  virtual std::string BuildUpperBound(const std::string& field_name,
                                      const std::string& encoded_max_value) = 0;
  virtual std::string GetSeparator() const = 0;
  virtual std::string GetPrefix() const = 0;
  virtual std::string GetVersion() const = 0;
};

class DefaultCompositeKeyBuilder : public CompositeKeyBuilderBase {
 public:
  DefaultCompositeKeyBuilder(const std::string& separator,
                             const std::string& prefix,
                             const std::string& version)
      : separator_(separator), prefix_(prefix), version_(version) {}

  std::string Build(const std::string& field_name,
                    const std::string& encoded_value,
                    const std::string& primary_key) override;

  std::string BuildPrefix(const std::string& field_name,
                          const std::string& encoded_value) override;

  std::string BuildLowerBound(const std::string& field_name,
                              const std::string& encoded_min_value) override;

  std::string BuildUpperBound(const std::string& field_name,
                              const std::string& encoded_max_value) override;

  std::string GetSeparator() const override { return separator_; }
  std::string GetPrefix() const override { return prefix_; }
  std::string GetVersion() const override { return version_; }

  class KeyBuilder {
   public:
    explicit KeyBuilder(const std::string& separator);
    KeyBuilder& add(const std::string& component);
    KeyBuilder& addSeparator();
    std::string build() const;

   private:
    std::string result_;
    const std::string separator_;
  };

  KeyBuilder CreateBuilder() const;

 private:
  const std::string separator_;
  const std::string prefix_;
  const std::string version_;
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
  int Delete(const std::string& key);
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
  int UpdateCompositeKey(const std::string& primary_key,
                         const std::string& field_name,
                         const std::string& old_field_value,
                         const std::string& new_field_value,
                         CompositeKeyType old_field_type,
                         CompositeKeyType new_field_type);

  void SetCompositeKeyBuilder(std::unique_ptr<CompositeKeyBuilderBase> builder);

 private:
  std::string EncodeValue(const std::string& value,
                          CompositeKeyType field_type);
  std::string EncodeInteger(int32_t value);
  std::string EncodeBoolean(bool value);
  std::string EncodeTimestamp(int64_t value);

  std::string BuildCompositeKey(const std::string& field_name,
                                const std::string& encoded_value,
                                const std::string& primary_key);
  std::string BuildCompositeKeyPrefix(const std::string& field_name,
                                      const std::string& encoded_value);
  std::vector<std::string> ExtractPrimaryKeys(
      const std::vector<std::string>& composite_keys);

  std::unique_ptr<Storage> storage_;
  std::unique_ptr<TransactionManager> contract_manager_;
  std::unique_ptr<CompositeKeyBuilderBase> composite_key_builder_;

  const std::string composite_val_marker_ = "Y";
};

}  // namespace resdb

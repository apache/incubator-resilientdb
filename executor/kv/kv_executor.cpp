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

#include "executor/kv/kv_executor.h"

#include <glog/logging.h>

#include "executor/contract/executor/contract_executor.h"

namespace resdb {

KVExecutor::KVExecutor(std::unique_ptr<Storage> storage)
    : storage_(std::move(storage)) {
  contract_manager_ =
      std::make_unique<resdb::contract::ContractTransactionManager>(
          storage_.get());

  // Initialize default composite key builder
  composite_key_builder_ = std::make_unique<DefaultCompositeKeyBuilder>(
      ":", "idx", "v1");
}

std::unique_ptr<google::protobuf::Message> KVExecutor::ParseData(
    const std::string& request) {
  std::unique_ptr<KVRequest> kv_request = std::make_unique<KVRequest>();
  if (!kv_request->ParseFromString(request)) {
    LOG(ERROR) << "parse data fail";
    return nullptr;
  }
  return kv_request;
}

std::unique_ptr<std::string> KVExecutor::ExecuteRequest(
    const google::protobuf::Message& request) {
  KVResponse kv_response;
  const KVRequest& kv_request = dynamic_cast<const KVRequest&>(request);

  if (kv_request.cmd() == KVRequest::SET) {
    Set(kv_request.key(), kv_request.value());
  } else if (kv_request.cmd() == KVRequest::GET) {
    kv_response.set_value(Get(kv_request.key()));
  } else if (kv_request.cmd() == KVRequest::DEL_VAL) {
    kv_response.set_value(std::to_string(Delete(kv_request.key())));
  } else if (kv_request.cmd() == KVRequest::GETALLVALUES) {
    kv_response.set_value(GetAllValues());
  } else if (kv_request.cmd() == KVRequest::GETRANGE) {
    kv_response.set_value(GetRange(kv_request.key(), kv_request.value()));
  } else if (kv_request.cmd() == KVRequest::SET_WITH_VERSION) {
    SetWithVersion(kv_request.key(), kv_request.value(), kv_request.version());
  } else if (kv_request.cmd() == KVRequest::GET_WITH_VERSION) {
    GetWithVersion(kv_request.key(), kv_request.version(),
                   kv_response.mutable_value_info());
  } else if (kv_request.cmd() == KVRequest::GET_ALL_ITEMS) {
    GetAllItems(kv_response.mutable_items());
  } else if (kv_request.cmd() == KVRequest::GET_KEY_RANGE) {
    GetKeyRange(kv_request.min_key(), kv_request.max_key(),
                kv_response.mutable_items());
  } else if (kv_request.cmd() == KVRequest::GET_HISTORY) {
    GetHistory(kv_request.key(), kv_request.min_version(),
               kv_request.max_version(), kv_response.mutable_items());
  } else if (kv_request.cmd() == KVRequest::GET_TOP) {
    GetTopHistory(kv_request.key(), kv_request.top_number(),
                  kv_response.mutable_items());
  } else if (kv_request.cmd() == KVRequest::CREATE_COMPOSITE_KEY) {
    int status = CreateCompositeKey(
        kv_request.key(), kv_request.field_name(), kv_request.value(),
        static_cast<CompositeKeyType>(kv_request.field_type()));
    kv_response.set_value(std::to_string(status));
  } else if (kv_request.cmd() == KVRequest::UPDATE_COMPOSITE_KEY) {
    int status = UpdateCompositeKey(
        kv_request.key(), kv_request.field_name(), kv_request.min_value(),
        kv_request.max_value(),
        static_cast<CompositeKeyType>(kv_request.field_type()),
        static_cast<CompositeKeyType>(kv_request.field_type()));
    kv_response.set_value(std::to_string(status));
  } else if (kv_request.cmd() == KVRequest::GET_BY_COMPOSITE_KEY) {
    auto results = GetByCompositeKey(
        kv_request.field_name(), kv_request.value(),
        static_cast<CompositeKeyType>(kv_request.field_type()));
    std::string result;
    Items* items = kv_response.mutable_items();
    for (const auto& document : results) {
      Item* item = items->add_item();
      item->set_key("");
      item->mutable_value_info()->set_value(document);
    }
  } else if (kv_request.cmd() == KVRequest::GET_COMPOSITE_KEY_RANGE) {
    auto results = GetByCompositeKeyRange(
        kv_request.field_name(), kv_request.min_value(), kv_request.max_value(),
        static_cast<CompositeKeyType>(kv_request.field_type()));
    Items* items = kv_response.mutable_items();
    for (const auto& document : results) {
      Item* item = items->add_item();
      item->set_key("");
      item->mutable_value_info()->set_value(document);
    }
  } else if (!kv_request.smart_contract_request().empty()) {
    std::unique_ptr<std::string> resp =
        contract_manager_->ExecuteData(kv_request.smart_contract_request());
    if (resp != nullptr) {
      kv_response.set_smart_contract_response(*resp);
    }
  }

  std::unique_ptr<std::string> resp_str = std::make_unique<std::string>();
  if (!kv_response.SerializeToString(resp_str.get())) {
    return nullptr;
  }

  return resp_str;
}

std::unique_ptr<std::string> KVExecutor::ExecuteData(
    const std::string& request) {
  KVRequest kv_request;
  KVResponse kv_response;

  if (!kv_request.ParseFromString(request)) {
    LOG(ERROR) << "parse data fail";
    return nullptr;
  }

  LOG(ERROR) << " execute cmd:" << kv_request.cmd();
  if (kv_request.cmd() == KVRequest::SET) {
    Set(kv_request.key(), kv_request.value());
  } else if (kv_request.cmd() == KVRequest::GET) {
    kv_response.set_value(Get(kv_request.key()));
  } else if (kv_request.cmd() == KVRequest::DEL_VAL) {
    kv_response.set_value(std::to_string(Delete(kv_request.key())));
  } else if (kv_request.cmd() == KVRequest::GETALLVALUES) {
    kv_response.set_value(GetAllValues());
  } else if (kv_request.cmd() == KVRequest::GETRANGE) {
    kv_response.set_value(GetRange(kv_request.key(), kv_request.value()));
  } else if (kv_request.cmd() == KVRequest::SET_WITH_VERSION) {
    SetWithVersion(kv_request.key(), kv_request.value(), kv_request.version());
  } else if (kv_request.cmd() == KVRequest::GET_WITH_VERSION) {
    GetWithVersion(kv_request.key(), kv_request.version(),
                   kv_response.mutable_value_info());
  } else if (kv_request.cmd() == KVRequest::GET_ALL_ITEMS) {
    GetAllItems(kv_response.mutable_items());
  } else if (kv_request.cmd() == KVRequest::GET_KEY_RANGE) {
    GetKeyRange(kv_request.min_key(), kv_request.max_key(),
                kv_response.mutable_items());
  } else if (kv_request.cmd() == KVRequest::GET_HISTORY) {
    GetHistory(kv_request.key(), kv_request.min_version(),
               kv_request.max_version(), kv_response.mutable_items());
  } else if (kv_request.cmd() == KVRequest::GET_TOP) {
    GetTopHistory(kv_request.key(), kv_request.top_number(),
                  kv_response.mutable_items());
  } else if (kv_request.cmd() == KVRequest::CREATE_COMPOSITE_KEY) {
    int status = CreateCompositeKey(
        kv_request.key(), kv_request.field_name(), kv_request.value(),
        static_cast<CompositeKeyType>(kv_request.field_type()));
    kv_response.set_value(std::to_string(status));
  } else if (kv_request.cmd() == KVRequest::GET_BY_COMPOSITE_KEY) {
    auto results = GetByCompositeKey(
        kv_request.field_name(), kv_request.value(),
        static_cast<CompositeKeyType>(kv_request.field_type()));
    Items* items = kv_response.mutable_items();
    for (const auto& document : results) {
      Item* item = items->add_item();
      item->set_key("");
      item->mutable_value_info()->set_value(document);
    }
  } else if (kv_request.cmd() == KVRequest::UPDATE_COMPOSITE_KEY) {
    int status = UpdateCompositeKey(
        kv_request.key(), kv_request.field_name(), kv_request.min_value(),
        kv_request.max_value(),
        static_cast<CompositeKeyType>(kv_request.field_type()),
        static_cast<CompositeKeyType>(kv_request.field_type()));
    kv_response.set_value(std::to_string(status));
  } else if (kv_request.cmd() == KVRequest::GET_COMPOSITE_KEY_RANGE) {
    auto results = GetByCompositeKeyRange(
        kv_request.field_name(), kv_request.min_value(), kv_request.max_value(),
        static_cast<CompositeKeyType>(kv_request.field_type()));
    Items* items = kv_response.mutable_items();
    for (const auto& document : results) {
      Item* item = items->add_item();
      item->set_key("");
      item->mutable_value_info()->set_value(document);
    }
  } else if (!kv_request.smart_contract_request().empty()) {
    std::unique_ptr<std::string> resp =
        contract_manager_->ExecuteData(kv_request.smart_contract_request());
    if (resp != nullptr) {
      kv_response.set_smart_contract_response(*resp);
    }
  }

  std::unique_ptr<std::string> resp_str = std::make_unique<std::string>();
  if (!kv_response.SerializeToString(resp_str.get())) {
    return nullptr;
  }
  return resp_str;
}

void KVExecutor::Set(const std::string& key, const std::string& value) {
  storage_->SetValue(key, value);
}

std::string KVExecutor::Get(const std::string& key) {
  return storage_->GetValue(key);
}

int KVExecutor::Delete(const std::string& key) {
  return storage_->DelValue(key);
}

std::string KVExecutor::GetAllValues() { return storage_->GetAllValues(); }

std::string KVExecutor::GetRange(const std::string& min_key,
                                 const std::string& max_key) {
  return storage_->GetRange(min_key, max_key);
}

void KVExecutor::SetWithVersion(const std::string& key,
                                const std::string& value, int version) {
  storage_->SetValueWithVersion(key, value, version);
}

void KVExecutor::GetWithVersion(const std::string& key, int version,
                                ValueInfo* info) {
  std::pair<std::string, int> ret = storage_->GetValueWithVersion(key, version);
  info->set_value(ret.first);
  info->set_version(ret.second);
}

void KVExecutor::GetAllItems(Items* items) {
  const std::map<std::string, std::pair<std::string, int>>& ret =
      storage_->GetAllItems();
  for (auto it : ret) {
    Item* item = items->add_item();
    item->set_key(it.first);
    item->mutable_value_info()->set_value(it.second.first);
    item->mutable_value_info()->set_version(it.second.second);
  }
}

void KVExecutor::GetKeyRange(const std::string& min_key,
                             const std::string& max_key, Items* items) {
  const std::map<std::string, std::pair<std::string, int>>& ret =
      storage_->GetKeyRange(min_key, max_key);
  for (auto it : ret) {
    Item* item = items->add_item();
    item->set_key(it.first);
    item->mutable_value_info()->set_value(it.second.first);
    item->mutable_value_info()->set_version(it.second.second);
  }
}

void KVExecutor::GetHistory(const std::string& key, int min_version,
                            int max_version, Items* items) {
  const std::vector<std::pair<std::string, int>>& ret =
      storage_->GetHistory(key, min_version, max_version);
  for (auto it : ret) {
    Item* item = items->add_item();
    item->set_key(key);
    item->mutable_value_info()->set_value(it.first);
    item->mutable_value_info()->set_version(it.second);
  }
}

void KVExecutor::GetTopHistory(const std::string& key, int top_number,
                               Items* items) {
  const std::vector<std::pair<std::string, int>>& ret =
      storage_->GetTopHistory(key, top_number);
  for (auto it : ret) {
    Item* item = items->add_item();
    item->set_key(key);
    item->mutable_value_info()->set_value(it.first);
    item->mutable_value_info()->set_version(it.second);
  }
}

// Method implementations for DefaultCompositeKeyBuilder::KeyBuilder
DefaultCompositeKeyBuilder::KeyBuilder::KeyBuilder(const std::string& separator)
    : separator_(separator) {
  result_.reserve(256);
}

DefaultCompositeKeyBuilder::KeyBuilder&
DefaultCompositeKeyBuilder::KeyBuilder::add(const std::string& component) {
  if (!result_.empty()) {
    result_ += separator_;
  }
  result_ += component;
  return *this;
}

DefaultCompositeKeyBuilder::KeyBuilder&
DefaultCompositeKeyBuilder::KeyBuilder::addSeparator() {
  result_ += separator_;
  return *this;
}

std::string DefaultCompositeKeyBuilder::KeyBuilder::build() const {
  return result_;
}

DefaultCompositeKeyBuilder::KeyBuilder
DefaultCompositeKeyBuilder::CreateBuilder() const {
  return KeyBuilder(separator_);
}

std::string DefaultCompositeKeyBuilder::Build(const std::string& field_name,
                                              const std::string& encoded_value,
                                              const std::string& primary_key) {
  return CreateBuilder()
      .add(prefix_)
      .add(version_)
      .add(field_name)
      .add(encoded_value)
      .add(primary_key)
      .build();
}

std::string DefaultCompositeKeyBuilder::BuildPrefix(
    const std::string& field_name, const std::string& encoded_value) {
  return CreateBuilder()
      .add(prefix_)
      .add(version_)
      .add(field_name)
      .add(encoded_value)
      .addSeparator()
      .build();
}

std::string DefaultCompositeKeyBuilder::BuildLowerBound(
    const std::string& field_name, const std::string& encoded_min_value) {
  return CreateBuilder()
      .add(prefix_)
      .add(version_)
      .add(field_name)
      .add(encoded_min_value)
      .addSeparator()
      .build();
}

std::string DefaultCompositeKeyBuilder::BuildUpperBound(
    const std::string& field_name, const std::string& encoded_max_value) {
  return CreateBuilder()
      .add(prefix_)
      .add(version_)
      .add(field_name)
      .add(encoded_max_value)
      .add("\xFF")
      .addSeparator()
      .build();
}

std::string KVExecutor::BuildCompositeKey(const std::string& field_name,
                                          const std::string& encoded_value,
                                          const std::string& primary_key) {
  return composite_key_builder_->Build(field_name, encoded_value, primary_key);
}

std::string KVExecutor::BuildCompositeKeyPrefix(
    const std::string& field_name, const std::string& encoded_value) {
  return composite_key_builder_->BuildPrefix(field_name, encoded_value);
}

void KVExecutor::SetCompositeKeyBuilder(
    std::unique_ptr<CompositeKeyBuilderBase> builder) {
  composite_key_builder_ = std::move(builder);
}

std::vector<std::string> KVExecutor::ExtractPrimaryKeys(
    const std::vector<std::string>& composite_keys) {
  std::vector<std::string> primary_keys;
  if (!composite_key_builder_) {
    LOG(ERROR) << "composite key builder is not initialized";
    return primary_keys;
  }
  const std::string separator = composite_key_builder_->GetSeparator();
  for (const auto& composite_key : composite_keys) {
    size_t last_separator = composite_key.find_last_of(separator);
    if (last_separator != std::string::npos) {
      std::string primary_key = composite_key.substr(last_separator + 1);
      primary_keys.push_back(primary_key);
    } else {
      LOG(ERROR) << "invalid composite key. no separator found in composite key"
                 << composite_key;
    }
  }
  return primary_keys;
}

int KVExecutor::CreateCompositeKey(const std::string& primary_key,
                                   const std::string& field_name,
                                   const std::string& field_value,
                                   CompositeKeyType field_type) {
  std::string encoded_value = EncodeValue(field_value, field_type);
  std::string composite_key =
      BuildCompositeKey(field_name, encoded_value, primary_key);
  return storage_->SetValue(composite_key, composite_val_marker_);
}

std::vector<std::string> KVExecutor::GetByCompositeKey(
    const std::string& field_name, const std::string& field_value,
    CompositeKeyType field_type) {
  std::string encoded_value = EncodeValue(field_value, field_type);
  std::string prefix = BuildCompositeKeyPrefix(field_name, encoded_value);

  auto results = storage_->GetKeysByPrefix(prefix);

  std::vector<std::string> primary_keys = ExtractPrimaryKeys(results);

  std::vector<std::string> documents;
  for (const auto& primary_key : primary_keys) {
    std::string document = storage_->GetValue(primary_key);
    if (!document.empty()) {
      documents.push_back(document);
    }
  }
  return documents;
}

std::vector<std::string> KVExecutor::GetByCompositeKeyRange(
    const std::string& field_name, const std::string& min_value,
    const std::string& max_value, CompositeKeyType field_type) {
  std::string encoded_min = EncodeValue(min_value, field_type);
  std::string encoded_max = EncodeValue(max_value, field_type);

  std::string start_key =
      composite_key_builder_->BuildLowerBound(field_name, encoded_min);
  std::string end_key =
      composite_key_builder_->BuildUpperBound(field_name, encoded_max);

  std::vector<std::string> composite_keys =
      storage_->GetKeyRangeByPrefix(start_key, end_key);

  std::vector<std::string> primary_keys = ExtractPrimaryKeys(composite_keys);

  std::vector<std::string> documents;
  for (const auto& primary_key : primary_keys) {
    std::string document = storage_->GetValue(primary_key);
    if (!document.empty()) {
      documents.push_back(document);
    }
  }

  return documents;
}

int KVExecutor::UpdateCompositeKey(const std::string& primary_key,
                                   const std::string& field_name,
                                   const std::string& old_field_value,
                                   const std::string& new_field_value,
                                   CompositeKeyType old_field_type,
                                   CompositeKeyType new_field_type) {
  std::string encoded_old_field_value =
      EncodeValue(old_field_value, old_field_type);

  std::string delete_composite_key =
      BuildCompositeKey(field_name, encoded_old_field_value, primary_key);
  std::string check = storage_->GetValue(delete_composite_key);

  if (check == composite_val_marker_) {
    int status_delete = storage_->DelValue(delete_composite_key);
    if (status_delete != 0) {
      LOG(ERROR) << "delete composite key status fail";
      return -1;
    }
  }
  int status_create = CreateCompositeKey(
      primary_key, field_name, new_field_value, old_field_type);  // TODO
  if (status_create != 0) {
    LOG(ERROR) << "create composite key status fail";
    return -1;
  }
  return 0;
}

}  // namespace resdb

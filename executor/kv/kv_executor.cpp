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
  LOG(ERROR) << " set key:" << key;
  storage_->SetValue(key, value);
}

std::string KVExecutor::Get(const std::string& key) {
  LOG(ERROR) << " get key:" << key;
  return storage_->GetValue(key);
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

std::string KVExecutor::EncodeValue(const std::string& value,
                                    CompositeKeyType field_type) {
  switch (field_type) {
    case CompositeKeyType::INTEGER:
      return EncodeInteger(std::stoi(value));
    case CompositeKeyType::BOOLEAN:
      return EncodeBoolean(value == "true" || value == "1");
    case CompositeKeyType::STRING:
      return value;
    case CompositeKeyType::TIMESTAMP:
      return EncodeTimestamp(std::stoll(value));
  }
  return value;
}

std::string KVExecutor::EncodeInteger(int32_t value) {
  std::string bytes(4, 0);
  bytes[0] = (value >> 24) & 0xFF;
  bytes[1] = (value >> 16) & 0xFF;
  bytes[2] = (value >> 8) & 0xFF;
  bytes[3] = value & 0xFF;
  return bytes;
}

std::string KVExecutor::EncodeBoolean(bool value) {
  return std::string(1, value ? 1 : 0);
}

std::string KVExecutor::EncodeTimestamp(int64_t value) {
  std::string bytes(8, 0);
  for (int i = 0; i < 8; i++) {
    bytes[i] = (value >> (56 - 8 * i)) & 0xFF;
  }
  return bytes;
}

// Helper functions
std::string KVExecutor::BuildCompositeKey(const std::string& field_name,
                                          const std::string& encoded_value,
                                          const std::string& primary_key) {
  return composite_key_prefix_ + composite_key_separator_ + field_name +
         composite_key_separator_ + encoded_value + composite_key_separator_ +
         primary_key;
}

std::vector<std::string> KVExecutor::ExtractPrimaryKeys(
    const std::vector<std::string>& composite_keys) {
  std::vector<std::string> primary_keys;
  for (const auto& composite_key : composite_keys) {
    size_t last_colon = composite_key.find_last_of(composite_key_separator_);
    if (last_colon != std::string::npos) {
      std::string primary_key = composite_key.substr(last_colon + 1);
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
  return storage_->SetValue(composite_key, "");
}

std::vector<std::string> KVExecutor::GetByCompositeKey(
    const std::string& field_name, const std::string& field_value,
    CompositeKeyType field_type) {
  std::string encoded_value = EncodeValue(field_value, field_type);
  std::string prefix = composite_key_prefix_ + composite_key_separator_ +
                       field_name + composite_key_separator_ + encoded_value +
                       composite_key_separator_;

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

  std::string start_key = composite_key_prefix_ + composite_key_separator_ +
                          field_name + composite_key_separator_ + encoded_min +
                          composite_key_separator_;

  std::string end_key = composite_key_prefix_ + composite_key_separator_ +
                        field_name + composite_key_separator_ + encoded_max +
                        composite_key_separator_ + "\xFF";

  std::vector<std::string> composite_keys = storage_->GetKeyRangeByPrefix(start_key, end_key);

  std::cout << "Found " << composite_keys.size()
            << " composite keys:" << std::endl;
  for (const auto& key : composite_keys) {
    std::cout << "  " << key << std::endl;
  }

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

}  // namespace resdb

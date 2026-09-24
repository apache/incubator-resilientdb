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

#include <set>

#include "chain/storage/composite_key_codec.h"
#include "executor/contract/executor/contract_executor.h"

namespace resdb {
namespace {

// Index entry keys start with "ck\0". SET must not write them, or a client
// could plant fake index entries.
bool IsCompositeKeyRange(const std::string& key) {
  static const std::string kPrefix = std::string(
      storage::kCompositeKeyNamespace) + storage::kCompositeKeyDelim;
  return key.compare(0, kPrefix.size(), kPrefix) == 0;
}

}  // namespace

KVExecutor::KVExecutor(std::unique_ptr<Storage> storage) {
  storage_ = std::move(storage);
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
  } else if (kv_request.cmd() == KVRequest::SQL) {
    std::string result = ExecuteSQL(kv_request.sql_query());
    kv_response.set_sql_response(result);
    kv_response.set_value(result);  // keep legacy field populated
  } else if (kv_request.cmd() == KVRequest::CREATE_INDEX_ENTRY ||
             kv_request.cmd() == KVRequest::DELETE_INDEX_ENTRY ||
             kv_request.cmd() == KVRequest::UPDATE_INDEX_ENTRY ||
             kv_request.cmd() == KVRequest::QUERY_BY_INDEX) {
    ExecuteIndexCommand(kv_request, &kv_response);
  }
  else if(!kv_request.smart_contract_request().empty()){
    std::unique_ptr<std::string> resp = contract_manager_->ExecuteData(kv_request.smart_contract_request());
    if(resp != nullptr){
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
  }  else if (kv_request.cmd() == KVRequest::SQL) {
    std::string result = ExecuteSQL(kv_request.sql_query());
    kv_response.set_sql_response(result);
    kv_response.set_value(result);  // keep legacy field populated
  } else if (kv_request.cmd() == KVRequest::CREATE_INDEX_ENTRY ||
             kv_request.cmd() == KVRequest::DELETE_INDEX_ENTRY ||
             kv_request.cmd() == KVRequest::UPDATE_INDEX_ENTRY ||
             kv_request.cmd() == KVRequest::QUERY_BY_INDEX) {
    ExecuteIndexCommand(kv_request, &kv_response);
  }
  else if(!kv_request.smart_contract_request().empty()){
    std::unique_ptr<std::string> resp = contract_manager_->ExecuteData(kv_request.smart_contract_request());
    if(resp != nullptr){
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
  if (IsCompositeKeyRange(key)) {
    LOG(ERROR) << "SET rejected: keys starting with \"ck\\0\" are reserved "
                  "for secondary indexes";
    return;
  }
  storage_->SetValueWithSeq(key, value, seq_);
}

std::string KVExecutor::Get(const std::string& key) {
  return storage_->GetValueWithSeq(key, 0).first;
}

std::string KVExecutor::GetAllValues() { return ""; }

// Get values on a range of keys
std::string KVExecutor::GetRange(const std::string& min_key,
                                 const std::string& max_key) {
  return storage_->GetRange(min_key, max_key);
}

void KVExecutor::SetWithVersion(const std::string& key,
                                const std::string& value, int version) {
  if (IsCompositeKeyRange(key)) {
    LOG(ERROR) << "SET_WITH_VERSION rejected: keys starting with \"ck\\0\" "
                  "are reserved for secondary indexes";
    return;
  }
  storage_->SetValueWithVersion(key, value, version);
}

void KVExecutor::GetWithVersion(const std::string& key, int version,
                                ValueInfo* info) {
  std::pair<std::string, int> ret = storage_->GetValueWithVersion(key, version);
  info->set_value(ret.first);
  info->set_version(ret.second);
}

void KVExecutor::GetAllItems(Items* items) { return; }

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

std::string KVExecutor::ExecuteSQL(const std::string& sql_query) {
  // Basic validation: SQL commands should carry a query string.
  if (sql_query.empty()) {
    LOG(ERROR) << "SQL command received with empty sql_query";
    return "Error: empty SQL query";
  }

  return storage_->ExecuteSQL(sql_query);
}

void KVExecutor::ExecuteIndexCommand(const KVRequest& request,
                                     KVResponse* response) {
  std::vector<std::string> attributes(request.attributes().begin(),
                                      request.attributes().end());
  switch (request.cmd()) {
    case KVRequest::CREATE_INDEX_ENTRY:
      response->set_success(CreateIndexEntry(request.index_name(), attributes,
                                             request.primary_key()));
      break;
    case KVRequest::DELETE_INDEX_ENTRY:
      response->set_success(DeleteIndexEntry(request.index_name(), attributes,
                                             request.primary_key()));
      break;
    case KVRequest::UPDATE_INDEX_ENTRY: {
      std::vector<std::string> new_attributes(request.new_attributes().begin(),
                                              request.new_attributes().end());
      response->set_success(UpdateIndexEntry(request.index_name(), attributes,
                                             new_attributes,
                                             request.primary_key()));
      break;
    }
    case KVRequest::QUERY_BY_INDEX:
      QueryByIndex(request.index_name(), attributes, request.with_values(),
                   response->mutable_items());
      break;
    default:
      break;
  }
}

// MemoryDB stores SET and SET_WITH_VERSION records separately, so check both.
bool KVExecutor::PrimaryKeyExists(const std::string& key) {
  return !Get(key).empty() ||
         !storage_->GetValueWithVersion(key, 0).first.empty();
}

// Exact match: a prefix search for "user:1" also returns "user:10".
bool KVExecutor::IndexEntryExists(const std::string& composite_key) {
  for (const std::string& key :
       storage_->GetByCompositeKeyPrefix(composite_key)) {
    if (key == composite_key) {
      return true;
    }
  }
  return false;
}

bool KVExecutor::CreateIndexEntry(const std::string& index_name,
                                  const std::vector<std::string>& attributes,
                                  const std::string& primary_key) {
  if (index_name.empty() || primary_key.empty()) {
    LOG(ERROR) << "CreateIndexEntry: index_name and primary_key are required";
    return false;
  }
  if (!PrimaryKeyExists(primary_key)) {
    LOG(ERROR) << "CreateIndexEntry: primary key not found: " << primary_key;
    return false;
  }
  std::string composite_key =
      storage::EncodeCompositeKey(index_name, attributes, primary_key);
  if (composite_key.empty()) {
    LOG(ERROR) << "CreateIndexEntry: input contains a \\0 byte";
    return false;
  }
  return storage_->CreateCompositeKey(composite_key) == 0;
}

bool KVExecutor::DeleteIndexEntry(const std::string& index_name,
                                  const std::vector<std::string>& attributes,
                                  const std::string& primary_key) {
  if (index_name.empty() || primary_key.empty()) {
    LOG(ERROR) << "DeleteIndexEntry: index_name and primary_key are required";
    return false;
  }
  std::string composite_key =
      storage::EncodeCompositeKey(index_name, attributes, primary_key);
  if (composite_key.empty() || !IndexEntryExists(composite_key)) {
    LOG(ERROR) << "DeleteIndexEntry: no such index entry";
    return false;
  }
  return storage_->DeleteCompositeKey(composite_key) == 0;
}

bool KVExecutor::UpdateIndexEntry(
    const std::string& index_name,
    const std::vector<std::string>& old_attributes,
    const std::vector<std::string>& new_attributes,
    const std::string& primary_key) {
  if (index_name.empty() || primary_key.empty()) {
    LOG(ERROR) << "UpdateIndexEntry: index_name and primary_key are required";
    return false;
  }
  std::string old_key =
      storage::EncodeCompositeKey(index_name, old_attributes, primary_key);
  std::string new_key =
      storage::EncodeCompositeKey(index_name, new_attributes, primary_key);
  if (old_key.empty() || new_key.empty()) {
    LOG(ERROR) << "UpdateIndexEntry: input contains a \\0 byte";
    return false;
  }
  if (!IndexEntryExists(old_key)) {
    LOG(ERROR) << "UpdateIndexEntry: no existing entry to update";
    return false;
  }
  return storage_->UpdateCompositeKey(old_key, new_key) == 0;
}

void KVExecutor::QueryByIndex(const std::string& index_name,
                              const std::vector<std::string>& attribute_prefix,
                              bool with_values, Items* items) {
  if (index_name.empty()) {
    LOG(ERROR) << "QueryByIndex: index_name is required";
    return;
  }
  std::string prefix =
      storage::EncodeCompositeKeyPrefix(index_name, attribute_prefix);
  if (prefix.empty()) {
    LOG(ERROR) << "QueryByIndex: input contains a \\0 byte";
    return;
  }
  // A record can have several matching entries (e.g. {"Davis", "active"} and
  // {"Davis", "inactive"}), so collect into a set: each key once, sorted.
  std::set<std::string> primary_keys;
  for (const std::string& composite_key :
       storage_->GetByCompositeKeyPrefix(prefix)) {
    std::string decoded_index;
    std::vector<std::string> decoded_attributes;
    std::string primary_key;
    if (storage::DecodeCompositeKey(composite_key, &decoded_index,
                                    &decoded_attributes, &primary_key)) {
      primary_keys.insert(primary_key);
    }
  }
  for (const std::string& primary_key : primary_keys) {
    Item* item = items->add_item();
    item->set_key(primary_key);
    if (with_values) {
      // Read here, so the caller doesn't send one request per match.
      std::string value = Get(primary_key);
      if (value.empty()) {
        value = storage_->GetValueWithVersion(primary_key, 0).first;
      }
      item->mutable_value_info()->set_value(value);
    }
  }
}

}  // namespace resdb

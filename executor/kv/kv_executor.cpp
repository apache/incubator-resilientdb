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

namespace resdb {

KVExecutor::KVExecutor(std::unique_ptr<Storage> storage)
    : storage_(std::move(storage)) {}

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

std::string KVExecutor::GetAllValues() { return storage_->GetAllValues(); }

// Get values on a range of keys
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

}  // namespace resdb

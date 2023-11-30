/*
 * Copyright (c) 2019-2022 ExpoLab, UC Davis
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
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
  LOG(ERROR)<<"EXEC TEST";

  if (!kv_request.ParseFromString(request)) {
    LOG(ERROR) << "parse data fail";
    return nullptr;
  }

  if (kv_request.cmd() == Operation::SET) {
    Set(kv_request.key(), kv_request.value());
  } else if (kv_request.cmd() == Operation::GET) {
    kv_response.set_value(Get(kv_request.key()));
  } else if (kv_request.cmd() == Operation::GETALLVALUES) {
    kv_response.set_value(GetAllValues());
  } else if (kv_request.cmd() == Operation::GETRANGE) {
    kv_response.set_value(GetRange(kv_request.key(), kv_request.value()));
  } else if (kv_request.cmd() == Operation::SET_WITH_VERSION) {
    SetWithVersion(kv_request.key(), kv_request.value(), kv_request.version());
  } else if (kv_request.cmd() == Operation::GET_WITH_VERSION) {
    GetWithVersion(kv_request.key(), kv_request.version(),
                   kv_response.mutable_value_info());
  } else if (kv_request.cmd() == Operation::GET_ALL_ITEMS) {
    GetAllItems(kv_response.mutable_items());
  } else if (kv_request.cmd() == Operation::GET_KEY_RANGE) {
    GetKeyRange(kv_request.min_key(), kv_request.max_key(),
                kv_response.mutable_items());
  } else if (kv_request.cmd() == Operation::GET_HISTORY) {
    GetHistory(kv_request.key(), kv_request.min_version(),
               kv_request.max_version(), kv_response.mutable_items());
  } else if (kv_request.cmd() == Operation::GET_TOP) {
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

Storage* KVExecutor::GetStorage() { return state_->GetStorage(); }

}  // namespace resdb

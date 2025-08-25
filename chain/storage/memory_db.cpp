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

#include "chain/storage/memory_db.h"

#include <glog/logging.h>

namespace resdb {
namespace storage {

std::unique_ptr<Storage> NewMemoryDB() { return std::make_unique<MemoryDB>(); }

MemoryDB::MemoryDB() {}

int MemoryDB::SetValue(const std::string& key, const std::string& value) {
  kv_map_[key] = value;
  return 0;
}

int MemoryDB::DelValue(const std::string& key) {
  kv_map_.erase(key);
  return 0;
}

std::string MemoryDB::GetAllValues(void) {
  std::string values = "[";
  bool first_iteration = true;
  for (auto kv : kv_map_) {
    if (!first_iteration) values.append(",");
    first_iteration = false;
    values.append(kv.second);
  }
  values.append("]");
  return values;
}

std::string MemoryDB::GetRange(const std::string& min_key,
                               const std::string& max_key) {
  std::string values = "[";
  bool first_iteration = true;
  for (auto kv : kv_map_) {
    if (kv.first >= min_key && kv.first <= max_key) {
      if (!first_iteration) values.append(",");
      first_iteration = false;
      values.append(kv.second);
    }
  }
  values.append("]");
  return values;
}

std::string MemoryDB::GetValue(const std::string& key) {
  auto search = kv_map_.find(key);
  if (search != kv_map_.end())
    return search->second;
  else {
    return "";
  }
}

int MemoryDB::SetValueWithVersion(const std::string& key,
                                  const std::string& value, int version) {
  auto it = kv_map_with_v_.find(key);
  if ((it == kv_map_with_v_.end() && version != 0) ||
      (it != kv_map_with_v_.end() && it->second.back().second != version)) {
    LOG(ERROR) << " value version not match. key:" << key << " db version:"
               << (it == kv_map_with_v_.end() ? 0 : it->second.back().second)
               << " user version:" << version;
    return -2;
  }
  kv_map_with_v_[key].push_back(std::make_pair(value, version + 1));
  return 0;
}

std::pair<std::string, int> MemoryDB::GetValueWithVersion(
    const std::string& key, int version) {
  auto search_it = kv_map_with_v_.find(key);
  if (search_it != kv_map_with_v_.end() && search_it->second.size()) {
    auto it = search_it->second.end();
    do {
      --it;
      if (it->second == version) {
        return *it;
      }
      if (it->second < version) {
        break;
      }
    } while (it != search_it->second.begin());
    it = --search_it->second.end();
    LOG(ERROR) << " key:" << key << " no version:" << version
               << " return max:" << it->second;
    return *it;
  }
  return std::make_pair("", 0);
}

std::map<std::string, std::pair<std::string, int>> MemoryDB::GetAllItems() {
  std::map<std::string, std::pair<std::string, int>> resp;

  for (const auto& it : kv_map_with_v_) {
    resp.insert(std::make_pair(it.first, it.second.back()));
  }
  return resp;
}

std::map<std::string, std::pair<std::string, int>> MemoryDB::GetKeyRange(
    const std::string& min_key, const std::string& max_key) {
  LOG(ERROR) << "min key:" << min_key << " max key:" << max_key;
  std::map<std::string, std::pair<std::string, int>> resp;
  for (const auto& it : kv_map_with_v_) {
    if (it.first >= min_key && it.first <= max_key) {
      resp.insert(std::make_pair(it.first, it.second.back()));
    }
  }
  return resp;
}

std::vector<std::pair<std::string, int>> MemoryDB::GetHistory(
    const std::string& key, int min_version, int max_version) {
  std::vector<std::pair<std::string, int>> resp;
  auto search_it = kv_map_with_v_.find(key);
  if (search_it == kv_map_with_v_.end()) {
    return resp;
  }

  auto it = search_it->second.end();
  do {
    --it;
    if (it->second < min_version) {
      break;
    }
    if (it->second <= max_version) {
      resp.push_back(*it);
    }
  } while (it != search_it->second.begin());
  return resp;
}

std::vector<std::pair<std::string, int>> MemoryDB::GetTopHistory(
    const std::string& key, int top_number) {
  std::vector<std::pair<std::string, int>> resp;
  auto search_it = kv_map_with_v_.find(key);
  if (search_it == kv_map_with_v_.end()) {
    return resp;
  }

  auto it = search_it->second.end();
  do {
    --it;
    resp.push_back(*it);
    if (resp.size() >= static_cast<size_t>(top_number)) {
      break;
    }
  } while (it != search_it->second.begin());
  return resp;
}

std::vector<std::string> MemoryDB::GetKeysByPrefix(const std::string& prefix) {
  std::vector<std::string> resp;
  for (const auto& kv : kv_map_) {
    if (kv.first.find(prefix) == 0) { 
      resp.push_back(kv.second);
    }
  }
  return resp;
}

std::vector<std::string> MemoryDB::GetKeyRangeByPrefix(const std::string& start_prefix, const std::string& end_prefix) {
  std::vector<std::string> resp;
  for (const auto& kv : kv_map_) {
    if (kv.first >= start_prefix && kv.first <= end_prefix) {
      resp.push_back(kv.first);
    }
  }
  return resp;
}

}  // namespace storage
}  // namespace resdb

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

}  // namespace storage
}  // namespace resdb

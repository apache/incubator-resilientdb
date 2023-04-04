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

#include "storage/in_mem_kv_storage.h"

#include <glog/logging.h>

namespace resdb {

std::unique_ptr<Storage> NewInMemKVStorage() {
  return std::make_unique<InMemKVStorage>();
}

int InMemKVStorage::SetValue(const std::string& key, const std::string& value) {
  kv_map_[key] = value;
  return 0;
}

std::string InMemKVStorage::GetValue(const std::string& key) {
  auto search = kv_map_.find(key);
  if (search != kv_map_.end())
    return search->second;
  else
    return "";
}

std::string InMemKVStorage::GetAllValues(void) {
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

std::string InMemKVStorage::GetRange(const std::string& min_key,
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

}  // namespace resdb

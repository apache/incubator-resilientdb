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
#include "platform/consensus/ordering/thunderbolt/executor/x_manager/data_storage.h"

#include <mutex>

#include "glog/logging.h"

namespace resdb {
namespace contract {

namespace {

int GetHashKey(const uint256_t& address) {
  // Get big-endian form
  const uint8_t* bytes = intx::as_bytes(address);
  // uint8_t arr[32] = {};
  // memset(arr,0,sizeof(arr));
  // intx::be::store(arr, address);
  // const uint8_t* bytes = arr;
  size_t sz = sizeof(address);
  int v = 0;
  for (size_t i = 0; i < sz; ++i) {
    v += bytes[i];
  }
  return v % 1024;
}
}  // namespace

int64_t DataStorage::Store(const uint256_t& key, const uint256_t& value, bool) {
  int idx = GetHashKey(key);

  std::unique_lock<std::shared_mutex> lock(mutex_[idx]);
  // LOG(ERROR)<<"store key:"<<key<<" value:"<<value;
  int64_t v = s[idx][key].second;
  s[idx][key] = std::make_pair(value, v + 1);
  return v + 1;
}

std::pair<uint256_t, int64_t> DataStorage::Load(const uint256_t& key,
                                                bool) const {
  int idx = GetHashKey(key);
  std::shared_lock<std::shared_mutex> lock(mutex_[idx]);
  // LOG(ERROR)<<"load key:"<<key;
  auto e = s[idx].find(key);
  if (e == s[idx].end()) return std::make_pair(0, 0);
  return e->second;
}

bool DataStorage::Remove(const uint256_t& key, bool) {
  int idx = GetHashKey(key);
  std::unique_lock<std::shared_mutex> lock(mutex_[idx]);
  auto e = s[idx].find(key);
  if (e == s[idx].end()) return false;
  s[idx].erase(e);
  return true;
}

bool DataStorage::Exist(const uint256_t& key, bool) const {
  int idx = GetHashKey(key);
  std::shared_lock<std::shared_mutex> lock(mutex_[idx]);
  return s[idx].find(key) != s[idx].end();
}

int64_t DataStorage::GetVersion(const uint256_t& key, bool) const {
  int idx = GetHashKey(key);
  std::shared_lock<std::shared_mutex> lock(mutex_[idx]);
  auto it = s[idx].find(key);
  if (it == s[idx].end()) {
    return 0;
  }
  return it->second.second;
}

int64_t DataStorage::StoreWithVersion(const uint256_t& key,
                                      const uint256_t& value, int version,
                                      bool) {
  int idx = GetHashKey(key);
  std::unique_lock<std::shared_mutex> lock(mutex_[idx]);
  s[idx][key] = std::make_pair(value, version);
  return 0;
}

}  // namespace contract
}  // namespace resdb

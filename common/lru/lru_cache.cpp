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

#include "lru_cache.h"

#include "string"

namespace resdb {

template <typename KeyType, typename ValueType>
LRUCache<KeyType, ValueType>::LRUCache(int capacity) {
  capacity_ = capacity;
  cache_hits_ = 0;
  cache_misses_ = 0;
}

template <typename KeyType, typename ValueType>
LRUCache<KeyType, ValueType>::~LRUCache() {
  lookup_.clear();
  key_list_.clear();
  rlookup_.clear();
}

template <typename KeyType, typename ValueType>
ValueType LRUCache<KeyType, ValueType>::Get(KeyType key) {
  if (lookup_.find(key) == lookup_.end()) {
    cache_misses_++;
    return ValueType();
  }

  // Move accessed key to front of key list. This marks the key as used
  key_list_.splice(key_list_.begin(), key_list_, rlookup_[key]);

  cache_hits_++;
  return lookup_[key];
}

template <typename KeyType, typename ValueType>
void LRUCache<KeyType, ValueType>::Put(KeyType key, ValueType value) {
  if (lookup_.find(key) == lookup_.end()) {
    if (key_list_.size() == capacity_) {
      KeyType lru_key = key_list_.back();
      key_list_.pop_back();
      lookup_.erase(lru_key);
      rlookup_.erase(lru_key);
    }
    key_list_.push_front(key);
    rlookup_[key] = key_list_.begin();
  } else {
    key_list_.splice(key_list_.begin(), key_list_, rlookup_[key]);
  }
  lookup_[key] = value;  // Set the lookup_ here
}

template <typename KeyType, typename ValueType>
void LRUCache<KeyType, ValueType>::SetCapacity(int new_capacity) {
  if (new_capacity < capacity_) {
    while (key_list_.size() > new_capacity) {
      KeyType lru_key = key_list_.back();
      key_list_.pop_back();
      lookup_.erase(lru_key);
      rlookup_.erase(lru_key);
    }
  }
  capacity_ = new_capacity;
}

template <typename KeyType, typename ValueType>
int LRUCache<KeyType, ValueType>::GetCapacity() {
  return capacity_;
}

template <typename KeyType, typename ValueType>
void LRUCache<KeyType, ValueType>::Flush() {
  lookup_.clear();
  key_list_.clear();
  rlookup_.clear();
  cache_hits_ = 0;
  cache_misses_ = 0;
}

template <typename KeyType, typename ValueType>
int LRUCache<KeyType, ValueType>::GetCacheHits() const {
  return cache_hits_;
}

template <typename KeyType, typename ValueType>
int LRUCache<KeyType, ValueType>::GetCacheMisses() const {
  return cache_misses_;
}

template <typename KeyType, typename ValueType>
double LRUCache<KeyType, ValueType>::GetCacheHitRatio() const {
  int total_accesses = cache_hits_ + cache_misses_;
  if (total_accesses == 0) {
    return 0.0;
  }
  return static_cast<double>(cache_hits_) / total_accesses;
}

template class LRUCache<int, int>;
template class LRUCache<std::string, int>;
template class LRUCache<int, std::string>;
template class LRUCache<std::string, std::string>;

}  // namespace resdb
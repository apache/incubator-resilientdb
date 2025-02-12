#include "lru_cache.h"

#include <algorithm>

namespace resdb {

template <typename KeyType, typename ValueType>
LRUCache<KeyType, ValueType>::LRUCache(int capacity) {
  m_ = capacity;
  cache_hits_ = 0;
  cache_misses_ = 0;
}

template <typename KeyType, typename ValueType>
LRUCache<KeyType, ValueType>::~LRUCache() {
  um_.clear();
  dq_.clear();
}

template <typename KeyType, typename ValueType>
ValueType LRUCache<KeyType, ValueType>::Get(KeyType key) {
  if (!um_.count(key)) {
    cache_misses_++;
    return ValueType();
  }

  auto it = std::find(dq_.begin(), dq_.end(), key);
  dq_.erase(it);
  dq_.push_front(key);

  cache_hits_++;
  return um_.at(key);
}

template <typename KeyType, typename ValueType>
void LRUCache<KeyType, ValueType>::Put(KeyType key, ValueType value) {
  int s = dq_.size();

  if (!um_.count(key)) {
    if (s == m_) {
      um_.erase(dq_.back());
      dq_.pop_back();
    }
    // Insert the new key and value in the map and add it as most recently
    // used
    um_[key] = value;
    dq_.push_front(key);
  } else {
    // If the key is already in the cache, just update it and move it to the
    // front
    um_[key] = value;
    auto it = std::find(dq_.begin(), dq_.end(), key);
    dq_.erase(it);
    dq_.push_front(key);
  }
}

template <typename KeyType, typename ValueType>
void LRUCache<KeyType, ValueType>::SetCapacity(int new_capacity) {
  if (new_capacity < m_) {
    while (dq_.size() > new_capacity) {
      um_.erase(dq_.back());
      dq_.pop_back();
    }
  }
  m_ = new_capacity;
}

template <typename KeyType, typename ValueType>
void LRUCache<KeyType, ValueType>::Flush() {
  um_.clear();
  dq_.clear();
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

// Explicit instantiations of the template class for commonly used types
template class LRUCache<int, int>;
template class LRUCache<std::string, int>;
template class LRUCache<int, std::string>;
template class LRUCache<std::string, std::string>;
}  // namespace resdb
#include "lru_cache.h"

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
  key_iter_map_.clear();
}

template <typename KeyType, typename ValueType>
ValueType LRUCache<KeyType, ValueType>::Get(KeyType key) {
  if (um_.find(key) == um_.end()) {
    cache_misses_++;
    return ValueType();
  }

  // Move the accessed key to the front of the list
  dq_.splice(dq_.begin(), dq_, key_iter_map_[key]);

  cache_hits_++;
  return um_[key];
}

template <typename KeyType, typename ValueType>
void LRUCache<KeyType, ValueType>::Put(KeyType key, ValueType value) {
  if (um_.find(key) == um_.end()) {
    if (dq_.size() == m_) {
      // Remove the least recently used key
      KeyType lru_key = dq_.back();
      dq_.pop_back();
      um_.erase(lru_key);
      key_iter_map_.erase(lru_key);
    }
    // Insert the new key and value
    dq_.push_front(key);
    key_iter_map_[key] = dq_.begin();
  } else {
    // Update the value and move the key to the front of the list
    um_[key] = value;
    dq_.splice(dq_.begin(), dq_, key_iter_map_[key]);
  }
  um_[key] = value;
}

template <typename KeyType, typename ValueType>
void LRUCache<KeyType, ValueType>::SetCapacity(int new_capacity) {
  if (new_capacity < m_) {
    while (dq_.size() > new_capacity) {
      KeyType lru_key = dq_.back();
      dq_.pop_back();
      um_.erase(lru_key);
      key_iter_map_.erase(lru_key);
    }
  }
  m_ = new_capacity;
}

template <typename KeyType, typename ValueType>
void LRUCache<KeyType, ValueType>::Flush() {
  um_.clear();
  dq_.clear();
  key_iter_map_.clear();
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
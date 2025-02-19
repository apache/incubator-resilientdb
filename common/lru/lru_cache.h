#include <list>
#include <unordered_map>

namespace resdb {

template <typename KeyType, typename ValueType>
class LRUCache {
 public:
  LRUCache(int capacity);
  ~LRUCache();

  ValueType Get(KeyType key);
  void Put(KeyType key, ValueType value);
  void SetCapacity(int new_capacity);
  void Flush();
  int GetCacheHits() const;
  int GetCacheMisses() const;
  double GetCacheHitRatio() const;

 private:
  int capacity_;
  int cache_hits_;
  int cache_misses_;
  std::list<KeyType> key_list_;  // Doubly linked list to store keys
  std::unordered_map<KeyType, ValueType>
      lookup_;  // Hash map for key-value pairs
  std::unordered_map<KeyType, typename std::list<KeyType>::iterator>
      rlookup_;  // Hash map for key-iterator pairs
};

}  // namespace resdb

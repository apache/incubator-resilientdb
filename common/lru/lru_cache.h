#include <deque>
#include <unordered_map>

using namespace std;

namespace resdb {

template <typename KeyType, typename ValueType>
class LRUCache {
 public:
  // Constructor to initialize the cache with a given capacity
  LRUCache(int capacity);

  // Destructor
  ~LRUCache();

  // Get the value of the key if present in the cache
  ValueType Get(KeyType key);

  // Insert or update the key-value pair
  void Put(KeyType key, ValueType value);

  // Method to change the cache capacity
  void SetCapacity(int new_capacity);

  // Method to flush the cache (clear all entries)
  void Flush();

  // Method to get the cache hit count
  int GetCacheHits() const;

  // Method to get the cache miss count
  int GetCacheMisses() const;

  // Method to get the cache Hit Ratio
  double GetCacheHitRatio() const;

 private:
  int m_;             // Cache capacity
  int cache_hits_;    // Cache hits count
  int cache_misses_;  // Cache misses count

  deque<KeyType> dq_;  // To maintain most and least recently used items
  unordered_map<KeyType, ValueType> um_;  // Key-value map
};
}  // namespace resdb
#include "service/contract/executor/paral_sm/data_storage.h"

#include "glog/logging.h"

namespace resdb {
namespace contract {

int64_t DataStorage::Store(const uint256_t& key, const uint256_t& value, bool) {
  std::unique_lock lock(mutex_);
  // LOG(ERROR)<<"store key:"<<key<<" value:"<<value;
  int64_t v = s[key].second;
  s[key] = std::make_pair(value, v + 1);
  return v + 1;
}

std::pair<uint256_t, int64_t> DataStorage::Load(const uint256_t& key,
                                                bool) const {
  std::shared_lock lock(mutex_);
  // LOG(ERROR)<<"load key:"<<key;
  auto e = s.find(key);
  if (e == s.end()) return std::make_pair(0, 0);
  return e->second;
}

bool DataStorage::Remove(const uint256_t& key, bool) {
  std::unique_lock lock(mutex_);
  auto e = s.find(key);
  if (e == s.end()) return false;
  s.erase(e);
  return true;
}

bool DataStorage::Exist(const uint256_t& key, bool) const {
  std::shared_lock lock(mutex_);
  return s.find(key) != s.end();
}

int64_t DataStorage::GetVersion(const uint256_t& key, bool) const {
  LOG(ERROR) << "?????";
  std::shared_lock lock(mutex_);
  auto it = s.find(key);
  if (it == s.end()) {
    return 0;
  }
  return it->second.second;
}

}  // namespace contract
}  // namespace resdb

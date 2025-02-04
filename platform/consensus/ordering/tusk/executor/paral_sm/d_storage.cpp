#include "service/contract/executor/paral_sm/d_storage.h"

#include "glog/logging.h"

namespace resdb {
namespace contract {

namespace {

void InternalReset(const uint256_t& key, const uint256_t& value,
                   int64_t version,
                   std::map<uint256_t, std::pair<uint256_t, int64_t> >* db,
                   std::shared_mutex* mutex) {
  std::unique_lock lock(*mutex);
  (*db)[key] = std::make_pair(value, version);
}

int64_t InternalStore(const uint256_t& key, const uint256_t& value,
                      std::map<uint256_t, std::pair<uint256_t, int64_t> >* db,
                      std::shared_mutex* mutex) {
  std::unique_lock lock(*mutex);
  int64_t v = (*db)[key].second;
  (*db)[key] = std::make_pair(value, v + 1);
  return v + 1;
}

std::pair<uint256_t, int64_t> InternalLoad(
    const uint256_t& key,
    const std::map<uint256_t, std::pair<uint256_t, int64_t> >* db,
    std::shared_mutex* mutex) {
  std::shared_lock lock(*mutex);
  auto e = db->find(key);
  if (e == db->end()) return std::make_pair(0, 0);
  return e->second;
}

bool InternalRemove(const uint256_t& key,
                    std::map<uint256_t, std::pair<uint256_t, int64_t> >* db,
                    std::shared_mutex* mutex) {
  std::unique_lock lock(*mutex);
  auto e = db->find(key);
  if (e == db->end()) return false;
  db->erase(e);
  return true;
}

bool InternalExist(
    const uint256_t& key,
    const std::map<uint256_t, std::pair<uint256_t, int64_t> >* db,
    std::shared_mutex* mutex) {
  std::shared_lock lock(*mutex);
  return db->find(key) != db->end();
}

int64_t InternalGetVersion(
    const uint256_t& key,
    const std::map<uint256_t, std::pair<uint256_t, int64_t> >* db,
    std::shared_mutex* mutex) {
  std::shared_lock lock(*mutex);
  auto it = db->find(key);
  if (it == db->end()) {
    return 0;
  }
  return it->second.second;
}

}  // namespace

int64_t D_Storage::StoreWithVersion(const uint256_t& key,
                                    const uint256_t& value, int version,
                                    bool is_local) {
  // LOG(ERROR)<<"storage key:"<<key<<" value:"<<value<<" ver:"<<version<<" is
  // local:"<<is_local;
  if (is_local) {
    std::unique_lock lock(mutex_);
    c_s_[key] = std::make_pair(value, version);
  } else {
    std::unique_lock lock(g_mutex_);
    g_s_[key] = std::make_pair(value, version);
  }
  return 0;
}

int64_t D_Storage::Store(const uint256_t& key, const uint256_t& value,
                         bool is_to_local_view) {
  // LOG(ERROR)<<"store key:"<<key<<" value:"<<value<<"
  // local:"<<is_to_local_view;
  if (is_to_local_view) {
    return InternalStore(key, value, &c_s_, &mutex_);
  } else {
    return InternalStore(key, value, &g_s_, &g_mutex_);
  }
}

std::pair<uint256_t, int64_t> D_Storage::Load(const uint256_t& key,
                                              bool is_local_view) const {
  // LOG(ERROR)<<"load key:"<<key<<" local:"<<is_local_view;
  if (is_local_view) {
    auto ret = InternalLoad(key, &c_s_, &mutex_);
    if (ret.second == 0) {
      return InternalLoad(key, &g_s_, &mutex_);
    }
    return ret;
  } else {
    return InternalLoad(key, &g_s_, &g_mutex_);
  }
}

bool D_Storage::Remove(const uint256_t& key, bool is_local) {
  if (is_local) {
    return InternalRemove(key, &c_s_, &mutex_);
  } else {
    return InternalRemove(key, &g_s_, &g_mutex_);
  }
}

bool D_Storage::Exist(const uint256_t& key, bool is_local) const {
  if (is_local) {
    return InternalExist(key, &c_s_, &mutex_);
  } else {
    return InternalExist(key, &g_s_, &g_mutex_);
  }
}

int64_t D_Storage::GetVersion(const uint256_t& key, bool is_local) const {
  if (is_local) {
    int ret = InternalGetVersion(key, &c_s_, &mutex_);
    // LOG(ERROR)<<"get local ver:"<<(c_s_.find(key)->second.second)<<"
    // key:"<<key;
    if (ret == 0) {
      return InternalGetVersion(key, &g_s_, &g_mutex_);
    }
    return ret;
  } else {
    return InternalGetVersion(key, &g_s_, &g_mutex_);
  }
}

void D_Storage::Reset(const uint256_t& key, const uint256_t& value,
                      int64_t version, bool is_local) {
  if (is_local) {
    InternalReset(key, value, version, &c_s_, &mutex_);
  } else {
    InternalReset(key, value, version, &g_s_, &g_mutex_);
  }
}

}  // namespace contract
}  // namespace resdb

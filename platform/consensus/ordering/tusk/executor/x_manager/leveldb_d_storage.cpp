#include "service/contract/executor/x_manager/leveldb_d_storage.h"

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
  for (int i = 0; i < sz; ++i) {
    v += bytes[i];
  }
  return v % 1024;
}
void Write(const uint256_t& key, const uint256_t& value, int version,
           ResLevelDB* db) {
  std::string addr = eevm::to_hex_string(key);

  const uint8_t* bytes = intx::as_bytes(value);
  size_t sz = sizeof(value);
  db->SetValue(addr, std::string((const char*)bytes, sz));
  return;
}

uint256_t Read(const uint256_t& key, ResLevelDB* db) {
  std::string addr = eevm::to_hex_string(key);

  std::string v = db->GetValue(addr);
  if (v.empty()) {
    return 0;
  }

  uint8_t tmp[32] = {};
  memcpy(tmp, v.c_str(), 32);
  return intx::le::load<uint256_t>(tmp);
}

void InternalReset(const uint256_t& key, const uint256_t& value,
                   int64_t version,
                   std::map<uint256_t, std::pair<uint256_t, int64_t> >* db,
                   std::shared_mutex* mutex, ResLevelDB* storage) {
  std::unique_lock lock(*mutex);
  (*db)[key] = std::make_pair(value, version);
  if (storage) {
    Write(key, value, version, storage);
  }
}

int64_t InternalStore(const uint256_t& key, const uint256_t& value,
                      std::map<uint256_t, std::pair<uint256_t, int64_t> >* db,
                      std::shared_mutex* mutex, ResLevelDB* storage) {
  std::unique_lock lock(*mutex);
  int64_t v = (*db)[key].second;
  (*db)[key] = std::make_pair(value, v + 1);
  if (storage) {
    Write(key, value, v + 1, storage);
  }
  return v + 1;
}

std::pair<uint256_t, int64_t> InternalLoad(
    const uint256_t& key,
    const std::map<uint256_t, std::pair<uint256_t, int64_t> >* db,
    std::shared_mutex* mutex, ResLevelDB* storage) {
  std::shared_lock lock(*mutex);
  auto e = db->find(key);
  if (e == db->end()) {
    if (storage) {
      return std::make_pair(Read(key, storage), 0);
    }
    return std::make_pair(0, 0);
  }
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

LevelDB_D_Storage ::LevelDB_D_Storage() {
  db_ = std::make_unique<ResLevelDB>("./");
}

int64_t LevelDB_D_Storage::StoreWithVersion(const uint256_t& key,
                                            const uint256_t& value, int version,
                                            bool is_local) {
  // LOG(ERROR)<<"storage key:"<<key<<" value:"<<value<<" ver:"<<version<<" is
  // local:"<<is_local;
  int idx = GetHashKey(key);
  if (is_local) {
    std::unique_lock lock(mutex_[idx]);
    c_s_[idx][key] = std::make_pair(value, version);
  } else {
    std::unique_lock lock(g_mutex_[idx]);
    g_s_[idx][key] = std::make_pair(value, version);
    Write(key, value, version, db_.get());
  }
  return 0;
}

int64_t LevelDB_D_Storage::Store(const uint256_t& key, const uint256_t& value,
                                 bool is_to_local_view) {
  // LOG(ERROR)<<"store key:"<<key<<" value:"<<value<<"
  // local:"<<is_to_local_view;
  int idx = GetHashKey(key);
  if (is_to_local_view) {
    return InternalStore(key, value, &c_s_[idx], &mutex_[idx], nullptr);
  } else {
    return InternalStore(key, value, &g_s_[idx], &g_mutex_[idx], db_.get());
  }
}

std::pair<uint256_t, int64_t> LevelDB_D_Storage::Load(
    const uint256_t& key, bool is_local_view) const {
  // LOG(ERROR)<<"load key:"<<key<<" local:"<<is_local_view;
  int idx = GetHashKey(key);
  if (is_local_view) {
    auto ret = InternalLoad(key, &c_s_[idx], &mutex_[idx], nullptr);
    if (ret.second == 0) {
      return InternalLoad(key, &g_s_[idx], &mutex_[idx], db_.get());
    }
    return ret;
  } else {
    return InternalLoad(key, &g_s_[idx], &g_mutex_[idx], db_.get());
  }
}

bool LevelDB_D_Storage::Remove(const uint256_t& key, bool is_local) {
  int idx = GetHashKey(key);
  if (is_local) {
    return InternalRemove(key, &c_s_[idx], &mutex_[idx]);
  } else {
    return InternalRemove(key, &g_s_[idx], &g_mutex_[idx]);
  }
}

bool LevelDB_D_Storage::Exist(const uint256_t& key, bool is_local) const {
  int idx = GetHashKey(key);
  if (is_local) {
    return InternalExist(key, &c_s_[idx], &mutex_[idx]);
  } else {
    return InternalExist(key, &g_s_[idx], &g_mutex_[idx]);
  }
}

int64_t LevelDB_D_Storage::GetVersion(const uint256_t& key,
                                      bool is_local) const {
  int idx = GetHashKey(key);
  if (is_local) {
    int ret = InternalGetVersion(key, &c_s_[idx], &mutex_[idx]);
    // LOG(ERROR)<<"get local ver:"<<(c_s_.find(key)->second.second)<<"
    // key:"<<key;
    if (ret == 0) {
      return InternalGetVersion(key, &g_s_[idx], &g_mutex_[idx]);
    }
    return ret;
  } else {
    return InternalGetVersion(key, &g_s_[idx], &g_mutex_[idx]);
  }
}

void LevelDB_D_Storage::Reset(const uint256_t& key, const uint256_t& value,
                              int64_t version, bool is_local) {
  int idx = GetHashKey(key);
  if (is_local) {
    InternalReset(key, value, version, &c_s_[idx], &mutex_[idx], nullptr);
  } else {
    InternalReset(key, value, version, &g_s_[idx], &g_mutex_[idx], db_.get());
  }
}

}  // namespace contract
}  // namespace resdb

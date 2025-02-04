#include "service/contract/executor/manager/level_d_storage.h"

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
  // LOG(ERROR)<<"store key:"<<key<<" v:"<<v+1;
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
    // LOG(ERROR)<<"get version key:"<<key<<" v:"<<0;
    return 0;
  }
  // LOG(ERROR)<<"get version key:"<<key<<" v:"<<it->second.second;
  return it->second.second;
}

std::string GetString(const uint256_t& key) { return ""; }

uint256_t GetInt256(const std::string& value) { return 0; }

std::string GetData(const uint256_t& value, int64_t version) {
  std::string v_str = GetString(value);
  std::string v((const char*)&version, sizeof(value));
  v.append(v_str, v_str.size());
  return v;
}

int64_t GetVersion(const std::string& value) {
  int64_t v;
  memcpy(&v, value.c_str(), sizeof(v));
  return v;
}

uint256_t GetValue(const std::string& value) {
  return GetInt256(std::string(value.c_str() + sizeof(int64_t),
                               value.size() - sizeof(int64_t)));
}

int64_t InternalStore(const uint256_t& key, const uint256_t& value,
                      ResLevelDB* db, std::shared_mutex* mutex) {
  std::unique_lock lock(*mutex);
  std::string old_value = db->GetValue(GetString(key));
  int64_t v = GetVersion(old_value);
  db->SetValue(GetString(key), GetData(value, v + 1));
  // LOG(ERROR)<<"store key:"<<key<<" v:"<<v+1;
  return v + 1;
}

std::pair<uint256_t, int64_t> InternalLoad(const uint256_t& key, ResLevelDB* db,
                                           std::shared_mutex* mutex) {
  std::shared_lock lock(*mutex);
  std::string value = db->GetValue(GetString(key));
  return std::make_pair(GetValue(value), GetVersion(value));
}

bool InternalRemove(const uint256_t& key, ResLevelDB* db,
                    std::shared_mutex* mutex) {
  std::unique_lock lock(*mutex);
  db->SetValue(GetString(key), "");
  return true;
}

bool InternalExist(const uint256_t& key, ResLevelDB* db,
                   std::shared_mutex* mutex) {
  std::shared_lock lock(*mutex);
  std::string value = db->GetValue(GetString(key));
  return value.empty();
}

void InternalReset(const uint256_t& key, const uint256_t& value,
                   int64_t version, ResLevelDB* db, std::shared_mutex* mutex) {
  std::unique_lock lock(*mutex);
  db->SetValue(GetString(key), GetData(value, version));
}

int64_t InternalGetVersion(const uint256_t& key, ResLevelDB* db,
                           std::shared_mutex* mutex) {
  std::shared_lock lock(*mutex);
  std::string value = db->GetValue(GetString(key));
  return GetVersion(value);
}

}  // namespace

int64_t D_Storage::Store(const uint256_t& key, const uint256_t& value,
                         bool is_to_local_view) {
  // LOG(ERROR)<<"store key:"<<key<<" value:"<<value<<"
  // local:"<<is_to_local_view;
  if (is_to_local_view) {
    return InternalStore(key, value, &c_s_, &mutex_);
  } else {
    return InternalStore(key, value, g_s_.get(), &g_mutex_);
  }
}

std::pair<uint256_t, int64_t> D_Storage::Load(const uint256_t& key,
                                              bool is_local_view) const {
  // LOG(ERROR)<<"load key:"<<key<<" local:"<<is_local_view;
  if (is_local_view) {
    auto ret = InternalLoad(key, &c_s_, &mutex_);
    if (ret.second == 0) {
      // LOG(ERROR)<<"get from global:";
      return InternalLoad(key, g_s_.get(), &mutex_);
    }
    return ret;
  } else {
    return InternalLoad(key, g_s_.get(), &g_mutex_);
  }
}

bool D_Storage::Remove(const uint256_t& key, bool is_local) {
  if (is_local) {
    return InternalRemove(key, &c_s_, &mutex_);
  } else {
    return InternalRemove(key, g_s_.get(), &g_mutex_);
  }
}

bool D_Storage::Exist(const uint256_t& key, bool is_local) const {
  if (is_local) {
    return InternalExist(key, &c_s_, &mutex_);
  } else {
    return InternalExist(key, g_s_.get(), &g_mutex_);
  }
}

int64_t D_Storage::GetVersion(const uint256_t& key, bool is_local) const {
  // LOG(ERROR)<<"get version key:"<<key<<" islocal:"<<is_local;
  if (is_local) {
    int ret = InternalGetVersion(key, &c_s_, &mutex_);
    if (ret == 0) {
      ret = InternalGetVersion(key, g_s_.get(), &g_mutex_);
      // LOG(ERROR)<<"get from global:"<<ret;
    }
    // LOG(ERROR)<<"get version key:"<<key<<" islocal:"<<is_local<<" v:"<<ret;
    return ret;
  } else {
    return InternalGetVersion(key, g_s_.get(), &g_mutex_);
  }
}

void D_Storage::Reset(const uint256_t& key, const uint256_t& value,
                      int64_t version, bool is_local) {
  // LOG(ERROR)<<"set key:"<<key<<" value:"<<value<<" local:"<<is_local<<"
  // version:"<<version;
  if (is_local) {
    InternalReset(key, value, version, &c_s_, &mutex_);
  } else {
    InternalReset(key, value, version, g_s_.get(), &g_mutex_);
  }
}

}  // namespace contract
}  // namespace resdb

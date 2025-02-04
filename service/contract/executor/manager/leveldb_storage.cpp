#include "service/contract/executor/manager/leveldb_storage.h"

#include "glog/logging.h"

namespace resdb {
namespace contract {

namespace {

int GetHashKey(const uint256_t& address) {
  return 0;
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
  return v % 2048;
}
}  // namespace

LevelDBStorage ::LevelDBStorage() {
  db_ = std::make_unique<ResLevelDB>("./", std::nullopt);
}

void LevelDBStorage::Write(const uint256_t& key, const uint256_t& value,
                           int version) {
  std::string addr = eevm::to_hex_string(key);

  const uint8_t* bytes = intx::as_bytes(value);
  size_t sz = sizeof(value);
  db_->SetValue(addr, std::string((const char*)bytes, sz));
  return;

  // LOG(ERROR)<<"addr:"<<addr<<" value:"<<value;
  char* buf = new char[sz + sizeof(version)];
  memcpy(buf, &version, sizeof(version));
  memcpy(buf + sizeof(value), bytes, sz);

  db_->SetValue(addr, std::string(buf, sz + sizeof(version)));
  LOG(ERROR) << "write db";
  delete buf;
}

uint256_t LevelDBStorage::Read(const uint256_t& key) const {
  std::string addr = eevm::to_hex_string(key);

  std::string v = db_->GetValue(addr);
  if (v.empty()) {
    return 0;
  }

  uint8_t tmp[32] = {};
  memcpy(tmp, v.c_str(), 32);
  return intx::le::load<uint256_t>(tmp);
}

int64_t LevelDBStorage::Store(const uint256_t& key, const uint256_t& value,
                              bool) {
  int idx = GetHashKey(key);
  std::unique_lock lock(mutex_[idx]);
  // LOG(ERROR)<<"store key:"<<key<<" value:"<<value;
  int64_t v = s[idx][key].second;
  s[idx][key] = std::make_pair(value, v + 1);
  Write(key, value, v + 1);
  return v + 1;
}

std::pair<uint256_t, int64_t> LevelDBStorage::Load(const uint256_t& key,
                                                   bool) const {
  int idx = GetHashKey(key);
  std::shared_lock lock(mutex_[idx]);
  // LOG(ERROR)<<"load key:"<<key;
  auto e = s[idx].find(key);
  if (e == s[idx].end()) {
    uint256_t v = Read(key);
    return std::make_pair(v, 0);
  }
  return e->second;
}

bool LevelDBStorage::Remove(const uint256_t& key, bool) {
  int idx = GetHashKey(key);
  std::unique_lock lock(mutex_[idx]);
  auto e = s[idx].find(key);
  if (e == s[idx].end()) return false;
  s[idx].erase(e);
  return true;
}

bool LevelDBStorage::Exist(const uint256_t& key, bool) const {
  int idx = GetHashKey(key);
  std::shared_lock lock(mutex_[idx]);
  return s[idx].find(key) != s[idx].end();
}

int64_t LevelDBStorage::GetVersion(const uint256_t& key, bool) const {
  int idx = GetHashKey(key);
  std::shared_lock lock(mutex_[idx]);
  auto it = s[idx].find(key);
  if (it == s[idx].end()) {
    return 0;
  }
  return it->second.second;
}

}  // namespace contract
}  // namespace resdb

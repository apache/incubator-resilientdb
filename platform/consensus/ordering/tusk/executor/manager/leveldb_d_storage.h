#pragma once

#include <map>
#include <shared_mutex>

#include "service/contract/executor/manager/data_storage.h"
#include "storage/res_leveldb.h"

namespace resdb {
namespace contract {

class LevelDB_D_Storage : public DataStorage {
public:
  LevelDB_D_Storage();

public:
  virtual int64_t Store(const uint256_t& key, const uint256_t& value, bool is_local);
  virtual std::pair<uint256_t,int64_t> Load(const uint256_t& key, bool is_from_local_view) const;
  virtual bool Remove(const uint256_t& key, bool is_local);
  virtual bool Exist(const uint256_t& key, bool is_local) const;

  virtual int64_t GetVersion(const uint256_t& key, bool is_local) const;

  virtual void Reset(const uint256_t& key, const uint256_t& value, int64_t version, bool is_local); 

protected:
  std::map<uint256_t, std::pair<uint256_t,int64_t> > c_s_[1024];
  mutable std::shared_mutex mutex_[1024];

  std::map<uint256_t, std::pair<uint256_t,int64_t> > g_s_[1024];
  mutable std::shared_mutex g_mutex_[1024];

  std::unique_ptr<ResLevelDB> db_;
};

}
} // namespace resdb

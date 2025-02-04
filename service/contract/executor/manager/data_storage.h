#pragma once

#include <map>
#include <shared_mutex>

#include "eEVM/util.h"

namespace resdb {
namespace contract {

class DataStorage {
 public:
  DataStorage() = default;
  virtual ~DataStorage() = default;

  virtual int64_t Store(const uint256_t& key, const uint256_t& value,
                        bool is_local = false);
  virtual std::pair<uint256_t, int64_t> Load(const uint256_t& key,
                                             bool is_local_view = false) const;
  virtual bool Remove(const uint256_t& key, bool is_local = false);
  virtual bool Exist(const uint256_t& key, bool is_local = false) const;

  virtual int64_t GetVersion(const uint256_t& key, bool is_local = false) const;

  virtual void Reset(const uint256_t& key, const uint256_t& value,
                     int64_t version, bool is_local = false) {}
  virtual void Flush(){};

 protected:
  std::map<uint256_t, std::pair<uint256_t, int64_t> > s;
  // std::map<uint256_t, std::pair<uint256_t,int64_t> > s[4096];
  mutable std::shared_mutex mutex_[4096];
};

}  // namespace contract
}  // namespace resdb

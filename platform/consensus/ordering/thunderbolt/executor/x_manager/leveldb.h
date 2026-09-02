#pragma once

#include <map>
#include <shared_mutex>

#include "eEVM/util.h"
#include "service/contract/executor/manager/data_storage.h"
#include "storage/res_leveldb.h"

namespace resdb {
namespace contract {

class LevelDB : public DataStorage {
 public:
  LevelDB();
  virtual ~LevelDB() = default;

  virtual void Flush();

 private:
  std::unique_ptr<ResLevelDB> db_;
};

}  // namespace contract
}  // namespace resdb

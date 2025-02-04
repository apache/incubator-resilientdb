#pragma once

#include <map>
#include <shared_mutex>

#include "service/contract/executor/common/contract_execute_info.h"
#include "service/contract/executor/x_manager/data_storage.h"

namespace resdb {
namespace contract {
namespace x_manager {

class ConcurrencyController {
 public:
  ConcurrencyController(DataStorage* storage);

  virtual void PushCommit(int64_t commit_id,
                          const ModifyMap& local_changes_) = 0;

  const DataStorage* GetStorage() const;

 protected:
  DataStorage* storage_;
};

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb

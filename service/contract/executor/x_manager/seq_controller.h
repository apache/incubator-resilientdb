#pragma once

#include <bitset>
#include <deque>
#include <set>
#include <shared_mutex>
#include <thread>
#include <unordered_set>

#include "platform/common/queue/lock_free_queue.h"
#include "service/contract/executor/x_manager/concurrency_controller.h"
#include "service/contract/executor/x_manager/data_storage.h"

namespace resdb {
namespace contract {
namespace x_manager {

class SeqController : public ConcurrencyController {
 public:
  SeqController(DataStorage* storage, int window_size);
  virtual ~SeqController();

  // ==============================
  virtual void Store(const int64_t commit_id, const uint256_t& key,
                     const uint256_t& value, int version);
  virtual uint256_t Load(const int64_t commit_id, const uint256_t& key,
                         int version);

  virtual void PushCommit(int64_t commit_id, const ModifyMap& local_changes_) {}

  void Clear();

  const ModifyMap& GetChangeList(int64_t commit_id);

 private:
  DataStorage* storage_;
  std::vector<ModifyMap> changes_list_;
  int window_size_;
};

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb

#pragma once

#include <deque>
#include <set>
#include <shared_mutex>
#include <thread>

#include "platform/common/queue/lock_free_queue.h"
#include "service/contract/executor/manager/concurrency_controller.h"

namespace resdb {
namespace contract {

class VController : public ConcurrencyController {
 public:
  VController(DataStorage* storage);
  ~VController();

  virtual void PushCommit(int64_t commit_id, const ModifyMap& local_changes_);
  bool Commit(int64_t commit_id);
  const ModifyMap* GetChangeList(int64_t commit_id) const;

 private:
  bool CheckCommit(int64_t commit_id);

 private:
  const int window_size_ = 2000;
  std::vector<ModifyMap> changes_list_;
};

}  // namespace contract
}  // namespace resdb

#pragma once

#include "service/contract/executor/manager/concurrency_controller.h"

#include <map>
#include <shared_mutex>

namespace resdb {
namespace contract {

class TwoPhaseOOOController : public ConcurrencyController {
  public:
    TwoPhaseOOOController(DataStorage * storage);

    virtual void PushCommit(int64_t commit_id, const ModifyMap& local_changes_);

    bool Commit(int64_t commit_id);

    // Before each 2PL, make sure clear the data from the previous round.
    void Clear();

  private:
    bool CheckCommit(int64_t commit_id);

  protected:
    mutable std::shared_mutex mutex_;
    std::vector<ModifyMap> changes_list_;
    std::map<uint256_t, int64_t> first_commit_;
    const int window_size_ = 1000;
};

}
} // namespace resdb

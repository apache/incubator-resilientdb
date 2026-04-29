#pragma once

#include "service/contract/executor/manager/concurrency_controller.h"

#include <map>
#include <shared_mutex>

namespace resdb {
namespace contract {

class TestController : public ConcurrencyController {
  public:
    TestController(DataStorage * storage);

    virtual void PushCommit(int64_t commit_id, const ModifyMap& local_changes_);
};

}
} // namespace resdb

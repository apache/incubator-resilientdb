#pragma once

#include "service/contract/executor/manager/concurrency_controller.h"

namespace resdb {
namespace contract {

class OOOController : public ConcurrencyController {
  public:
    OOOController(DataStorage * storage);
    ~OOOController();

    virtual void PushCommit(int64_t commit_id, const ModifyMap& local_changes);
};

}
} // namespace resdb

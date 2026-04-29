#include "service/contract/executor/manager/ooo_controller.h"

#include <glog/logging.h>

namespace resdb {
namespace contract {

OOOController::OOOController(DataStorage * storage) : ConcurrencyController(storage){
}

OOOController::~OOOController(){}

void OOOController::PushCommit(int64_t commit_id, const ModifyMap& local_changes) {
  for(const auto& it : local_changes){
    bool done = false;
    for(int i = it.second.size()-1; i >=0 && !done;--i){
      const auto& op = it.second[i];
      switch(op.state){
        case LOAD:
          break;
        case STORE:
          storage_->Store(it.first, op.data);
          done = true;
          break;
        case REMOVE:
          storage_->Remove(it.first);
          done = true;
          break;
      }
    }
  }
}

}
} // namespace eevm

#include "service/contract/executor/manager/test_controller.h"

#include <glog/logging.h>

namespace resdb {
namespace contract {

TestController::TestController(DataStorage * storage) : ConcurrencyController(storage){}

void TestController::PushCommit(int64_t commit_id, const ModifyMap& local_changes) {
  for(auto it : local_changes){
    bool done = false;
    for(int i = it.second.size()-1; i >=0 && !done;--i){
      const auto& op = it.second[i];
      switch(op.state){
        case LOAD:
          break;
        case STORE:
          //LOG(ERROR)<<"commit:"<<it.first<<" data:"<<op.data<<" commit id:"<<commit_id;
          storage_->Store(it.first, op.data);
          done = true;
          break;
        case REMOVE:
          //LOG(ERROR)<<"remove:"<<it.first<<" data:";
          storage_->Remove(it.first);
          done = true;
          break;
      }
    }
  }
  return ;
}

}
} // namespace eevm

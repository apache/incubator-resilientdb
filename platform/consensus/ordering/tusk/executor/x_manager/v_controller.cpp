#include "service/contract/executor/x_manager/v_controller.h"

#include <glog/logging.h>

namespace resdb {
namespace contract {
namespace x_manager {

VController::VController(DataStorage * storage) : ConcurrencyController(storage){

    changes_list_.resize(window_size_);
    for(int i = 0; i < window_size_; ++i){
      changes_list_[i].clear();
    }
}

VController::~VController(){}

const ConcurrencyController::ModifyMap * VController::GetChangeList(int64_t commit_id) const {
  return &changes_list_[commit_id];
}

void VController::PushCommit(int64_t commit_id, const ModifyMap& local_changes) {
  changes_list_[commit_id] = local_changes;
}

bool VController::CheckCommit(int64_t commit_id) {
  const auto& change_set = changes_list_[commit_id];
  if(change_set.empty()){
    LOG(ERROR)<<" no commit id record found:"<<commit_id;
    //assert(!change_set.empty());
    return false;
  }

  for(const auto& it : change_set){
    const uint256_t& address = it.first;
    for(auto& op : it.second){
      if(op.state == LOAD){
        uint64_t v = storage_->GetVersion(address);
        //LOG(ERROR)<<"get version:"<<v<<" op version:"<<op.version;
        if(op.version != v){
          return false;
        }
      }
    }
  }
  return true;
}

bool VController::Commit(int64_t commit_id){
  if(!CheckCommit(commit_id)){
    return false;
  }

  const auto& change_set = changes_list_[commit_id];
  if(change_set.empty()){
    LOG(ERROR)<<" no commit id record found:"<<commit_id;
    return false;
  }
  std::set<int64_t> new_commit_ids;
  for(const auto& it : change_set){
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
  return true;

}

}
}
} // namespace eevm

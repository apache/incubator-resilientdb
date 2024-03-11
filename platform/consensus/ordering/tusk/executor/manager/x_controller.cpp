#include "service/contract/executor/manager/x_controller.h"

#include <glog/logging.h>

namespace resdb {
namespace contract {

XController::XController(DataStorage * storage) : ConcurrencyController(storage){

  for(int i = 0; i < window_size_; ++i){
    is_redo_.push_back(false);
  }

  Clear();
  last_commit_id_ = 0;
}

XController::~XController(){}

void XController::Clear(){
		last_commit_id_=0;
  
    changes_list_.resize(window_size_);
    pending_list_.resize(window_size_);
    for(int i = 0; i < window_size_; ++i){
      changes_list_[i].clear();
      pending_list_[i].clear();
      is_redo_[i] = false;
    }
    commit_list_.clear();
    m_list_.clear();
}

void XController::UseOCC(){
  use_occ_ = true;
}

std::vector<int64_t>& XController::GetRedo(){
  return redo_;
}

int XController::GetHashKey(const uint256_t& address){
  // Get big-endian form
  const uint8_t* bytes = intx::as_bytes(address);
      //uint8_t arr[32] = {};
      //memset(arr,0,sizeof(arr));
      //intx::be::store(arr, address);
      //const uint8_t* bytes = arr;
      size_t sz = sizeof(address);
      int v = 0;
      for(int i = 0; i < sz; ++i){
        v += bytes[i];
      }
  return v%1;
  //return v%128;
}

void XController::SetCallback(CallBack call_back) {
  call_back_ = call_back;
}

void XController::RedoCommit(int64_t commit_id) {
  if(is_redo_[commit_id]){
    return;
  }
  is_redo_[commit_id] = true;
  //LOG(ERROR)<<" add redo:"<<commit_id;
  redo_.push_back(commit_id);
}

void XController::CheckRedo(){
return;
  //LOG(ERROR)<<"check redo:"<<redo_list_.size();
  std::set<int64_t> list;
  for(int64_t id : redo_list_){
    if(use_occ_){
      list.insert(id);
      continue;
    }
    const auto& change_set = changes_list_[id];
    if(change_set.empty()){
      //LOG(ERROR)<<" no commit id record found:"<<id;
      assert(1==0);
      list.insert(id);
      continue;
    }

    bool ok = 1;
    for(const auto& it : change_set){
      const uint256_t& address = it.first;
      //LOG(ERROR)<<"address:"<<address<<" num:"<<ref_[address];
      if(ref_[address] > 0){
        ok = 0;
        break;
      }
    }
    if(ok){
      list.insert(id);

      //LOG(ERROR)<<"redo id:"<<id;
      for(const auto& it : change_set){
        const uint256_t& address = it.first;
        ref_[address]++;
      }
    }
  }
  //LOG(ERROR)<<"list size:"<<list.size();
  for(int64_t id : list){
    redo_list_.erase(redo_list_.find(id));
    RedoCommit(id);
  }
}

void XController::AddRedo(int64_t commit_id) {
RedoCommit(commit_id);

/*
  if(is_redo_[commit_id]){
    return;
  }
  redo_list_.insert(commit_id);
  */
  //LOG(ERROR)<<"add redo:"<<commit_id;
}

void XController::PushCommit(int64_t commit_id, const ModifyMap& local_changes) {
  //std::unique_lock lock(mutex_);
  //LOG(ERROR)<<"push commit:"<<commit_id;
  changes_list_[commit_id] = local_changes;
  //pending_list_[commit_id] = local_changes;
}

bool XController::Commit(int64_t commit_id){
  //std::unique_lock lock(mutex_);
  redo_.clear();
  //changes_list_[commit_id] = pending_list_[commit_id];
  is_redo_[commit_id]=false;
  bool ret = CommitInternal(commit_id);
  //CheckRedo();
  //LOG(ERROR)<<"commit ret:"<<ret;
  return ret;
}

bool XController::CommitInternal(int64_t commit_id){
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
          {
            storage_->Store(it.first, op.data);
            done = true;
            break;
          }
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

bool XController::CheckCommit(int64_t commit_id) {
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
        //LOG(ERROR)<<"get version:"<<v<<" op version:"<<op.version<<" commit id:"<<commit_id;
        if(op.version != v){
          //LOG(ERROR)<<"get version:"<<v<<" op version:"<<op.version<<" commit id:"<<commit_id;
          AddRedo(commit_id);
          return false;
        }
     }
    }
  }
  return true;
}


const ConcurrencyController::ModifyMap * XController::GetChangeList(int64_t commit_id){
  // read lock
  //LOG(ERROR)<<"get change list:"<<changes_list_[commit_id].size();
  return &changes_list_[commit_id];
}

void XController::Remove(int64_t commit_id){
  changes_list_[commit_id].clear();
}

/*
int64_t XController::Remove(const uint256_t& address, int64_t commit_id, uint64_t v){
      int idx = GetHashKey(address);
  //std::unique_lock lock(mutex_);
  //std::unique_lock lock(mutexs_[idx]);
  //LOG(ERROR)<<"load idx:"<<idx;
  if(commit_list_.find(address) == commit_list_.end()){
    LOG(ERROR)<<" remove address:"<<address<<" commit id:"<<commit_id<<" not exist";
  }
  //assert(commit_list_.find(address) != commit_list_.end());

  auto& it = commit_list_[address];
  //assert(*it.begin() == commit_id);
  it.pop_front();
  //it.erase(it.begin());
  //LOG(ERROR)<<"update address:"<<address<<" mlist:"<<m_list_[address];
  if(it.empty()) {
    commit_list_.erase(commit_list_.find(address));
    //commit_list_[idx].erase(commit_list_[idx].find(address));
    return -1;
  }
  return it.front();
}
*/


}
} // namespace eevm

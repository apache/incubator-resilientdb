#include "service/contract/executor/paral_sm/xcontroller.h"

#include <glog/logging.h>
#include <stack>
#include <queue>

namespace resdb {
namespace contract {
namespace paral_sm {

namespace {

    int GetHashKey(const uint256_t& address){
      // Get big-endian form
      uint8_t arr[32] = {};
      memset(arr,0,sizeof(arr));
      intx::be::store(arr, address);
      uint32_t v = 0;
      for(int i = 0; i < 32; ++i){
        v += arr[i];
      }
      return v%128;
    }

    bool CheckFirstCommit(const uint256_t& address, int64_t commit_id, 
        const XController::CommitList *commit_list, bool is_pre){
      int idx = GetHashKey(address);
      const auto& it = commit_list[idx].find(address);
      if(it == commit_list[idx].end()){
        if(is_pre){
          LOG(ERROR)<<"no address:"<<commit_id<<" add:"<<address;
          assert(1==0);
        }
        return false;
      }
      const auto& commit_set  = it->second;
      if(commit_set.empty()){
        LOG(ERROR)<<"commit set is empty:"<<commit_id<<" is pre:"<<is_pre<<" addr:"<<address;
      }
      assert(!commit_set.empty());

      if(commit_set.front() < commit_id){
        //if(*commit_set.begin() < commit_id){
        // not the first candidate
        //LOG(ERROR)<<"still have dep. commit id:"<<commit_id<<" dep:"<<*commit_list_[address].begin();
        return false;
      }
      if(*commit_set.begin() > commit_id){
        LOG(ERROR)<<"set header:"<<*commit_set.begin()<<" current:"<<commit_id<<" is pre:"<<is_pre;
        if(is_pre==0){
          return false;
        }
      }
      assert(*commit_set.begin() == commit_id);
      return true;
    }

    void AppendRecord(const uint256_t& address, 
        int64_t commit_id, XController::CommitList * commit_list) {
      int hash_idx = GetHashKey(address);
      auto& commit_set = commit_list[hash_idx][address];
      if(!commit_set.empty()&& commit_set.back() > commit_id){
        LOG(ERROR)<<"append to old data:"<<commit_id<<" back:"<<commit_set.back();
      }
      assert(commit_set.empty() || commit_set.back() < commit_id);
      commit_set.push_back(commit_id);
      return;
    }

    void AppendHeadRecord(const uint256_t& address, int hash_idx, 
          const std::vector<int64_t>& commit_id, XController::CommitList * commit_list) {
          auto& commit_set = commit_list[hash_idx][address];
          for(auto id : commit_id){
            //LOG(ERROR)<<"push front id:"<<id<<" idx:"<<hash_idx<<" address:"<<address;
            assert(commit_set.empty() || id < commit_set.front());
            commit_set.push_front(id);
          }
    }

    int64_t RemoveRecord(const uint256_t& address, int64_t commit_id, XController::CommitList * commit_list) {
      int idx = GetHashKey(address);
      auto it = commit_list[idx].find(address);
      if(it == commit_list[idx].end()){
        LOG(ERROR)<<" remove address:"<<address<<" commit id:"<<commit_id<<" not exist";
        return -3;
      }

      auto& set_it = commit_list[idx][address];
      if(set_it.front() != commit_id){
        LOG(ERROR)<< "address id::"<<commit_id<<" front:"<<set_it.front();
        return -2;
      }
      //LOG(ERROR)<< "address id::"<<commit_id<<" address:"<<address;

      set_it.pop_front();
      if(set_it.empty()) {
        //LOG(ERROR)<<"erase idx:"<<idx<<" address:"<<address;
        commit_list[idx].erase(it);
        return -1;
      }
      return set_it.front();
    }

    int64_t RemoveFromRecord(const uint256_t& address, int64_t commit_id, XController::CommitList * commit_list) {
      int idx = GetHashKey(address);
      auto it = commit_list[idx].find(address);
      if(it == commit_list[idx].end()){
        LOG(ERROR)<<" remove address:"<<address<<" commit id:"<<commit_id<<" not exist";
        return -3;
      }

      auto& set_it = it->second;
      std::stack<int> tmp;
      while(!set_it.empty()){
        if(set_it.front() == commit_id){
          break;
        }
        tmp.push(set_it.front());
        set_it.pop_front();
      }
      set_it.pop_front();
      while(!tmp.empty()){
        set_it.push_front(tmp.top());
        tmp.pop();
      }

      if(set_it.empty()) {
        //LOG(ERROR)<<"erase idx:"<<idx<<" address:"<<address;
        commit_list[idx].erase(it);
        return -1;
      }
      return set_it.front();
    }


}

XController::XController(DataStorage * storage, int window_size) : 
    ConcurrencyController(storage), window_size_(window_size), storage_(static_cast<D_Storage*>(storage)){
    for(int i = 0; i < 32; i++){
      if((1<<i) >window_size_){
        window_size_ = (1<<i)-1;
        break;
      }
    }
    //LOG(ERROR)<<"init window size:"<<window_size_;
  Clear();
}

XController::~XController(){}

void XController::Clear(){
		last_commit_id_=1;
    current_commit_id_=1;
    last_pre_commit_id_=0;

    assert(window_size_+1<=1024);
    //is_redo_.resize(window_size_+1);
    //is_done_.resize(window_size_+1);
    changes_list_.resize(window_size_+1);
    rechanges_list_.resize(window_size_+1);

    for(int i = 0; i < window_size_; ++i){
      changes_list_[i].clear();
      rechanges_list_[i].clear();
      is_redo_[i] = 0;
      //is_done_[i] = false;
    }

    for(int i = 0; i < 1024; ++i){
      commit_list_[i].clear();
      pre_commit_list_[i].clear();
    }
}

std::vector<int64_t>& XController::GetRedo(){
  return redo_;
}

std::vector<int64_t>& XController::GetDone(){
  return done_;
}

void XController::RedoCommit(int64_t commit_id, int flag) {
  int idx = commit_id&window_size_;

  if(is_redo_[idx]==1){
    LOG(ERROR)<<"commit id has been redo:"<<commit_id;
    return;
  }

  if(is_redo_[idx]==flag+1){
    LOG(ERROR)<<"commit id has been redo:"<<commit_id<<" flag:"<<flag;
    return;
  }

  is_redo_[commit_id&window_size_] = flag+1;
  //LOG(ERROR)<<"add redo:"<<commit_id<<"flag:"<<flag;
  redo_.push_back(commit_id);
}

void XController::CommitDone(int64_t commit_id) {
  //LOG(ERROR)<<"commit done:"<<commit_id;
  /*
  int idx = commit_id&window_size_;
  if(is_done_[idx]){
    LOG(ERROR)<<"commit id has been done:"<<commit_id<<idx;
    assert(1==0);
    return;
  }
  LOG(ERROR)<<"done:"<<commit_id<<" idx:"<<idx;
  is_done_[idx] = true;
  */
  done_.push_back(commit_id);
}


void XController::PushCommit(int64_t commit_id, const ModifyMap& local_changes) {
  int idx = commit_id&window_size_;
  
  //LOG(ERROR)<<"push commit:"<<commit_id<<" idx:"<<idx<<" changes_list_:"<<changes_list_[idx].empty()<<" local change:"<<local_changes.empty();
  if(!changes_list_[idx].empty()){
    rechanges_list_[idx] = local_changes;
  }
  else {
    changes_list_[idx] = local_changes;
  }
  //is_redo_[idx] = 0;
  //is_done_[idx] =false;
  //LOG(ERROR)<<"push commit:"<<commit_id<<" idx:"<<idx;
}

bool XController::CheckPreCommit(int64_t commit_id) {
  //LOG(ERROR)<<"check pre commit:"<<commit_id;
  const auto& change_set = changes_list_[commit_id&window_size_];
  if(change_set.empty()){
    LOG(ERROR)<<" no commit id record found:"<<commit_id;
    assert(!change_set.empty());
    return false;
  }

  for(const auto& it : change_set){
    const uint256_t& address = it.first;
    int hash_idx = GetHashKey(address);
    if(commit_list_[hash_idx].find(address) == commit_list_[hash_idx].end()){
      continue;
    }
    auto& commit_set = commit_list_[hash_idx][address];
    if(!commit_set.empty()){
      //LOG(ERROR)<<"address front:"<<commit_set.front()<<" back:"<<commit_set.back();
    }
    if(!commit_set.empty() && commit_set.back() > commit_id){
      return false;
    }
  }
  return true;
}


bool XController::CheckCommit(int64_t commit_id, const CommitList * commit_list,
    bool is_pre) {
  const auto& change_set = changes_list_[commit_id&window_size_];
  if(change_set.empty()){
    LOG(ERROR)<<" no commit id record found:"<<commit_id;
    assert(!change_set.empty());
    return false;
  }

  for(const auto& it : change_set){
    if(!CheckFirstCommit(it.first, commit_id, commit_list, is_pre)){
      //LOG(ERROR)<<"not the first one:"<<commit_id;
      return false;
    }
  }

  for(const auto& it : change_set){
    for(auto& op : it.second){
      if(op.state == LOAD){
        uint64_t v = storage_->GetVersion(it.first, is_pre);
        //LOG(ERROR)<<"op log:"<<op.version<<" db ver:"<<v<<" is pre:"<<is_pre<<" address:"<<it.first;
        if(op.version != v){
          //LOG(ERROR)<<"op log:"<<op.version<<" db ver:"<<v<<" is pre:"<<is_pre<<" address:"<<it.first<<" check fail";
          RedoCommit(commit_id, 0);
          return false;
        }
      }
    }
  }
  return true;
}

void XController::Remove(int64_t commit_id){
  //LOG(ERROR)<<"remove:"<<commit_id;
  int idx = commit_id&window_size_;
  changes_list_[idx].clear();
  rechanges_list_[idx].clear();
  is_redo_[idx]=0;
}

void XController::RollBackData(const uint256_t& address, const Data& data){
  //LOG(ERROR)<<" roll back:"<<address<<" version:"<<data.version<<" value:"<<data.data<<" old value:"<<data.old_data;
  if(data.state == LOAD){
    storage_->Reset(address, data.data, data.version, /*is_pre=*/true);
  }else {
    storage_->Reset(address, data.old_data, data.version, /*is_pre=*/true);
  }
}

// move id from commitlist back to precommitlist
void XController::RedoConflict(int64_t commit_id) {
  //LOG(ERROR)<<"move conflict id:"<<commit_id;

  std::priority_queue<int64_t, std::vector<int64_t>, std::greater<int64_t>>q;
  std::set<int64_t> v;
  q.push(commit_id);
  v.insert(commit_id);

  while(!q.empty()){
    int64_t cur_id = q.top();
    q.pop();
    const auto& change_set = changes_list_[cur_id&window_size_];

    for(const auto& it : change_set){
      const uint256_t& address = it.first;
      int hash_idx = GetHashKey(address);
      if(commit_list_[hash_idx].find(address) == commit_list_[hash_idx].end()){
        continue;
      }
      //LOG(ERROR)<<"check commit id:"<<cur_id<<" data version:"<<it.second.begin()->version;
      auto& commit_set = commit_list_[hash_idx][address];

      if(cur_id == commit_id){
        auto& precommit_set = pre_commit_list_[hash_idx][address];
        assert(*precommit_set.begin() == commit_id); 
        precommit_set.pop_front();
      }

      std::vector<int64_t> back_list;
      while(!commit_set.empty() && commit_set.back() >= cur_id){
        int64_t back_id = commit_set.back();
        commit_set.pop_back();

        back_list.push_back(back_id);
        if(v.find(back_id) == v.end()){
          q.push(back_id);
          v.insert(back_id);
        }
        if(back_id == cur_id) {
          break;
        }
      }
      //LOG(ERROR)<<"hahs list size:"<<back_list.size()<<" commit set size:"<<commit_set.size();
      if(commit_set.empty()){
        commit_list_[hash_idx].erase(commit_list_[hash_idx].find(address));
      }

      if(!back_list.empty()){
        //LOG(ERROR)<<"roll back from:"<<back_list.back();
        const auto& first_change_set = changes_list_[back_list.back()&window_size_];
        RollBackData(address, *first_change_set.find(address)->second.begin());
      }
      if(cur_id == commit_id){
        back_list.push_back(commit_id);
      }
      AppendHeadRecord(address, hash_idx, back_list, pre_commit_list_);
      /*
      for(int64_t id : pre_commit_list_[hash_idx][address]){
        LOG(ERROR)<<" addr:"<<address<<" has id:"<<id;
      }
      */
    }
  }
}

bool XController::PostCommit(int64_t commit_id){
  //LOG(ERROR)<<"post commit:"<<commit_id;
  const auto& change_set = changes_list_[commit_id&window_size_];
  if(change_set.empty()){
    //LOG(ERROR)<<"no data";
    return false;
  }

  if(!CheckCommit(commit_id, commit_list_, false)){
    //LOG(ERROR)<<"check commit fail:"<<commit_id;
    return false;
  }
  if(change_set.empty()){
    LOG(ERROR)<<" no commit id record found:"<<commit_id;
    assert(1==0);
    return false;

  }
  for(const auto& it : change_set){
    bool done = false;
    const uint256_t& address = it.first;
    for(int i = it.second.size()-1; i >=0 && !done;--i){
      const auto& op = it.second[i];
      switch(op.state){
        case LOAD:
          break;
        case STORE:
          //LOG(ERROR)<<"commit:"<<it.first<<" data:"<<op.data<<" commit id:"<<commit_id;
          storage_->Store(it.first, op.data, false);
          done = true;
          break;
        case REMOVE:
          //LOG(ERROR)<<"remove:"<<it.first<<" data:";
          storage_->Remove(it.first, false);
          done = true;
          break;
      }
    }
    RemoveRecord(address, commit_id, commit_list_);
  }
  
  CommitDone(commit_id);
  Remove(commit_id);
  assert(last_commit_id_ == commit_id);
  last_commit_id_++;
  return true;
}

bool XController::PreCommitInternal(int64_t commit_id){
  if(!CheckCommit(commit_id, pre_commit_list_, true)){
    //LOG(ERROR)<<"check commit fail:"<<commit_id;
    return false;
  }

  if(!CheckPreCommit(commit_id)){
    //LOG(ERROR)<<"check pre commit fail:"<<commit_id;
    RedoConflict(commit_id);

    if(!CheckCommit(commit_id, pre_commit_list_, true)){
      //LOG(ERROR)<<"check commit fail:"<<commit_id;
      return false;
    }
  }

  const auto& change_set = changes_list_[commit_id&window_size_];
  if(change_set.empty()){
    LOG(ERROR)<<" no commit id record found:"<<commit_id;
    assert(1==0);
    return false;
  }

  std::set<int64_t> new_commit_ids;
  for(const auto& it : change_set){
    const uint256_t& address = it.first;
    //LOG(ERROR)<<"get pre-op:"<<address;
    bool done = false;
    for(int i = it.second.size()-1; i >=0 && !done;--i){
      const auto& op = it.second[i];
      switch(op.state){
        case LOAD:
          break;
        case STORE:
          //LOG(ERROR)<<"commit:"<<it.first<<" data:"<<op.data<<" commit id:"<<commit_id;
          storage_->Store(it.first, op.data, true);
          done = true;
          break;
        case REMOVE:
          //LOG(ERROR)<<"remove:"<<it.first<<" data:";
          storage_->Remove(it.first, true);
          done = true;
          break;
      }
    }
  
    //LOG(ERROR)<<"append id:"<<commit_id<<" to commit list";
    AppendRecord(address, commit_id, commit_list_);

    int64_t  next_commit_id = RemoveRecord(address, commit_id, pre_commit_list_);
    assert(next_commit_id>=-1);
    if(next_commit_id> 0 && next_commit_id <= last_pre_commit_id_){
      //LOG(ERROR)<<"commit id:"<<commit_id<<" trigger:"<<next_commit_id;
      assert(commit_id<next_commit_id);
      new_commit_ids.insert(next_commit_id);
    }
    if(next_commit_id>last_pre_commit_id_){
      assert(1==0);
    }
  }


  for(int64_t redo_commit: new_commit_ids){
    RedoCommit(redo_commit, 1);
  }
  return true;
}

void XController::MergeChangeList(int64_t commit_id){
    int idx = commit_id&window_size_;
    if(rechanges_list_[idx].empty()){
      return;
    }

    std::set<uint256_t> new_list;
    for(const auto& new_addr: rechanges_list_[idx]) {
      new_list.insert(new_addr.first);
    }

    for(const auto& old_addr : changes_list_[idx]){
      if(new_list.find(old_addr.first) == new_list.end()){
        //int64_t  next_commit_id = RemoveRecord(old_addr.first, commit_id, pre_commit_list_);
        int64_t next_commit_id = RemoveFromRecord(old_addr.first, commit_id, pre_commit_list_);

        //LOG(ERROR)<<" old addr:"<<old_addr.first<<" removed"<<" next:"<<next_commit_id<<" curr id:"<<commit_id;
        assert(next_commit_id>=-1);
        if(next_commit_id> 0 && next_commit_id <= last_pre_commit_id_ && next_commit_id > commit_id){
          //LOG(ERROR)<<"commit id:"<<commit_id<<" remove trigger:"<<next_commit_id;
          RedoCommit(next_commit_id, 1);
        }
      }
    }

    swap(rechanges_list_[idx], changes_list_[idx]);
    rechanges_list_[idx].clear();

    const auto & local_changes = changes_list_[idx];
    //LOG(ERROR)<<"merge list:"<<local_changes.empty();
    for(const auto& it : local_changes){
      const uint256_t& address = it.first;
      int hash_idx = GetHashKey(address);
      auto& commit_set = pre_commit_list_[hash_idx][address];
      if(commit_set.empty()){
        //LOG(ERROR)<<"merge front:"<<commit_id;
        assert(commit_set.empty() || commit_set.front() > commit_id);
        commit_set.push_front(commit_id);
      }
      else {
        std::stack<int64_t> tmp;
        while(!commit_set.empty() && commit_set.front() <= commit_id){
          tmp.push(commit_set.front());
          commit_set.pop_front();
        }
        if(tmp.empty() || tmp.top() != commit_id){
          tmp.push(commit_id);
        }
        while(!tmp.empty()){
          commit_set.push_front(tmp.top());
          tmp.pop();
        }
        //LOG(ERROR)<<"queue size:"<<commit_set.size()<<" addr:"<<address;
      }
    }
}


bool XController::PreCommit(int64_t commit_id){
  //LOG(ERROR)<<"pre commit:"<<commit_id<<" last pre commit_id:"<<last_pre_commit_id_;
  if(commit_id>last_pre_commit_id_){
    assert(rechanges_list_[commit_id&window_size_].empty());

    const auto & local_changes = changes_list_[commit_id&window_size_];
    //LOG(ERROR)<<"commit :"<<commit_id<<" last :"<<last_commit_id_<<" list size:"<<local_changes.size();  
    for(const auto& it : local_changes){
      //LOG(ERROR)<<" get op address:"<<it.first<<" commit id:"<<commit_id;
      AppendRecord(it.first, commit_id, pre_commit_list_);
    }
  }
  else {
    MergeChangeList(commit_id);

/*
    if(!CheckPreCommit(commit_id)){
      LOG(ERROR)<<"check pre commit fail:"<<commit_id;
      RedoConflict(commit_id);
    }
    */
  }

  bool ret = PreCommitInternal(commit_id);
  if(commit_id == last_pre_commit_id_+1){
    last_pre_commit_id_++;
  }
  //LOG(ERROR)<<"pre commit done:"<<ret<<" next:"<<last_pre_commit_id_<<" ret:"<<ret;
  return ret;
}

bool XController::Commit(int64_t commit_id){
  redo_.clear();
  done_.clear();
  is_redo_[commit_id&window_size_] = false;
  //LOG(ERROR)<<"commit:"<<commit_id<<" idx:"<<(commit_id&window_size_);
  //assert(is_done_[commit_id&window_size_] ==false);
  if(!PreCommit(commit_id)){
    return false;
  }

  while(current_commit_id_ <= last_commit_id_){
    bool ret = PostCommit(current_commit_id_);
    if(!ret){
      break;
    }
    current_commit_id_++;
  }
  //LOG(ERROR)<<"current commit id:"<<current_commit_id_<<" last commit id:"<<last_commit_id_;

  return true;
}

}
}
} 


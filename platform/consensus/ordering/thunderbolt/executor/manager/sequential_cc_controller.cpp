#include "service/contract/executor/manager/sequential_cc_controller.h"

#include <glog/logging.h>

namespace resdb {
namespace contract {

SequentialCCController::SequentialCCController(DataStorage* storage)
    : ConcurrencyController(storage) {
  for (int i = 0; i < window_size_; ++i) {
    is_redo_.push_back(false);
  }

  Clear();
  last_commit_id_ = 0;
}

SequentialCCController::~SequentialCCController() {}

void SequentialCCController::Clear() {
  last_commit_id_ = 0;

  changes_list_.resize(window_size_);
  for (int i = 0; i < window_size_; ++i) {
    changes_list_[i].clear();
    is_redo_[i] = false;
  }
  for (int i = 0; i < 1024; ++i) {
    commit_list_[i].clear();
  }
  m_list_.clear();
}

std::vector<int64_t>& SequentialCCController::GetRedo() { return redo_; }

int SequentialCCController::GetHashKey(const uint256_t& address) {
  // Get big-endian form
  uint8_t arr[32] = {};
  memset(arr, 0, sizeof(arr));
  intx::be::store(arr, address);
  uint32_t v = 0;
  for (int i = 0; i < 32; ++i) {
    v += arr[i];
  }
  return v % 128;
}

void SequentialCCController::SetCallback(CallBack call_back) {
  call_back_ = call_back;
}

void SequentialCCController::RedoCommit(int64_t commit_id, int flag) {
  if (is_redo_[commit_id]) {
    return;
  }
  is_redo_[commit_id] = true;
  redo_.push_back(commit_id);
}

void SequentialCCController::PushCommit(int64_t commit_id,
                                        const ModifyMap& local_changes) {
  if (commit_id <= last_commit_id_) {
    // assert(!changes_list_[commit_id].empty());
    changes_list_[commit_id] = local_changes;
    // assert(Commit(commit_id));
    return;
  }

  {
    changes_list_[commit_id] = local_changes;
    /*
    for(const auto& it : local_changes){
      int hash_idx = GetHashKey(it.first);
      //LOG(ERROR)<<"address:"<<it.first<<" idx:"<<hash_idx;
      std::unique_lock lock(mutexs_[hash_idx]);
      //LOG(ERROR)<<"load idx:"<<hash_idx;
      auto& commit_set = commit_list_[hash_idx][it.first];
      commit_set.insert(commit_id);
    }
    */
  }
}

bool SequentialCCController::Commit(int64_t commit_id) {
  if (commit_id > last_commit_id_) {
    const auto& local_changes = changes_list_[commit_id];
    for (const auto& it : local_changes) {
      int hash_idx = GetHashKey(it.first);
      // LOG(ERROR)<<"address:"<<it.first<<" idx:"<<hash_idx;
      // std::unique_lock lock(mutexs_[hash_idx]);
      // LOG(ERROR)<<"load idx:"<<hash_idx;
      auto& commit_set = commit_list_[hash_idx][it.first];
      commit_set.push_back(commit_id);
      // commit_set.insert(commit_id);
    }
  }

  redo_.clear();
  bool ret = CommitInternal(commit_id);
  if (commit_id == last_commit_id_ + 1) {
    last_commit_id_++;
  }
  return ret;
}

bool SequentialCCController::CommitInternal(int64_t commit_id) {
  if (!CheckCommit(commit_id)) {
    return false;
  }
  const auto& change_set = changes_list_[commit_id];
  if (change_set.empty()) {
    LOG(ERROR) << " no commit id record found:" << commit_id;
    return false;
  }
  std::set<int64_t> new_commit_ids;
  for (const auto& it : change_set) {
    bool done = false;
    const uint256_t& address = it.first;
    int64_t new_v = 0;
    for (int i = it.second.size() - 1; i >= 0 && !done; --i) {
      const auto& op = it.second[i];
      switch (op.state) {
        case LOAD:
          break;
        case STORE:
          // LOG(ERROR)<<"commit:"<<it.first<<" data:"<<op.data<<" commit
          // id:"<<commit_id;
          new_v = storage_->Store(it.first, op.data);
          done = true;
          break;
        case REMOVE:
          // LOG(ERROR)<<"remove:"<<it.first<<" data:";
          storage_->Remove(it.first);
          done = true;
          break;
      }
    }
    int64_t next_commit_id = Remove(address, commit_id, new_v);
    // if(next_commit_id> 0){
    if (next_commit_id > 0 && next_commit_id <= last_commit_id_) {
      new_commit_ids.insert(next_commit_id);
    }
    if (done && next_commit_id > last_commit_id_) {
      if (IsRead(address, next_commit_id)) {
        new_commit_ids.insert(next_commit_id);
      }
    }
  }
  Remove(commit_id);
  for (int64_t redo_commit : new_commit_ids) {
    RedoCommit(redo_commit, 1);
  }
  return true;
}

bool SequentialCCController::CheckFirstCommit(const uint256_t& address,
                                              int64_t commit_id) {
  int idx = GetHashKey(address);
  // std::shared_lock lock(mutexs_[idx]);
  // LOG(ERROR)<<"load idx:"<<idx;
  // std::shared_lock lock(mutex_);
  if (commit_list_[idx].find(address) == commit_list_[idx].end()) {
    LOG(ERROR) << "no address:" << commit_id << " add:" << address;
    return false;
  }
  const auto& commit_set = commit_list_[idx][address];
  // assert(*commit_set.begin()>0);

  if (commit_set.front() < commit_id) {
    // if(*commit_set.begin() < commit_id){
    // not the first candidate
    // LOG(ERROR)<<"still have dep. commit id:"<<commit_id<<"
    // dep:"<<*commit_list_[address].begin();
    return false;
  }
  // assert(*commit_set.begin() == commit_id);
  return true;
}

bool SequentialCCController::IsRead(const uint256_t& address,
                                    int64_t commit_id) {
  const auto& change_set = changes_list_[commit_id];
  if (change_set.empty()) {
    LOG(ERROR) << " no commit id record found:" << commit_id;
    // assert(!change_set.empty());
    return false;
  }

  for (const auto& it : change_set) {
    if (it.first != address) {
      continue;
    }
    for (auto& op : it.second) {
      if (op.state == LOAD) {
        return true;
      }
    }
  }
  return false;
}

bool SequentialCCController::CheckCommit(int64_t commit_id) {
  const auto& change_set = changes_list_[commit_id];
  if (change_set.empty()) {
    LOG(ERROR) << " no commit id record found:" << commit_id;
    // assert(!change_set.empty());
    return false;
  }

  for (const auto& it : change_set) {
    const uint256_t& address = it.first;
    if (!CheckFirstCommit(it.first, commit_id)) {
      return false;
    }

    for (auto& op : it.second) {
      if (op.state == LOAD) {
        uint64_t v = storage_->GetVersion(address);
        if (op.version != v) {
          // LOG(ERROR)<<"check load:"<<address<<" version:"<<v<<" data
          // v:"<<op.version<<" commit id:"<<commit_id<<"
          // mlist:"<<m_list_[it.first];
          RedoCommit(commit_id, 0);
          return false;
        }
      }
    }
  }
  return true;
}

const ConcurrencyController::ModifyMap* SequentialCCController::GetChangeList(
    int64_t commit_id) {
  // read lock
  return &changes_list_[commit_id];
}

void SequentialCCController::Remove(int64_t commit_id) {
  changes_list_[commit_id].clear();
}

int64_t SequentialCCController::Remove(const uint256_t& address,
                                       int64_t commit_id, uint64_t v) {
  int idx = GetHashKey(address);
  // std::unique_lock lock(mutex_);
  // std::unique_lock lock(mutexs_[idx]);
  // LOG(ERROR)<<"load idx:"<<idx;
  if (commit_list_[idx].find(address) == commit_list_[idx].end()) {
    LOG(ERROR) << " remove address:" << address << " commit id:" << commit_id
               << " not exist";
  }
  // assert(commit_list_.find(address) != commit_list_.end());

  auto& it = commit_list_[idx][address];
  // assert(*it.begin() == commit_id);
  it.pop_front();
  // it.erase(it.begin());
  // LOG(ERROR)<<"update address:"<<address<<" mlist:"<<m_list_[address];
  if (it.empty()) {
    commit_list_[idx].erase(commit_list_[idx].find(address));
    return -1;
  }
  return it.front();
}

}  // namespace contract
}  // namespace resdb

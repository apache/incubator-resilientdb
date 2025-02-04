#include "service/contract/executor/x_manager/e_controller.h"

#include <glog/logging.h>

#include <queue>
#include <stack>

#include "common/utils/utils.h"
#include "eEVM/exception.h"

//#define CDebug

namespace resdb {
namespace contract {
namespace x_manager {

namespace {

int GetHashKey(const uint256_t& address) {
  return 0;
  // Get big-endian form
  /*
  uint8_t arr[32] = {};
  memset(arr,0,sizeof(arr));
  intx::be::store(arr, address);
  uint32_t v = 0;
  for(int i = 0; i < 32; ++i){
    v += arr[i];
  }
  */

  const uint8_t* bytes = intx::as_bytes(address);
  // uint8_t arr[32] = {};
  // memset(arr,0,sizeof(arr));
  // intx::be::store(arr, address);
  // const uint8_t* bytes = arr;
  size_t sz = sizeof(address);
  int v = 0;
  for (int i = 0; i < sz; ++i) {
    v += bytes[i];
  }
  return v % 1;
}
}  // namespace
EController::EController(DataStorage* storage, int window_size)
    : ConcurrencyController(storage),
      window_size_(window_size),
      storage_(storage) {
  for (int i = 0; i < 32; i++) {
    if ((1 << i) > window_size_) {
      window_size_ = (1 << i) - 1;
      break;
    }
  }
  // LOG(ERROR)<<"init window size:"<<window_size_;
  Clear();
}

EController::~EController() {}

void EController::Clear() {
  assert(window_size_ + 1 <= 8192);
  changes_list_.resize(window_size_ + 1);
  pre_commit_list_.clear();
  cache_data_.clear();

  for (int i = 0; i < window_size_; ++i) {
    changes_list_[i].clear();
    wait_.push_back(false);
    aborted_[i] = false;
    finish_[i] = false;
    committed_[i] = false;
  }
}

class TimeTrack {
 public:
  TimeTrack(std::string name = "") {
    name_ = name;
    start_time_ = GetCurrentTime();
  }

  ~TimeTrack() {
    uint64_t end_time = GetCurrentTime();
    LOG(ERROR) << name_ << " run:" << (end_time - start_time_) << "ms";
  }

  double GetRunTime() {
    uint64_t end_time = GetCurrentTime();
    return (end_time - start_time_) / 1000000.0;
  }

 private:
  std::string name_;
  uint64_t start_time_;
};

// ========================================================
ModifyMap* EController::GetChangeList(int64_t commit_id) {
  // read lock
  return &(changes_list_[commit_id]);
}

bool EController::TryLock(const uint256_t& address, int64_t owner,
                          LockType type) {
  if (aborted_[owner]) {
    return false;
  }
  return true;
}

void EController::Abort(const int64_t commit_id) {
  if (aborted_[commit_id]) {
    return;
  }
  aborted_[commit_id] = true;

  std::lock_guard<std::mutex> lk(abort_mutex_);
  // LOG(ERROR)<<"abort:"<<commit_id;
  abort_list_.push_back(commit_id);
}

std::vector<int64_t> EController::FetchAbort() {
  std::vector<int64_t> p;
  {
    std::lock_guard<std::mutex> lk(abort_mutex_);
    p = abort_list_;
    abort_list_.clear();
  }
  return p;
}

void EController::Clear(int64_t commit_id) {
#ifdef CDebug
  // LOG(ERROR)<<"CLEAR id:"<<commit_id;
#endif

  std::unique_lock lock(g_mutex_);
  int idx = commit_id & window_size_;
  for (auto it : changes_list_[idx]) {
    const uint256_t& address = it.first;
    RemovePreRecord(address, commit_id);
  }

  changes_list_[commit_id].clear();
  wait_[commit_id] = false;
  aborted_[commit_id] = false;
  finish_[commit_id] = false;
}

void EController::StoreInternal(const int64_t commit_id,
                                const uint256_t& address,
                                const uint256_t& value, int version) {
  if (!TryLock(address, commit_id, WRITE)) {
    throw eevm::Exception(eevm::Exception::Type::outOfGas, "Get lock fail");
    return;
  }
  Data data = Data(STORE, value);
  AppendPreRecord(address, commit_id, data);
#ifdef CDebug
  // LOG(ERROR)<<"store data commit id:"<<commit_id<<" addr:"<<address<<"
  // value:"<<data.data<<" ver:"<<data.version;
#endif
}

uint256_t EController::LoadInternal(const int64_t commit_id,
                                    const uint256_t& address, int version) {
  if (!TryLock(address, commit_id, READ)) {
    throw eevm::Exception(eevm::Exception::Type::outOfGas, "Get lock fail");
    return 0;
  }
  Data data = Data(LOAD);
  AppendPreRecord(address, commit_id, data);
#ifdef CDebug
// LOG(ERROR)<<"load data commit id:"<<commit_id<<" addr:"<<address<<"
// value:"<<data.data<<" ver:"<<data.version;
#endif
  return data.data;
}

void EController::Store(const int64_t commit_id, const uint256_t& address,
                        const uint256_t& value, int version) {
  StoreInternal(commit_id, address, value, version);
}

uint256_t EController::Load(const int64_t commit_id, const uint256_t& address,
                            int version) {
  return LoadInternal(commit_id, address, version);
}

bool EController::Remove(const int64_t commit_id, const uint256_t& key,
                         int version) {
  Store(commit_id, key, 0, version);
  return true;
}

void EController::AddRedo(int64_t commit_id) {
  if (is_redo_[commit_id]) {
#ifdef CDebug
// LOG(ERROR)<<"redoing:"<<commit_id;
#endif
    return;
  }
  if (finish_[commit_id] == false) {
#ifdef CDebug
// LOG(ERROR)<<"not finish:"<<commit_id;
#endif
    return;
  }
  is_redo_[commit_id] = true;
#ifdef CDebug
  LOG(ERROR) << "add redo :" << commit_id;
#endif
  redo_.push_back(commit_id);
}

void EController::AppendPreRecord(const uint256_t& address, int64_t commit_id,
                                  Data& data) {
  {
    int idx = commit_id & window_size_;
    int hash_idx = GetHashKey(address);
    // std::lock_guard<std::mutex> lk(mutex_[hash_idx]);
    std::unique_lock lock(g_mutex_);
    // std::unique_lock lock(mutex_[hash_idx]);

    auto& commit_set = pre_commit_list_[address];
#ifdef CDebug
    LOG(ERROR) << "append commit set:" << commit_id << " address:" << address
               << " size:" << commit_set.size();
#endif

    std::stack<std::unique_ptr<DataInfo>> tmp;
    while (!commit_set.empty()) {
      auto& it = commit_set.back();
      if (!committed_[it->commit_id] &&
          (it->commit_id > commit_id || it->version == -1)) {
        // LOG(ERROR)<<"get data front address:"<<address<<" commit
        // id:"<<commit_id<<" front:"<<it->commit_id<<"
        // committed:"<<committed_[it->commit_id]; LOG(ERROR)<<"push
        // :"<<commit_set.back()->commit_id<<" version:"<<it->version;
        if (it->commit_id != commit_id) {
          tmp.push(std::move(it));
        }
        commit_set.pop_back();
      } else {
        break;
      }
    }

    if (data.state == LOAD) {
      if (commit_set.empty()) {
        auto cache_it = cache_data_.find(address);
        if (cache_it == cache_data_.end()) {
          auto ret = storage_->Load(address, false);
          data.data = ret.first;
          data.version = ret.second;
          // LOG(ERROR)<<"get from db commit id:"<<commit_id<<"
          // address:"<<address;
        } else {
          LOG(ERROR) << "get from cache commit id:" << commit_id
                     << " address:" << address;
          data.data = cache_it->second.first;
          data.version = cache_it->second.second;
        }
#ifdef CDebug
        LOG(ERROR) << "LOAD from db:"
                   << " version:" << data.version << " commit id:" << commit_id
                   << " address:" << address;
#endif
        // assert(ret.second<=1);
      } else {
        auto& it = commit_set.back();
        data.data = it->data.data;
        data.version = it->data.version;
#ifdef CDebug
        LOG(ERROR) << "LOAD from :" << it->commit_id
                   << " version:" << it->data.version << " address:" << address
                   << " commit id:" << commit_id
                   << " data version:" << it->version;
#endif
      }
    } else {
      if (commit_set.empty()) {
      } else {
        auto& it = commit_set.back();
        data.version = it->data.version + 1;
#ifdef CDebug
        LOG(ERROR) << "LOAD from :" << it->commit_id
                   << " version:" << it->data.version << " address:" << address
                   << " commit id:" << commit_id;
#endif
      }
    }

    int type = 1;
    if (data.state == LOAD) {
      type = 1;
    } else {
      type = 2;
    }

    // LOG(ERROR)<<"type:"<<type;
    if (!commit_set.empty()) {
      auto& it = commit_set.back();
      if (it->commit_id == commit_id) {
        it->type |= type;
        it->data = data;
        // assert(it->version==0);
      } else {
        commit_set.push_back(std::make_unique<DataInfo>(commit_id, type, data));
        commit_set.back()->version = 0;
      }
      // LOG(ERROR)<<"back type:"<<commit_set.back()->type;
    } else {
      commit_set.push_back(std::make_unique<DataInfo>(commit_id, type, data));
      commit_set.back()->version = 0;
    }

    bool flag = true;
    while (!tmp.empty()) {
      int ok = 1;
      if (data.state == STORE && flag) {
        if (tmp.top()->type & 1) {
#ifdef CDebug
          LOG(ERROR) << "abbort:" << tmp.top()->commit_id
                     << " from:" << commit_id << " address:" << address;
#endif
          ok = 0;
          Abort(tmp.top()->commit_id);
        } else if (tmp.top()->type & 2) {
          flag = false;
        }
      }
      if (ok) {
        commit_set.push_back(std::move(tmp.top()));
      }
      tmp.pop();
    }
    // LOG(ERROR)<<"add size:"<<pre_commit_list_[hash_idx][address].size();
  }
  int idx = commit_id & window_size_;
  // std::unique_lock lock(change_list_mutex_[idx]);
  auto& change_set = changes_list_[idx][address];
  if (change_set.empty()) {
#ifdef CDebug
    LOG(ERROR) << "append commit set:" << commit_id << " address:" << address
               << " size:" << change_set.size() << " commit id:" << commit_id
               << " type:" << data.state;
#endif
    change_set.push_back(data);
  } else if (data.state != LOAD) {
    if (change_set.back().state != LOAD) {
      change_set.pop_back();
    }
    // LOG(ERROR)<<"append commit set:"<<commit_id<<" address:"<<address<<"
    // size:"<<change_set.size()<<" commit id:"<<commit_id<<" type:"<<data.state;
    change_set.push_back(data);
  }
  // LOG(ERROR)<<"commit id:"<<commit_id<<" size:"<<change_set.size();
  assert(change_set.size() < 3);
}

void EController::RemovePreRecord(const uint256_t& address, int64_t commit_id) {
  // LOG(ERROR)<<" release:"<<commit_id<<" address:"<<address;
  {
    int idx = commit_id & window_size_;
    // int hash_idx = GetHashKey(address);
    // std::lock_guard<std::mutex> lk(mutex_[hash_idx]);
    // std::unique_lock lock(g_mutex_);
    // std::unique_lock lock(mutex_[hash_idx]);

    auto& commit_set = pre_commit_list_[address];
    // LOG(ERROR)<<"remove commit set:"<<commit_id<<" address:"<<address<<"
    // size:"<<commit_set.size()<<" commit:"<<committed_[commit_id];
    if (committed_[commit_id]) {
      return;
    }

    std::stack<std::unique_ptr<DataInfo>> tmp;
    while (!commit_set.empty()) {
      auto& it = commit_set.back();
      if (it->commit_id != commit_id) {
        // LOG(ERROR)<<"push queue:"<<commit_set.back()->commit_id<<"
        // address:"<<address;
        tmp.push(std::move(it));
        commit_set.pop_back();
      } else {
        break;
      }
    }

    // LOG(ERROR)<<"start:"<<is_start_[idx]<<" versioin:"<<version_[idx];
    int type = 0;
    if (!commit_set.empty()) {
      auto& it = commit_set.back();
      if (it->commit_id == commit_id) {
        type = it->type;
        // LOG(ERROR)<<"remove old data:"<<commit_id<<" address:"<<address;
        commit_set.pop_back();
      }
    }

    bool flag = true;
    while (!tmp.empty()) {
      int ok = 1;
      if ((type & 2) && flag) {
        if (tmp.top()->type & 1) {
          tmp.top()->version = -1;
#ifdef CDebug
          LOG(ERROR) << "abort :" << tmp.top()->commit_id
                     << " from release:" << commit_id;
#endif
          ok = 0;
          Abort(tmp.top()->commit_id);
        } else if ((tmp.top()->type & 2)) {
          flag = false;
        }
      }
      if (ok) {
        commit_set.push_back(std::move(tmp.top()));
      }
      tmp.pop();
    }
    // LOG(ERROR)<<"add size:"<<pre_commit_list_[hash_idx][address].size();
  }
}

// ==========================================

bool EController::CheckCommit(int64_t commit_id) {
  const auto& change_set = changes_list_[commit_id];
  if (change_set.empty()) {
    LOG(ERROR) << " no commit id record found:" << commit_id;
    assert(!change_set.empty());
    return false;
  }

  for (const auto& it : change_set) {
    // LOG(ERROR)<<"op size:"<<it.second.size()<<" commit id:"<<commit_id;
    for (auto& op : it.second) {
      const auto& address = it.first;
      uint64_t v = storage_->GetVersion(it.first, false);
      if (op.state == STORE) {
        if (op.version + 1 != v) {
          LOG(ERROR) << "state:" << op.state << " addr:" << it.first
                     << " version:" << op.version << " db version:" << v
                     << " address:" << address << " commit id:" << commit_id;
          return false;
        }
      } else {
        if (op.version != v) {
          LOG(ERROR) << "state:" << op.state << " addr:" << it.first
                     << " version:" << op.version << " db version:" << v
                     << " address:" << address << " commit id:" << commit_id;
          return false;
        }
      }
      break;
    }
  }
  return true;
}

bool EController::CommitUpdates(int64_t commit_id) {
#ifdef CDebug
  LOG(ERROR) << "check commit:" << commit_id << " wait:" << wait_[commit_id]
             << " abort:" << aborted_[commit_id];
#endif
  wait_[commit_id] = false;
  commit_delay_[commit_id] = GetCurrentTime() - commit_time_[commit_id];
  const auto& change_set = changes_list_[commit_id];
  if (change_set.empty()) {
#ifdef CDebug
    LOG(ERROR) << " no commit id record found:" << commit_id;
#endif
    assert(!change_set.empty());
    return false;
  }
  if (aborted_[commit_id]) {
    // Clear(commit_id);
    return false;
  }
  if (committed_[commit_id]) {
#ifdef CDebug
    LOG(ERROR) << " commit id committed:" << commit_id;
#endif
    assert(1 == 0);
  }
  // std::vector<int> lock_index;
  // std::set<int> locked;
  std::vector<std::pair<uint256_t, std::pair<uint256_t, uint64_t>>> tmp_data;
  //(std::make_pair(op.data, op.version));
  {
    std::unique_lock lock1(g_mutex_);
    // std::vector<std::unique_ptr<std::unique_lock<std::mutex>>> lock_list;
    /*
    for(const auto& it : change_set){
      const auto& address = it.first;
      int hash_idx = GetHashKey(address);
      lock_index.push_back(hash_idx);
    }
    */

    for (const auto& it : change_set) {
      const auto& address = it.first;
      auto& commit_set = pre_commit_list_[address];
      std::unique_ptr<DataInfo> last_commit = nullptr;
      while (!commit_set.empty()) {
        auto& it = commit_set.front();
        if (committed_[it->commit_id]) {
          last_commit = std::move(commit_set.front());
          commit_set.pop_front();
        } else {
          break;
        }
      }
      assert(!commit_set.empty());
      /*
      if(last_commit){
        LOG(ERROR)<<"get last commit:"<<last_commit->commit_id<<" commit
      id:"<<commit_id<<" address:"<<address;
      }
      */
      if (commit_set.empty() || commit_set.front()->commit_id != commit_id) {
        wait_[commit_id] = true;
#ifdef CDebug
        if (commit_set.empty()) {
          LOG(ERROR) << "not the first one: empty:" << commit_id;
          assert(!commit_set.empty());
        } else {
          LOG(ERROR) << "not the first one:" << commit_set.front()->commit_id
                     << " wait for:" << commit_id
                     << " abort:" << aborted_[commit_set.front()->commit_id]
                     << " address:" << address;
        }
#endif
      }
      if (last_commit) {
        commit_set.push_front(std::move(last_commit));
      }
      if (wait_[commit_id]) {
        return true;
      }
    }

    for (const auto& it : change_set) {
      // int idx = lock_index[i++];
      const auto& address = it.first;
      // int idx = GetHashKey(address);
      auto& commit_set = pre_commit_list_[address];
#ifdef CDebug
      LOG(ERROR) << "commit:" << commit_id << " address:" << address
                 << " data version:" << it.second.back().version
                 << " data sze:" << commit_set.size();
#endif
      while (!commit_set.empty()) {
        auto& it = commit_set.front();
        if (committed_[it->commit_id]) {
          commit_set.pop_front();
        } else {
          break;
        }
      }
      std::unique_ptr<DataInfo> last_commit = std::move(commit_set.front());
      commit_set.pop_front();
      if (commit_set.empty()) {
      } else {
        // LOG(ERROR)<<" commit id:"<<commit_id<<"
        // trigger:"<<commit_set.front()->commit_id<<"
        // wait:"<<wait_[commit_set.front()->commit_id];
        assert(commit_set.front()->commit_id != commit_id);
        if (wait_[commit_set.front()->commit_id]) {
          pd_.insert(commit_set.front()->commit_id);
        }
      }
      commit_set.push_front(std::move(last_commit));
    }
    committed_[commit_id] = true;
    /*
       if(!CheckCommit(commit_id)){
       LOG(ERROR)<<"check commit fail:"<<commit_id;
       assert(1==0);
       return false;
       }
     */
    for (const auto& it : change_set) {
      // LOG(ERROR)<<"commit addr:"<<it.first<<" size:"<<it.second.size();
      bool done = false;
      for (int i = it.second.size() - 1; i >= 0 && !done; --i) {
        const auto& op = it.second[i];
        switch (op.state) {
          case LOAD:
            // LOG(ERROR)<<"load";
            break;
          case STORE: {
            if (op.version == 0) {
              // storage_->Store(it.first, op.data);
              cache_data_[it.first] = std::make_pair(op.data, 1);
              tmp_data.push_back(
                  std::make_pair(it.first, std::make_pair(op.data, 0)));
              // LOG(ERROR)<<"save to cache commit id:"<<commit_id<<"
              // address:"<<it.first;
            } else {
              auto c_it = cache_data_.find(it.first);
              if (c_it == cache_data_.end()) {
                uint64_t v = storage_->GetVersion(it.first, false);
                if (op.version > v) {
                  // cache_data_[it.first] = std::make_pair(op.data,
                  // op.version); tmp_data.push_back(std::make_pair(it.first,
                  // std::make_pair(op.data, op.version)));
                  storage_->StoreWithVersion(it.first, op.data, op.version);
                  // LOG(ERROR)<<"save to cache commit id:"<<commit_id<<"
                  // address:"<<it.first;
                }
              } else {
                uint64_t v = c_it->second.second;
                if (op.version > v) {
                  // cache_data_[it.first] = std::make_pair(op.data,
                  // op.version); tmp_data.push_back(std::make_pair(it.first,
                  // std::make_pair(op.data, op.version)));
                  storage_->StoreWithVersion(it.first, op.data, op.version);
                  // LOG(ERROR)<<"save to cache commit id:"<<commit_id<<"
                  // address:"<<it.first;
                }
              }
            }
            done = true;
            break;
          }
          case REMOVE:
            // LOG(ERROR)<<"remove:"<<it.first<<" data:";
            // storage_->Remove(it.first, false);
            done = true;
            break;
        }
      }
    }
  }

  for (auto& v : tmp_data) {
    if (v.second.second == 0) {
      storage_->Store(v.first, v.second.first);
    } else {
      storage_->StoreWithVersion(v.first, v.second.first, v.second.second,
                                 false);
    }
  }

  done_.push_back(commit_id);
#ifdef CDebug
  LOG(ERROR) << "commit done:" << commit_id;
#endif
  return true;
}

bool EController::Commit(int64_t commit_id) {
#ifdef CDebug
  LOG(ERROR) << "commit id:" << commit_id;
#endif
  redo_.clear();
  done_.clear();
  finish_[commit_id] = true;
  commit_time_[commit_id] = GetCurrentTime();
  is_redo_[commit_id] = false;
  bool ret = CommitUpdates(commit_id);
  if (!ret) {
    AddRedo(commit_id);
  }
  while (pd_.size()) {
    int id = *pd_.begin();
    pd_.erase(pd_.begin());
    if (!CommitUpdates(id)) {
      AddRedo(id);
    }
  }
  std::vector<int64_t> list = FetchAbort();
  for (int64_t id : list) {
    AddRedo(id);
  }
  // LOG(ERROR)<<"commit time:"<<GetCurrentTime-time;
  // changes_log_[commit_id]=changes_list_[commit_id];
  // changes_list_[commit_id].clear();
  return true;
}

const std::vector<int64_t>& EController::GetRedo() { return redo_; }

const std::vector<int64_t>& EController::GetDone() { return done_; }

uint64_t EController::GetDelay(int64_t commit_id) {
  return commit_delay_[commit_id];
}

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb

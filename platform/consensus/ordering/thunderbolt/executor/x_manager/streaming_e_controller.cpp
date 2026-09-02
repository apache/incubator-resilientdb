#include "service/contract/executor/x_manager/streaming_e_controller.h"

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
  // Get big-endian form
  uint8_t arr[32] = {};
  memset(arr, 0, sizeof(arr));
  intx::be::store(arr, address);
  uint32_t v = 0;
  for (int i = 0; i < 32; ++i) {
    v += arr[i];
  }

  /*
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
  */
  return v % 1;
}
}  // namespace

StreamingEController::StreamingEController(DataStorage* storage,
                                           int window_size)
    : ConcurrencyController(storage),
      window_size_(window_size),
      storage_(static_cast<D_Storage*>(storage)) {
  for (int i = 0; i < 32; i++) {
    if ((1 << i) > window_size_) {
      window_size_ = (1 << i) - 1;
      break;
    }
  }
  // LOG(ERROR)<<"init window size:"<<window_size_;
  Clear();
}

StreamingEController::~StreamingEController() {}

void StreamingEController::SetPrecommitCallback(
    std::function<void(int64_t)> precommit_callback) {
  precommit_callback_ = std::move(precommit_callback);
}

void StreamingEController::Clear() {
  last_commit_id_ = 1;
  current_commit_id_ = 1;
  last_pre_commit_id_ = 0;

  assert(window_size_ + 1 <= 4096);
  // is_redo_.resize(window_size_+1);
  // is_done_.resize(window_size_+1);
  changes_list_.resize(window_size_ + 1);
  rechanges_list_.resize(window_size_ + 1);

  for (int i = 0; i < window_size_; ++i) {
    changes_list_[i].clear();
    rechanges_list_[i].clear();
    is_redo_[i] = 0;
    is_invalid_[i] = 0;
    commit_[i] = 0;
    version_[i] = 0;
    is_start_[i] = true;
    // is_done_[i] = false;
  }

  for (int i = 0; i < 4096; ++i) {
    commit_list_[i].clear();
  }
  pre_commit_list_.clear();
}

void StreamingEController::GenRedo() {
  // std::unique_lock lock(mutex_[commit_id&window_size_]);

  return;
}

std::vector<int64_t>& StreamingEController::GetRedo() {
  // std::unique_lock lock(mutex_[commit_id&window_size_]);
  return redo_;
}

std::vector<int64_t>& StreamingEController::GetDone() { return done_; }

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
void StreamingEController::StoreInternal(const int64_t commit_id,
                                         const uint256_t& address,
                                         const uint256_t& value, int version) {
  {
    std::unique_lock lock(valid_mutex_[commit_id & window_size_]);
    if (is_invalid_[commit_id & window_size_]) {
      // LOG(ERROR)<<"aborted:"<<commit_id;
      throw eevm::Exception(eevm::Exception::Type::outOfGas, "Get lock fail");
    }
  }
  Data data = Data(STORE, value);
  AppendPreRecord(address, commit_id, data, version);
  // LOG(ERROR)<<"store data commit id:"<<commit_id<<" addr:"<<address<<"
  // value:"<<data.data<<" ver:"<<data.version;
}

uint256_t StreamingEController::LoadInternal(const int64_t commit_id,
                                             const uint256_t& address,
                                             int version) {
  {
    std::unique_lock lock(valid_mutex_[commit_id & window_size_]);
    if (is_invalid_[commit_id & window_size_]) {
      // LOG(ERROR)<<"aborted:"<<commit_id;
      throw eevm::Exception(eevm::Exception::Type::outOfGas, "Get lock fail");
    }
  }
  Data data = Data(LOAD);
  AppendPreRecord(address, commit_id, data, version);
  // LOG(ERROR)<<"load data commit id:"<<commit_id<<" addr:"<<address<<"
  // value:"<<data.data<<" ver:"<<data.version;
  return data.data;
}

void StreamingEController::Store(const int64_t commit_id,
                                 const uint256_t& address,
                                 const uint256_t& value, int version) {
  // TimeTrack tract("store");
  StoreInternal(commit_id, address, value, version);
}

uint256_t StreamingEController::Load(const int64_t commit_id,
                                     const uint256_t& address, int version) {
  // TimeTrack tract("remove");
  return LoadInternal(commit_id, address, version);
}

bool StreamingEController::Remove(const int64_t commit_id, const uint256_t& key,
                                  int version) {
  // LOG(ERROR)<<"remove key:"<<key<<" commit id:"<<commit_id;
  // TimeTrack tract("remove");
  Store(commit_id, key, 0, version);
  return true;
}

void StreamingEController::Abort(int64_t commit_id) {
#ifdef CDebug
  LOG(ERROR) << "abort:" << commit_id;
#endif
  is_invalid_[commit_id & window_size_] = true;

  std::lock_guard<std::mutex> lk(abort_mutex_);
  abort_list_.push_back(commit_id);
}

std::vector<int64_t> StreamingEController::FetchAbort() {
  std::vector<int64_t> p;
  {
    std::lock_guard<std::mutex> lk(abort_mutex_);
    p = abort_list_;
    abort_list_.clear();
  }
  return p;
}

void StreamingEController::AppendPreRecord(const uint256_t& address,
                                           int64_t commit_id, Data& data,
                                           int version) {
  data.commit_version = version;
#ifdef CDebug
  LOG(ERROR) << "append record addr:" << address << " commit id:" << commit_id;
#endif
  {
    int idx = commit_id & window_size_;
    // std::lock_guard<std::mutex> lk(mutex_[hash_idx]);
    std::unique_lock lock(g_mutex_);
    // std::unique_lock lock(mutex_[hash_idx]);
    if (is_invalid_[commit_id]) {
#ifdef CDebug
      LOG(ERROR) << "append commit id:" << commit_id << " address:" << address
                 << " aborted";
#endif
      return;
    }

    auto& commit_set = pre_commit_list_[address];
#ifdef CDebug
    LOG(ERROR) << "append commit set:" << commit_id << " address:" << address
               << " size:" << commit_set.size()
               << " invalid:" << is_invalid_[commit_id];
#endif

    std::stack<std::unique_ptr<DataInfo>> tmp;
    while (!commit_set.empty()) {
      if (commit_set.back()->commit_id > commit_id) {
        // LOG(ERROR)<<"push :"<<commit_set.back()->commit_id;
        tmp.push(std::move(commit_set.back()));
        commit_set.pop_back();
      } else {
        break;
      }
    }

    if (data.state == LOAD) {
      if (commit_set.empty()) {
        auto ret = storage_->Load(address, false);
        data.data = ret.first;
        data.version = ret.second;
        // LOG(ERROR)<<"LOAD from db:"<<" version:"<<data.version;
      } else {
        data.data = commit_set.back()->data.data;
        data.version = commit_set.back()->data.version;
        // LOG(ERROR)<<"LOAD from :"<<it->second->commit_id<<" version:"<<
        // it->second->data.version;
      }
    } else {
      if (commit_set.empty()) {
        auto ret = storage_->Load(address, false);
        data.version = ret.second + 1;
        // LOG(ERROR)<<"LOAD from db:";
      } else {
        data.version = commit_set.back()->data.version + 1;
        // LOG(ERROR)<<"LOAD from :"<<it->second->commit_id<<"
        // version:"<<it->second->data.version;
      }
    }

    int type = 1;
    if (data.state == LOAD) {
      type = 1;
    } else {
      type = 2;
    }

    if (!commit_set.empty()) {
      if (commit_set.back()->commit_id == commit_id) {
        commit_set.back()->type |= type;
        commit_set.back()->data = data;
      } else {
        commit_set.push_back(std::make_unique<DataInfo>(commit_id, type, data));
      }
      // LOG(ERROR)<<"back type:"<<commit_set.back()->type;
    } else {
      commit_set.push_back(std::make_unique<DataInfo>(commit_id, type, data));
    }

    while (!tmp.empty()) {
      int ok = 1;
      if (data.state == STORE) {
        if (tmp.top()->type & 1) {
#ifdef CDebug
          LOG(ERROR) << "abbort:" << tmp.top()->commit_id
                     << " from:" << commit_id << " address:" << address;
#endif
          ok = 0;
          Abort(tmp.top()->commit_id);
        }
      }
      if (ok) {
        commit_set.push_back(std::move(tmp.top()));
      }
      tmp.pop();
    }
  }

  int idx = commit_id & window_size_;
  // std::unique_lock lock(change_list_mutex_[idx]);
  auto& change_set = changes_list_[idx][address];
  if (change_set.empty()) {
    change_set.push_back(data);
  } else if (data.state != LOAD && !change_set.empty()) {
    if (change_set.back().state != LOAD) {
      change_set.pop_back();
    }
    change_set.push_back(data);
  }
  assert(change_set.size() < 3);
  rechanges_list_[idx].insert(address);
  // LOG(ERROR)<<"append record addr:"<<address<<" commit id:"<<commit_id<<"
  // done:"<< change_set.size()<<" data value:"<<data.data;
  return;
}

void StreamingEController::AddRedo(int64_t commit_id) {
  if (is_redo_[commit_id]) {
    return;
  }
  redo_list_.insert(commit_id);
#ifdef CDebug
  LOG(ERROR) << "add redo:" << commit_id;
#endif
}

// ==========================================

void StreamingEController::RedoCommit(int64_t commit_id, int flag) {
  int idx = commit_id & window_size_;
  if (is_redo_[idx] == 1) {
    return;
  }
  if (is_start_[idx] == true) {
    return;
  }

  is_redo_[idx] = 1;

#ifdef CDebug
  LOG(ERROR) << "add redo:" << commit_id << " flag:" << flag;
#endif
  redo_.push_back(commit_id);
}

void StreamingEController::Release(int64_t commit_id) {}

void StreamingEController::RemovePreRecord(const uint256_t& address,
                                           int64_t commit_id) {
  {
    int idx = commit_id & window_size_;
    int hash_idx = GetHashKey(address);

    auto& commit_set = pre_commit_list_[address];
    // LOG(ERROR)<<"remove commit set:"<<commit_id<<" address:"<<address<<"
    // size:"<<commit_set.size()<<" commit:"<<committed_[commit_id];
    if (commit_[commit_id]) {
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

void StreamingEController::Clear(int64_t commit_id) {
#ifdef CDebug
  LOG(ERROR) << "clear commit id:" << commit_id;
#endif

  std::unique_lock lock(g_mutex_);
  int idx = commit_id & window_size_;
  for (auto it : changes_list_[idx]) {
    const uint256_t& address = it.first;
    RemovePreRecord(address, commit_id);
  }

  commit_[idx] = false;
  is_invalid_[idx] = false;
  is_start_[idx] = true;
  wait_[idx] = false;
  version_[idx]++;
  changes_list_[idx].clear();
}

bool StreamingEController::PreCommit(int64_t commit_id) {
#ifdef CDebug
  LOG(ERROR) << "pre commit:" << commit_id
             << " last pre commit:" << last_commit_id_;
#endif
  int idx = commit_id & window_size_;

  std::unique_lock lock(g_mutex_);
  wait_[idx] = false;

  if (is_invalid_[idx]) {
#ifdef CDebug
    LOG(ERROR) << " commit id:" << commit_id << " aborted:";
#endif
    RedoCommit(commit_id, 0);
    return false;
  }
  const auto& change_set = changes_list_[idx];
  if (change_set.empty()) {
#ifdef CDebug
    LOG(ERROR) << "no data set:" << commit_id;
#endif
    // assert(1==0);
    RedoCommit(commit_id, 0);
    return false;
  }

  {
    if (idx != last_commit_id_) {
      // LOG(ERROR)<<" commit id:"<<commit_id<<" wait:";
      wait_[idx] = true;
      return true;
    }

    bool ok = CheckCommit(commit_id);

    for (auto& it : changes_list_[commit_id]) {
      const uint256_t& address = it.first;
      auto& commit_set = pre_commit_list_[address];

      if (commit_set.empty()) {
        LOG(ERROR) << "commit is empty:" << commit_id;
        assert(!commit_set.empty());
      }
      if (commit_set.front()->commit_id != commit_id) {
        LOG(ERROR) << "not the head:" << commit_id
                   << " head:" << commit_set.front()->commit_id
                   << " address:" << address;
      }
      assert(!commit_set.empty() && commit_set.front()->commit_id == commit_id);
      commit_set.pop_front();
      /*
      if(!commit_set.empty()){
        LOG(ERROR)<<"after commit:"<<commit_set.front()->commit_id<<" commit
      id:"<<commit_id<<" address:"<<address;
        assert(commit_set.front()->commit_id != commit_id);
      }
      else {
        LOG(ERROR)<<"after commit empty. commit id:"<<commit_id<<"
      address:"<<address;
      }
      */
    }

    if (!ok) {
      RedoCommit(commit_id, 0);
      return false;
    }

    for (const auto& it : change_set) {
      bool done = false;
      const uint256_t& address = it.first;
      for (int i = it.second.size() - 1; i >= 0 && !done; --i) {
        const auto& op = it.second[i];
        switch (op.state) {
          case LOAD:
            break;
          case STORE:
            // LOG(ERROR)<<"commit:"<<it.first<<" data:"<<op.data<<" commit
            // id:"<<commit_id;
            storage_->StoreWithVersion(it.first, op.data, op.version, false);
            done = true;
            break;
          case REMOVE:
            // LOG(ERROR)<<"remove:"<<it.first<<" data:";
            storage_->Remove(it.first, false);
            done = true;
            break;
        }
      }
    }
    commit_[commit_id & window_size_] = true;
  }

  CommitDone(commit_id);
  last_commit_id_++;

  return true;
}

// ================= post commit =======================

void StreamingEController::CommitDone(int64_t commit_id) {
#ifdef CDebug
  LOG(ERROR) << "commit done:" << commit_id;
#endif
  done_.push_back(commit_id);
}

bool StreamingEController::CheckCommit(int64_t commit_id) {
  const auto& change_set = changes_list_[commit_id];
  if (change_set.empty()) {
    LOG(ERROR) << " no commit id record found:" << commit_id;
    assert(!change_set.empty());
    return false;
  }

  for (const auto& it : change_set) {
    for (auto& op : it.second) {
      uint64_t v = storage_->GetVersion(it.first, false);
      if (op.state == STORE) {
        if (op.version != v + 1) {
          LOG(ERROR) << "state:" << op.state << " addr:" << it.first
                     << " version:" << op.version << " db version:" << v;
          return false;
        }
      } else {
        if (op.version != v) {
          // LOG(ERROR)<<"state:"<<op.state<<" addr:"<<it.first<<"
          // version:"<<op.version<<" db version:"<<v;
          return false;
        }
      }
      break;
    }
  }
  return true;
}

bool StreamingEController::Commit(int64_t commit_id) {
#ifdef CDebug
  LOG(ERROR) << "commit === :" << commit_id
             << " last commit :" << last_commit_id_;
#endif
  redo_.clear();
  done_.clear();

  int idx = commit_id & window_size_;
  is_redo_[idx] = false;
  is_start_[idx] = false;

  auto p = FetchAbort();
  for (int id : p) {
    RedoCommit(id, 0);
  }

  if (!PreCommit(commit_id)) {
    return false;
  }

  if (wait_[idx]) {
    return true;
  }

  while (true) {
    int cur = last_commit_id_;
    if (!wait_[cur & window_size_]) {
      // LOG(ERROR)<<" last commit is waiting:"<<cur;
      break;
    }
    PreCommit(cur);
    if (!commit_[cur & window_size_]) {
      // LOG(ERROR)<<" last commit not commit :"<<cur;
      break;
    }
  }

  return true;
}

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb

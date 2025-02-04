#include "service/contract/executor/x_manager/2pl_controller.h"

#include <glog/logging.h>

#include <queue>
#include <stack>

#include "common/utils/utils.h"
#include "eEVM/exception.h"

//#define CDebug

namespace resdb {
namespace contract {
namespace x_manager {

TwoPLController::TwoPLController(DataStorage* storage, int window_size)
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

TwoPLController::~TwoPLController() {}

void TwoPLController::Clear() {
  assert(window_size_ + 1 <= 8192);
  changes_list_.resize(window_size_ + 1);
  pre_commit_list_.clear();
  lock_table_.clear();

  for (int i = 0; i < window_size_; ++i) {
    changes_list_[i].clear();
    aborted_[i] = false;
    finish_[i] = false;
    committed_[i] = false;
  }
}

// ========================================================
ModifyMap* TwoPLController::GetChangeList(int64_t commit_id) {
  // read lock
  return &(changes_list_[commit_id]);
}

bool TwoPLController::TryLock(const uint256_t& address, int64_t owner,
                              LockType type) {
  if (aborted_[owner]) {
    return false;
  }
  return true;
}

void TwoPLController::Abort(const int64_t commit_id) {
  if (aborted_[commit_id]) {
    return;
  }
  // LOG(ERROR)<<"abort:"<<commit_id;
  aborted_[commit_id] = true;
}

void TwoPLController::Clear(int64_t commit_id) {
#ifdef CDebug
  // LOG(ERROR)<<"CLEAR id:"<<commit_id;
#endif

  ReleaseLock(commit_id);

  changes_list_[commit_id].clear();
  aborted_[commit_id] = false;
  finish_[commit_id] = false;
}

void TwoPLController::StoreInternal(const int64_t commit_id,
                                    const uint256_t& address,
                                    const uint256_t& value, int version) {
  Data data = Data(STORE, value);
  AppendPreRecord(address, commit_id, data);
  if (!TryLock(address, commit_id, WRITE)) {
    throw eevm::Exception(eevm::Exception::Type::outOfGas, "Get lock fail");
    return;
  }
}

uint256_t TwoPLController::LoadInternal(const int64_t commit_id,
                                        const uint256_t& address, int version) {
  if (!TryLock(address, commit_id, READ)) {
    throw eevm::Exception(eevm::Exception::Type::outOfGas, "Get lock fail");
    return 0;
  }
  Data data = Data(LOAD);
  AppendPreRecord(address, commit_id, data);
  if (!TryLock(address, commit_id, READ)) {
    throw eevm::Exception(eevm::Exception::Type::outOfGas, "Get lock fail");
    return 0;
  }
  return data.data;
}

void TwoPLController::Store(const int64_t commit_id, const uint256_t& address,
                            const uint256_t& value, int version) {
  StoreInternal(commit_id, address, value, version);
}

uint256_t TwoPLController::Load(const int64_t commit_id,
                                const uint256_t& address, int version) {
  return LoadInternal(commit_id, address, version);
}

bool TwoPLController::Remove(const int64_t commit_id, const uint256_t& key,
                             int version) {
  Store(commit_id, key, 0, version);
  return true;
}

void TwoPLController::AddRedo(int64_t commit_id) {
  if (is_redo_[commit_id]) {
    return;
  }
  if (finish_[commit_id] == false) {
    return;
  }
  is_redo_[commit_id] = true;
  redo_.push_back(commit_id);
}

void TwoPLController::AppendPreRecord(const uint256_t& address,
                                      int64_t commit_id, Data& data) {
  // LOG(ERROR)<<"append address:"<<address<<" commit id:"<<commit_id<<"
  // state:"<<data.state;
  {
    std::unique_lock lock(g_mutex_);
    auto it = lock_table_.find(address);
    if (it != lock_table_.end() && it->second != commit_id) {
      // LOG(ERROR)<<"append address:"<<address<<" commit id:"<<commit_id<<"
      // state:"<<data.state<<" has been locked";
      Abort(commit_id);
      return;
    }
    lock_table_[address] = commit_id;
    if (data.state == LOAD) {
      auto ret = storage_->Load(address, false);
      data.data = ret.first;
      data.version = ret.second;
#ifdef CDebug
      LOG(ERROR) << "LOAD from db:"
                 << " address:" << address << " commit id:" << commit_id
                 << " version:" << data.version;
#endif
    }
  }
  int idx = commit_id & window_size_;
  // std::unique_lock lock(change_list_mutex_[idx]);
  auto& change_set = changes_list_[idx][address];
  if (change_set.empty()) {
    change_set.push_back(data);
  } else if (data.state != LOAD) {
    if (change_set.back().state != LOAD) {
      change_set.pop_back();
    }
    change_set.push_back(data);
  }
  assert(change_set.size() < 3);
}

void TwoPLController::ReleaseLock(int64_t commit_id) {
  const auto& change_set = changes_list_[commit_id];
  if (change_set.empty()) {
    // LOG(ERROR)<<" no commit id record found:"<<commit_id;
    return;
  }
  for (const auto& it : change_set) {
    const auto& address = it.first;
    std::unique_lock lock(g_mutex_);
    assert(lock_table_[address] == commit_id);
    lock_table_.erase(lock_table_.find(address));
    // LOG(ERROR)<<"release lock address:"<<address<<" commit id:"<<commit_id;
  }
}

// ==========================================

bool TwoPLController::CheckCommit(int64_t commit_id) {
  const auto& change_set = changes_list_[commit_id];
  if (change_set.empty()) {
    LOG(ERROR) << " no commit id record found:" << commit_id;
    assert(!change_set.empty());
    return false;
  }

  for (const auto& it : change_set) {
    for (auto& op : it.second) {
      const auto& address = it.first;
      uint64_t v = storage_->GetVersion(it.first, false);
      if (op.state == STORE) {
        /*
          if(op.version+1 != v){
            LOG(ERROR)<<"state:"<<op.state<<" addr:"<<it.first<<"
          version:"<<op.version<<" db version:"<<v<<" address:"<<address<<"
          commit id:"<<commit_id; return false;
          }
          */
      } else {
        // LOG(ERROR)<<"state:"<<op.state<<" addr:"<<it.first<<"
        // version:"<<op.version<<" db version:"<<v<<" address:"<<address<<"
        // commit id:"<<commit_id;
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

bool TwoPLController::CommitUpdates(int64_t commit_id) {
#ifdef CDebug
  LOG(ERROR) << "check commit:" << commit_id
             << " abort:" << aborted_[commit_id];
#endif
  if (committed_[commit_id]) {
#ifdef CDebug
    LOG(ERROR) << " commit id committed:" << commit_id;
#endif
    assert(1 == 0);
  }
  committed_[commit_id] = true;
  {
    std::unique_lock lock(g_mutex_);

    assert(CheckCommit(commit_id));

    const auto& change_set = changes_list_[commit_id];
    if (change_set.empty()) {
      LOG(ERROR) << " no commit id record found:" << commit_id;
      assert(!change_set.empty());
      return false;
    }

    for (const auto& it : change_set) {
      bool done = false;
      for (int i = it.second.size() - 1; i >= 0 && !done; --i) {
        const auto& op = it.second[i];
        switch (op.state) {
          case LOAD:
            // LOG(ERROR)<<"load";
            break;
          case STORE:
            // LOG(ERROR)<<"commit addr:"<<it.first<<" size:"<<it.second.size();
            storage_->Store(it.first, op.data);
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
  }
  ReleaseLock(commit_id);
  done_.push_back(commit_id);
#ifdef CDebug
  LOG(ERROR) << "commit done:" << commit_id;
#endif
  return true;
}

bool TwoPLController::Commit(int64_t commit_id) {
#ifdef CDebug
  LOG(ERROR) << "commit id:" << commit_id;
#endif
  redo_.clear();
  done_.clear();
  finish_[commit_id] = true;
  is_redo_[commit_id] = false;
  bool ret = CommitUpdates(commit_id);
  if (!ret) {
    AddRedo(commit_id);
  }
  return true;
}

const std::vector<int64_t>& TwoPLController::GetRedo() { return redo_; }
const std::vector<int64_t>& TwoPLController::GetDone() { return done_; }

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb

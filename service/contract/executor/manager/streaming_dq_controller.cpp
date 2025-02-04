#include "service/contract/executor/manager/streaming_dq_controller.h"

#include <glog/logging.h>

#include <queue>
#include <stack>

//#define CDebug

namespace resdb {
namespace contract {
namespace streaming {

StreamingDQController::StreamingDQController(DataStorage* storage,
                                             int window_size)
    : ConcurrencyController(storage),
      window_size_(window_size),
      storage_(storage) {
  // ConcurrencyController(storage), window_size_(window_size),
  // storage_(static_cast<D_Storage*>(storage)){
  for (int i = 0; i < 32; i++) {
    if ((1 << i) > window_size_) {
      window_size_ = (1 << i) - 1;
      break;
    }
  }
  // LOG(ERROR)<<"init window size:"<<window_size_;
  Clear();
}

StreamingDQController::~StreamingDQController() {}

void StreamingDQController::Clear() {
  last_commit_id_ = 1;
  current_commit_id_ = 1;
  last_pre_commit_id_ = 0;

  assert(window_size_ + 1 <= 4196);
  // is_redo_.resize(window_size_+1);
  // is_done_.resize(window_size_+1);
  changes_list_.resize(window_size_ + 1);
  rechanges_list_.resize(window_size_ + 1);

  for (int i = 0; i < window_size_; ++i) {
    changes_list_[i].clear();
    rechanges_list_[i].clear();
    is_redo_[i] = 0;
    // is_done_[i] = false;
    wait_[i] = false;
    finish_[i] = false;
    committed_[i] = false;
  }

  for (int i = 0; i < 1024; ++i) {
    commit_list_[i].clear();
    pre_commit_list_[i].clear();
  }
}

void StreamingDQController::Clear(int64_t commit_id) {
  int id = commit_id & window_size_;
  wait_[id] = false;
  finish_[id] = false;
  committed_[id] = false;
  is_redo_[id] = false;
}

std::vector<int64_t>& StreamingDQController::GetRedo() { return redo_; }

std::vector<int64_t>& StreamingDQController::GetDone() { return done_; }

void StreamingDQController::CommitDone(int64_t commit_id) {
  committed_[commit_id & window_size_] = true;
  // LOG(ERROR)<<"commit done:"<<commit_id;
  done_.push_back(commit_id);
}

void StreamingDQController::PushCommit(int64_t commit_id,
                                       const ModifyMap& local_changes) {
  int idx = commit_id & window_size_;
  changes_list_[idx] = local_changes;
#ifdef CDebug
  LOG(ERROR) << "push commit:" << commit_id;
#endif
}

bool StreamingDQController::CheckCommit(int64_t commit_id) {
  const auto& change_set = changes_list_[commit_id & window_size_];
  if (change_set.empty()) {
    // LOG(ERROR)<<" no commit id record found:"<<commit_id;
    assert(!change_set.empty());
    return false;
  }

  for (const auto& it : change_set) {
    for (auto& op : it.second) {
      // LOG(ERROR)<<"commit id:"<<commit_id<<" state:"<<op.state<<"
      // address:"<<it.first<<" v:"<<op.version;
      if (op.state == LOAD) {
        uint64_t v = storage_->GetVersion(it.first, false);
#ifdef CDebug
        LOG(ERROR) << "op log:" << op.version << " db ver:" << v
                   << " address:" << it.first << " commit id:" << commit_id;
#endif
        if (op.version != v) {
#ifdef CDebug
          LOG(ERROR) << "op log:" << op.version << " db ver:" << v
                     << " address:" << it.first << " check fail"
                     << " commid id:" << commit_id;
#endif
          return false;
        }
      }
    }
  }
  return true;
}

bool StreamingDQController::PreCommitInternal(int64_t commit_id) {
#ifdef CDebug
  LOG(ERROR) << " check commit:" << commit_id << " last:" << last_commit_id_;
#endif
  wait_[commit_id] = false;
  if (last_commit_id_ != commit_id) {
    wait_[commit_id] = true;
    return true;
  }
  if (!CheckCommit(commit_id)) {
#ifdef CDebug
    LOG(ERROR) << "check commit fail:" << commit_id;
#endif
    return false;
  }

  const auto& change_set = changes_list_[commit_id & window_size_];
  if (change_set.empty()) {
    LOG(ERROR) << " no commit id record found:" << commit_id;
    assert(1 == 0);
    return false;
  }

  std::set<int64_t> new_commit_ids;
  for (const auto& it : change_set) {
    const uint256_t& address = it.first;
    // LOG(ERROR)<<"get pre-op:"<<address;
    bool done = false;
    for (int i = it.second.size() - 1; i >= 0 && !done; --i) {
      const auto& op = it.second[i];
      switch (op.state) {
        case LOAD:
          break;
        case STORE:
#ifdef CDebug
          LOG(ERROR) << "commit:" << it.first << " data:" << op.data
                     << " commit id:" << commit_id << " version:" << op.version;
#endif
          // storage_->Reset(it.first, op.data, op.version, false);
          storage_->Store(it.first, op.data);
          done = true;
          break;
        case REMOVE:
          // LOG(ERROR)<<"remove:"<<it.first<<" data:"<<op.data<<" commit
          // id:"<<commit_id<<" version:"<<op.version;
          // LOG(ERROR)<<"remove:"<<it.first<<" data:";
          // storage_->Remove(it.first, true);
          storage_->Reset(it.first, 0, op.version, false);
          done = true;
          break;
      }
    }
  }

  CommitDone(commit_id);
  last_commit_id_++;
  return true;
}

void StreamingDQController::RedoCommit(int64_t commit_id, int flag) {
  if (is_redo_[commit_id]) {
    return;
  }
  is_redo_[commit_id] = true;
#ifdef CDebug
  LOG(ERROR) << "====== add redo:" << commit_id;
#endif
  redo_.push_back(commit_id);
}

bool StreamingDQController::PreCommit(int64_t commit_id) {
  bool ret = PreCommitInternal(commit_id);
  if (!ret) {
    RedoCommit(commit_id, 0);
  }
#ifdef CDebug
  LOG(ERROR) << "pre commit done:" << ret << " next:" << last_commit_id_
             << " ret:" << ret;
#endif
  return ret;
}

bool StreamingDQController::Commit(int64_t commit_id) {
#ifdef CDebug
  LOG(ERROR) << "commit :" << commit_id;
#endif
  redo_.clear();
  done_.clear();
  is_redo_[commit_id & window_size_] = false;
  finish_[commit_id & window_size_] = true;
  // wait_[commit_id&window_size_] = true;
  // LOG(ERROR)<<"commit:"<<commit_id<<" idx:"<<(commit_id&window_size_);
  // assert(is_done_[commit_id&window_size_] ==false);
  if (!PreCommit(commit_id)) {
    return false;
  }

  if (wait_[commit_id & window_size_]) {
    // AddWait(commit_id);
#ifdef CDebug
    LOG(ERROR) << " commit id:" << commit_id << " is waiting:";
#endif
    return true;
  }

#ifdef CDebug
  LOG(ERROR) << " get next:" << last_commit_id_
             << " wait;:" << wait_[last_commit_id_];
#endif
  while (true) {
    int cur_id = last_commit_id_;
    if (wait_[cur_id & window_size_] == false) {
      break;
    }
    bool ret = PreCommit(cur_id);
    if (!ret) {
      break;
    }
    if (cur_id == last_commit_id_) {
      break;
    }
  }
  // LOG(ERROR)<<"current commit id:"<<current_commit_id_<<" last commit
  // id:"<<last_commit_id_;

  return true;
}

}  // namespace streaming
}  // namespace contract
}  // namespace resdb

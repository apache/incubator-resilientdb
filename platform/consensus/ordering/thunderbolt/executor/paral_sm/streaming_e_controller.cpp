#include "service/contract/executor/x_manager/streaming_e_controller.h"

#include <glog/logging.h>

#include <queue>
#include <stack>

#include "common/utils/utils.h"

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
  return v % 128;
}

void AppendRecord(const uint256_t& address, int64_t commit_id,
                  StreamingEController::CommitList* commit_list) {
  int hash_idx = GetHashKey(address);
  auto& commit_set = commit_list[hash_idx][address];
  if (!commit_set.empty() && commit_set.back() > commit_id) {
    // LOG(ERROR)<<"append to old data:"<<commit_id<<"
    // back:"<<commit_set.back();
  }
  assert(commit_set.empty() || commit_set.back() < commit_id);
  commit_set.push_back(commit_id);
  return;
}

int64_t RemoveRecord(const uint256_t& address, int64_t commit_id,
                     StreamingEController::CommitList* commit_list) {
  int idx = GetHashKey(address);
  auto it = commit_list[idx].find(address);
  if (it == commit_list[idx].end()) {
    // LOG(ERROR)<<" remove address:"<<address<<" commit id:"<<commit_id<<" not
    // exist";
    return -3;
  }

  auto& set_it = commit_list[idx][address];
  if (set_it.front() != commit_id) {
    // LOG(ERROR)<< "address id::"<<commit_id<<" front:"<<set_it.front();
    return -2;
  }
  // LOG(ERROR)<< "address id::"<<commit_id<<" address:"<<address;

  set_it.erase(set_it.begin());
  if (set_it.empty()) {
    // LOG(ERROR)<<"erase idx:"<<idx<<" address:"<<address;
    commit_list[idx].erase(it);
    return -1;
  }
  return set_it.front();
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

  assert(window_size_ + 1 <= 1024);
  // is_redo_.resize(window_size_+1);
  // is_done_.resize(window_size_+1);
  changes_list_.resize(window_size_ + 1);
  rechanges_list_.resize(window_size_ + 1);

  for (int i = 0; i < window_size_; ++i) {
    changes_list_[i].clear();
    rechanges_list_[i].clear();
    is_redo_[i] = 0;
    // is_done_[i] = false;
  }

  for (int i = 0; i < 1024; ++i) {
    commit_list_[i].clear();
    pre_commit_list_[i].clear();
  }
}

std::vector<int64_t>& StreamingEController::GetRedo() { return redo_; }

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
  Data data = Data(STORE, value);
  AppendPreRecord(address, commit_id, data, version);
  // LOG(ERROR)<<"store data commit id:"<<commit_id<<" addr:"<<address<<"
  // value:"<<data.data<<" ver:"<<data.version;
}

uint256_t StreamingEController::LoadInternal(const int64_t commit_id,
                                             const uint256_t& address,
                                             int version) {
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

void StreamingEController::AppendPreRecord(const uint256_t& address,
                                           int64_t commit_id, Data& data,
                                           int version) {
  data.commit_version = version;
  // LOG(ERROR)<<"append record addr:"<<address<<" commit id:"<<commit_id;
  {
    int hash_idx = GetHashKey(address);
    // std::lock_guard<std::mutex> lk(mutex_[hash_idx]);
    std::unique_lock lock(mutex_[hash_idx]);

    auto& commit_set = pre_commit_list_[hash_idx][address];
    auto it = commit_set.find(commit_id);
    if (it == commit_set.end() || it->second.commit_version != version) {
      it = commit_set.insert(std::make_pair(commit_id, Data())).first;
      if (it == commit_set.begin()) {
        auto ret = storage_->Load(address, true);
        data.old_data = ret.first;
        data.version = ret.second;
        // LOG(ERROR)<<"get from db:"<<ret.second;
      } else {
        auto pt = it;
        --pt;
        data.old_data = pt->second.data;
        data.version = pt->second.version;
        // LOG(ERROR)<<"get from pre:"<<pt->first<<" ver:"<<pt->second.version;
      }
    } else {
      data.old_data = it->second.data;
      data.version = it->second.version;
      // LOG(ERROR)<<"get from self:"<<it->first<<" ver:"<<it->second.version<<"
      // commit ver:"<<it->second.commit_version;
    }

    if (data.state == LOAD) {
      data.data = data.old_data;
    } else {
      data.version += 1;
    }
    // LOG(ERROR)<<"!!!!! append id:"<<commit_id<<" state:"<<data.state<<"
    // value:"<<data.data<<" ver:"<<data.version<<" addr:"<<address<<" commit
    // version:"<<data.commit_version;

    it->second = data;
  }

  int idx = commit_id & window_size_;
  auto& change_set = changes_list_[idx][address];
  if (change_set.empty()) {
    change_set.push_back(data);
  } else {
    int old_version = change_set.front().commit_version;
    if (old_version != version) {
      for (auto it : changes_list_[idx]) {
        if (rechanges_list_[idx].find(it.first) == rechanges_list_[idx].end()) {
          rechanges_list_[idx][it.first].push_back(it.second.front());
        }
      }
      changes_list_[idx].clear();
    }

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
  }
  return;
}

// ==========================================

void StreamingEController::AppendPreRecord(
    const uint256_t& address, int hash_idx,
    const std::vector<int64_t>& commit_id) {
  // std::lock_guard<std::mutex> lk(mutex_[hash_idx]);
  std::unique_lock lock(mutex_[hash_idx]);
  auto& commit_set = pre_commit_list_[hash_idx][address];
  for (auto id : commit_id) {
    // LOG(ERROR)<<"roll back id:"<<id<<" idx:"<<hash_idx<<" address:"<<address;
    auto& op_set = commit_set[id];
    op_set = changes_list_[id & window_size_][address].back();
  }
}

int64_t StreamingEController::RemovePrecommitRecord(const uint256_t& address,
                                                    int64_t commit_id) {
  int idx = GetHashKey(address);
  // std::lock_guard<std::mutex> lk(mutex_[idx]);
  std::unique_lock lock(mutex_[idx]);
  auto it = pre_commit_list_[idx].find(address);
  if (it == pre_commit_list_[idx].end()) {
    // LOG(ERROR)<<" remove address:"<<address<<" commit id:"<<commit_id<<" not
    // exist";
    return -3;
  }

  auto& set_it = pre_commit_list_[idx][address];
  if (set_it.begin()->first != commit_id) {
    // LOG(ERROR)<< "address id::"<<commit_id<<" front:"<<set_it.begin()->first;
    return -2;
  }
  // LOG(ERROR)<< "address id::"<<commit_id<<" address:"<<address;

  set_it.erase(set_it.begin());
  if (set_it.empty()) {
    // LOG(ERROR)<<"erase idx:"<<idx<<" address:"<<address;
    pre_commit_list_[idx].erase(it);
    return -1;
  }
  return set_it.begin()->first;
}

void StreamingEController::Remove(int64_t commit_id) {
  // LOG(ERROR)<<"remove:"<<commit_id;
  int idx = commit_id & window_size_;
  changes_list_[idx].clear();
  rechanges_list_[idx].clear();
  is_redo_[idx] = 0;
}

void StreamingEController::CleanOldData(int64_t commit_id) {
  int idx = commit_id & window_size_;
  std::set<int64_t> new_commit_ids;
  for (auto it : rechanges_list_[idx]) {
    const uint256_t& addr = it.first;
    int64_t next_commit_id = RemovePrecommitRecord(addr, commit_id);

    // LOG(ERROR)<<"remove id:"<<commit_id<<" next_commit_id:"<<next_commit_id;
    if (next_commit_id > 0 && next_commit_id <= last_pre_commit_id_) {
      // LOG(ERROR)<<"commit id:"<<commit_id<<" trigger:"<<next_commit_id;
      assert(commit_id < next_commit_id);
      new_commit_ids.insert(next_commit_id);
    }
  }
  for (int64_t redo_commit : new_commit_ids) {
    RedoCommit(redo_commit, 1);
  }
  rechanges_list_[idx].clear();
}

void StreamingEController::RedoCommit(int64_t commit_id, int flag) {
  int idx = commit_id & window_size_;

  if (is_redo_[idx] == 1) {
    // LOG(ERROR)<<"commit id has been redo:"<<commit_id;
    return;
  }

  if (is_redo_[idx] == flag + 1) {
    // LOG(ERROR)<<"commit id has been redo:"<<commit_id<<" flag:"<<flag;
    return;
  }

  is_redo_[commit_id & window_size_] = flag + 1;
  // LOG(ERROR)<<"add redo:"<<commit_id<<"flag:"<<flag;
  redo_.push_back(commit_id);
}

bool StreamingEController::CheckFirstFromCommit(const uint256_t& address,
                                                int64_t commit_id) {
  int idx = GetHashKey(address);
  const auto& it = commit_list_[idx].find(address);
  if (it == commit_list_[idx].end()) {
    // LOG(ERROR)<<"commit list empty:";
    return false;
  }
  const auto& commit_set = it->second;
  if (commit_set.empty()) {
    // LOG(ERROR)<<"commit set is empty:"<<commit_id<<" is pre:0
    // addr:"<<address;
  }
  assert(!commit_set.empty());

  if (commit_set.front() < commit_id) {
    // not the first candidate
    // LOG(ERROR)<<"still have dep. commit id:"<<commit_id<<"
    // dep:"<<commit_set.front()<<" addr:"<<address;
    return false;
  }
  if (commit_set.front() > commit_id) {
    // LOG(ERROR)<<"set header:"<<commit_set.front()<<" current:"<<commit_id<<"
    // is pre: 0";
    return false;
  }
  assert(commit_set.front() == commit_id);
  return true;
}

bool StreamingEController::CheckFirstFromPreCommit(const uint256_t& address,
                                                   int64_t commit_id) {
  // LOG(ERROR)<<"check first:"<<commit_id;
  int idx = GetHashKey(address);
  // std::lock_guard<std::mutex> lk(mutex_[idx]);
  std::shared_lock lock(mutex_[idx]);
  const auto& it = pre_commit_list_[idx].find(address);
  if (it == pre_commit_list_[idx].end()) {
    // LOG(ERROR)<<"no address:"<<commit_id<<" add:"<<address;
    assert(1 == 0);
    return false;
  }
  const auto& commit_set = it->second;
  if (commit_set.empty()) {
    // LOG(ERROR)<<"commit set is empty:"<<commit_id<<" is pre: 1
    // addr:"<<address;
  }
  assert(!commit_set.empty());

  if (commit_set.begin()->first < commit_id) {
    // not the first candidate
    // LOG(ERROR)<<"still have dep. commit id:"<<commit_id<<"
    // dep:"<<commit_set.begin()->first<<" address:"<<address;
    return false;
  }
  if (commit_set.begin()->first > commit_id) {
    // LOG(ERROR)<<"set header:"<<commit_set.begin()->first<<"
    // current:"<<commit_id<<" is pre: 1";
  }
  assert(commit_set.begin()->first == commit_id);
  return true;
}

bool StreamingEController::CheckCommit(int64_t commit_id, bool is_pre) {
  int idx = commit_id & window_size_;
  // LOG(ERROR)<<"check commit "<<idx;
  const auto& change_set = changes_list_[commit_id & window_size_];
  if (change_set.empty()) {
    // LOG(ERROR)<<" no commit id record found:"<<commit_id;
    assert(!change_set.empty());
    return false;
  }

  for (const auto& it : change_set) {
    if (is_pre) {
      if (!CheckFirstFromPreCommit(it.first, commit_id)) {
        // LOG(ERROR)<<"not the first one:"<<commit_id;
        return false;
      }
    } else {
      if (!CheckFirstFromCommit(it.first, commit_id)) {
        // LOG(ERROR)<<"not the first one:"<<commit_id;
        return false;
      }
    }
  }

  if (is_pre) {
    if (!CheckPreCommit(commit_id)) {
      // LOG(ERROR)<<"check pre commit fail??:"<<commit_id;
      RedoConflict(commit_id);

      for (const auto& it : change_set) {
        if (!CheckFirstFromPreCommit(it.first, commit_id)) {
          // LOG(ERROR)<<"not the first one:"<<commit_id;
          return false;
        }
      }
    }
  }
  if (is_pre) {
    for (const auto& it : change_set) {
      for (auto& op : it.second) {
        // LOG(ERROR)<<"state:"<<op.state<<" addr:"<<it.first;
        if (op.state == LOAD) {
          uint64_t v = storage_->GetVersion(it.first, is_pre);
          // LOG(ERROR)<<"op log:"<<op.version<<" db ver:"<<v<<" is
          // pre:"<<is_pre<<" address:"<<it.first<<"
          // version:"<<op.commit_version;
          if (op.version != v) {
            // LOG(ERROR)<<"op log:"<<op.version<<" db ver:"<<v<<" is
            // pre:"<<true<<" address:"<<it.first<<" check fail"<<" commit
            // ver:"<<op.commit_version;
            if (is_pre == 0) {
              assert(1 == 0);
            }
            RedoCommit(commit_id, 0);
            return false;
          }
        } else {
          break;
        }
      }
    }
  }
  return true;
}

void StreamingEController::RollBackData(const uint256_t& address,
                                        const Data& data) {
  // LOG(ERROR)<<" roll back:"<<address<<" version:"<<data.version<<"
  // value:"<<data.data<<" old value:"<<data.old_data;
  if (data.state == LOAD) {
    storage_->Reset(address, data.data, data.version, true);
  } else {
    storage_->Reset(address, data.old_data, data.version, true);
  }
}

void StreamingEController::RedoConflict(int64_t commit_id) {
  // LOG(ERROR)<<"move conflict id:"<<commit_id;

  std::priority_queue<int64_t, std::vector<int64_t>, std::greater<int64_t>> q;
  std::set<int64_t> v;
  q.push(commit_id);
  v.insert(commit_id);

  while (!q.empty()) {
    int64_t cur_id = q.top();
    q.pop();
    const auto& change_set = changes_list_[cur_id & window_size_];

    for (const auto& it : change_set) {
      const uint256_t& address = it.first;
      int hash_idx = GetHashKey(address);
      if (commit_list_[hash_idx].find(address) ==
          commit_list_[hash_idx].end()) {
        continue;
      }
      // LOG(ERROR)<<"check commit id:"<<cur_id<<" data
      // version:"<<it.second.begin()->version;
      auto& commit_set = commit_list_[hash_idx][address];

      std::vector<int64_t> back_list;
      while (!commit_set.empty() && commit_set.back() >= cur_id) {
        int64_t back_id = commit_set.back();
        commit_set.pop_back();

        back_list.push_back(back_id);
        if (v.find(back_id) == v.end()) {
          q.push(back_id);
          v.insert(back_id);
        }
        if (back_id == cur_id) {
          break;
        }
      }
      // LOG(ERROR)<<"hahs list size:"<<back_list.size()<<" commit set
      // size:"<<commit_set.size();
      if (commit_set.empty()) {
        commit_list_[hash_idx].erase(commit_list_[hash_idx].find(address));
      }

      if (!back_list.empty()) {
        // LOG(ERROR)<<"roll back from:"<<back_list.back();
        const auto& first_change_set =
            changes_list_[back_list.back() & window_size_];
        RollBackData(address, *first_change_set.find(address)->second.begin());
      }
      if (cur_id == commit_id) {
        back_list.push_back(commit_id);
      }
      AppendPreRecord(address, hash_idx, back_list);
    }
  }
}

bool StreamingEController::CheckPreCommit(int64_t commit_id) {
  // LOG(ERROR)<<"check pre commit:"<<commit_id;
  const auto& change_set = changes_list_[commit_id & window_size_];
  if (change_set.empty()) {
    // LOG(ERROR)<<" no commit id record found:"<<commit_id;
    assert(!change_set.empty());
    return false;
  }

  for (const auto& it : change_set) {
    const uint256_t& address = it.first;
    int hash_idx = GetHashKey(address);
    if (commit_list_[hash_idx].find(address) == commit_list_[hash_idx].end()) {
      continue;
    }
    auto& commit_set = commit_list_[hash_idx][address];
    if (!commit_set.empty()) {
      // LOG(ERROR)<<"address front:"<<commit_set.front()<<"
      // back:"<<commit_set.back();
    }
    if (!commit_set.empty() && commit_set.back() > commit_id) {
      return false;
    }
  }
  return true;
}

bool StreamingEController::PreCommit(int64_t commit_id) {
  // LOG(ERROR)<<" precommit :"<<commit_id;
  if (!CheckCommit(commit_id, true)) {
    // LOG(ERROR)<<"check commit fail??";
    return false;
  }

  const auto& change_set = changes_list_[commit_id & window_size_];
  if (change_set.empty()) {
    // LOG(ERROR)<<" no commit id record found:"<<commit_id;
    assert(1 == 0);
    return false;
  }
  // LOG(ERROR)<<"check done";

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
          // LOG(ERROR)<<"commit:"<<it.first<<" data:"<<op.data<<" commit
          // id:"<<commit_id;
          storage_->StoreWithVersion(it.first, op.data, op.version, true);
          done = true;
          break;
        case REMOVE:
          // LOG(ERROR)<<"remove:"<<it.first<<" data:";
          storage_->Remove(it.first, true);
          done = true;
          break;
      }
    }

    // LOG(ERROR)<<"append id:"<<commit_id<<" to commit list";
    AppendRecord(address, commit_id, commit_list_);

    int64_t next_commit_id = RemovePrecommitRecord(address, commit_id);
    // LOG(ERROR)<<"next_commit_id:"<<next_commit_id;
    assert(next_commit_id >= -1);
    if (next_commit_id > 0 && next_commit_id <= last_pre_commit_id_) {
      // LOG(ERROR)<<"commit id:"<<commit_id<<" trigger:"<<next_commit_id;
      assert(commit_id < next_commit_id);
      new_commit_ids.insert(next_commit_id);
    }
    /*
    if(next_commit_id>last_pre_commit_id_){
      assert(1==0);
    }
    */
  }

  for (int64_t redo_commit : new_commit_ids) {
    RedoCommit(redo_commit, 1);
  }
  // LOG(ERROR)<<"done:"<<commit_id;
  CleanOldData(commit_id);

  if (precommit_callback_) {
    precommit_callback_(commit_id);
  }
  return true;
}

// ================= post commit =======================

void StreamingEController::CommitDone(int64_t commit_id) {
  // LOG(ERROR)<<"commit done:"<<commit_id;
  done_.push_back(commit_id);
}

// move id from commitlist back to precommitlist

bool StreamingEController::PostCommit(int64_t commit_id) {
  // LOG(ERROR)<<"post commit:"<<commit_id;
  const auto& change_set = changes_list_[commit_id & window_size_];
  if (change_set.empty()) {
    // LOG(ERROR)<<"no data";
    return false;
  }

  if (!CheckCommit(commit_id, false)) {
    // LOG(ERROR)<<"check commit fail:"<<commit_id;
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
    RemoveRecord(address, commit_id, commit_list_);
  }

  CommitDone(commit_id);
  Remove(commit_id);
  assert(last_commit_id_ == commit_id);
  last_commit_id_++;
  return true;
}

bool StreamingEController::Commit(int64_t commit_id) {
  redo_.clear();
  done_.clear();
  is_redo_[commit_id & window_size_] = false;
  // LOG(ERROR)<<"======================     commit id:"<<commit_id<<"
  // last_pre_commit_id_:"<<last_pre_commit_id_;
  bool ret = PreCommit(commit_id);

  if (commit_id == last_pre_commit_id_ + 1) {
    last_pre_commit_id_++;
  }

  if (!ret) {
    return false;
  }

  while (current_commit_id_ <= last_commit_id_) {
    bool ret = PostCommit(current_commit_id_);
    if (!ret) {
      break;
    }
    current_commit_id_++;
  }
  return true;
}

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb

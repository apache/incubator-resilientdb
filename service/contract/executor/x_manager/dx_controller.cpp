#include "service/contract/executor/x_manager/dx_controller.h"

#include <glog/logging.h>

#include <queue>
#include <stack>

#include "common/utils/utils.h"
#include "eEVM/exception.h"

//#define CDebug
//#define DDebug

namespace resdb {
namespace contract {
namespace x_manager {
namespace {
int GetHashKey(const uint256_t& address) {
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
  return v % 2048;
}
}  // namespace

DXController::DXController(DataStorage* storage, int window_size)
    : ConcurrencyController(storage),
      window_size_(window_size),
      storage_(storage) {
  for (int i = 0; i < 32; i++) {
    if ((1 << i) > window_size_) {
      window_size_ = (1 << i) - 1;
      break;
    }
  }
  assert(window_size_ + 1 <= 8192);
  changes_list_.resize(window_size_ + 1);
  addr_changes_list_.resize(window_size_ + 1);
  // LOG(ERROR)<<"init window size:"<<window_size_;
  Clear();
}

DXController::~DXController() {}

void DXController::Clear() {
  for (int i = 0; i < window_size_; ++i) {
    wait_.push_back(false);
    aborted_[i] = false;
    finish_[i] = false;
    committed_[i] = false;
    has_write_[i] = false;
    reach_[i].reset();
    reach_[i].set(i, true);
    key_[i].clear();

    addr_pre_[i].clear();
    addr_child_[i].clear();
    root_addr_[i].clear();
    lastr_.clear();
    lastw_.clear();
    pre_[i].clear();
    child_[i].clear();
    is_redo_[i] = true;
    changes_list_[i].clear();
    addr_changes_list_[i].clear();
    commit_time_[i] = 0;
  }

  key_id_ = 1;

  check_.clear();
  check_abort_.clear();
  ds_ = false;
  post_abort_.clear();
  pending_check_.clear();
  akey_.clear();
  pd_.clear();
  redo_.clear();
  done_.clear();
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
std::unique_ptr<ModifyMap> DXController::GetChangeList(int64_t commit_id) {
  // read lock
  // return std::move(changes_list_[commit_id]);
  return nullptr;
}

void DXController::Clear(int64_t commit_id) {
#ifdef CDebug
  LOG(ERROR) << "CLEAR id:" << commit_id;
#endif
  std::unique_lock lock(g_mutex_);
  changes_list_[commit_id].clear();
  addr_changes_list_[commit_id].clear();

  wait_[commit_id] = false;
  aborted_[commit_id] = false;
  finish_[commit_id] = false;
  committed_[commit_id] = false;
  has_write_[commit_id] = false;
  reach_[commit_id].reset();
  reach_[commit_id].set(commit_id, true);
  commit_time_[commit_id] = 0;

  /*
  throw eevm::Exception(
      eevm::Exception::Type::outOfGas,
      "Get lock fail");
      */
}

void DXController::StoreInternal(const int64_t commit_id,
                                 const uint256_t& address,
                                 const uint256_t& value, int version) {
  // LOG(ERROR)<<"store:"<<commit_id<<" is abort:"<<aborted_[commit_id];
  if (aborted_[commit_id]) {
    throw eevm::Exception(eevm::Exception::Type::outOfGas, "Get lock fail");
    return;
  }
  Data data = Data(STORE, value);
  AppendPreRecord(address, commit_id, data);
  if (aborted_[commit_id]) {
    throw eevm::Exception(eevm::Exception::Type::outOfGas, "Get lock fail");
    return;
  }
#ifdef CDebug
  // LOG(ERROR)<<"store data commit id:"<<commit_id<<" addr:"<<address<<"
  // value:"<<data.data<<" ver:"<<data.version;
#endif
}

uint256_t DXController::LoadInternal(const int64_t commit_id,
                                     const uint256_t& address, int version) {
  if (aborted_[commit_id]) {
    throw eevm::Exception(eevm::Exception::Type::outOfGas, "Get lock fail");
    return 0;
  }
  Data data = Data(LOAD);
  // int64_t time = GetCurrentTime();
  AppendPreRecord(address, commit_id, data);
  if (aborted_[commit_id]) {
    throw eevm::Exception(eevm::Exception::Type::outOfGas, "Get lock fail");
    return 0;
  }
#ifdef CDebug
// LOG(ERROR)<<"load data commit id:"<<commit_id<<" addr:"<<address<<"
// value:"<<data.data<<" ver:"<<data.version;
#endif
  return data.data;
}

void DXController::Store(const int64_t commit_id, const uint256_t& address,
                         const uint256_t& value, int version) {
  StoreInternal(commit_id, address, value, version);
}

uint256_t DXController::Load(const int64_t commit_id, const uint256_t& address,
                             int version) {
  return LoadInternal(commit_id, address, version);
}

bool DXController::Remove(const int64_t commit_id, const uint256_t& key,
                          int version) {
  Store(commit_id, key, 0, version);
  return true;
}

int DXController::AddressToId(const uint256_t& key) {
  int key_idx = GetHashKey(key);

  std::unique_lock lock(k_mutex_[key_idx]);
  if (key_[key_idx].find(key) == key_[key_idx].end()) {
    {
      // std::unique_lock lockx(abort_mutex_);
      // akey_[key_id_] = key;
      key_[key_idx][key] = key_id_++;
    }
  }
  return key_[key_idx][key];
}

uint256_t& DXController::GetAddress(int key) {
  // int key_idx = GetHashKey(key);
  std::unique_lock lockx(abort_mutex_);
  return akey_[key];
}

void DXController::AddRedo(int64_t commit_id) {
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

void DXController::AddDataToChangeList(std::vector<Data>& change_set,
                                       const Data& data) {
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

int DXController::GetLastR(const uint256_t& address, int addr_idx) {
#ifdef CDebug
  LOG(ERROR) << "get last r, lp size:" << lastr_[addr_idx].size()
             << " address:" << address << " addr idx:" << addr_idx;
#endif
  auto it = lastr_.find(addr_idx);
  if (it == lastr_.end()) {
    // LOG(ERROR)<<"no last:"<<address;
    if (lastw_.find(addr_idx) == lastw_.end()) {
      return -1;
    }
    return *lastw_[addr_idx].begin();
  }
  const auto& lp = it->second;
  if (lp.empty()) {
    // LOG(ERROR)<<"no last:"<<address;
    if (lastw_.find(addr_idx) == lastw_.end()) {
      return -1;
    }
    return *lastw_[addr_idx].begin();
  }
  return *lp.begin();

  int ret1 = -1, ret2 = -1, ret3 = -1;
  for (int idx : lp) {
    auto& change_set = changes_list_[idx][address];
    if (committed_[idx]) {
      LOG(ERROR) << "return commitid parent:" << idx;
      //      ret3 = idx;
    }
    // assert(change_set.size());
    if (change_set.size() == 1) {
      if (change_set.back().state == LOAD) {
        //    LOG(ERROR)<<"get from read idx:"<<idx;
        if (ret1 == -1) {
          ret1 = idx;
        } else if (ret1 != 0 &&
                   changes_list_[idx].size() < changes_list_[ret1].size()) {
          ret1 = idx;
        }
      }
    }
    if (change_set.size() == 2) {
      //   LOG(ERROR)<<"get from read/write idx:"<<idx;
      if (ret2 == -1) {
        ret2 = idx;
      } else if (ret2 != 0 &&
                 changes_list_[idx].size() < changes_list_[ret2].size()) {
        ret2 = idx;
      }
    }
    if (committed_[idx]) {
      LOG(ERROR) << "return commitid parent:" << idx;
      return idx;
    }
  }
  if (ret1 != -1) return ret1;
  if (ret2 != -1) return ret2;
  // if(ret3 != -1) return ret3;

  for (int idx : lastw_[addr_idx]) {
    auto& change_set = changes_list_[idx][address];
    if (change_set.size() == 1) {
      //  LOG(ERROR)<<"get from write idx:"<<idx;
      return idx;
    }
  }
  return -1;
}

int DXController::GetLastW(const uint256_t& address, int addr_idx) {
#ifdef CDebug
  LOG(ERROR) << "get last w, lp size:" << lastw_[addr_idx].size()
             << " address:" << address << " addr idx:" << addr_idx;
#endif
  // return -1;
  auto it = lastw_.find(addr_idx);
  if (it == lastw_.end()) {
    // LOG(ERROR)<<"no last:"<<address;
    if (ds_ == false || lastr_.find(addr_idx) == lastr_.end()) {
      return -1;
    }
    return *lastr_[addr_idx].begin();
  }
  const auto& lp = it->second;
  if (lp.empty()) {
    // LOG(ERROR)<<"no last:"<<address;
    if (ds_ == false || lastr_.find(addr_idx) == lastr_.end()) {
      return -1;
    }
    return *lastr_[addr_idx].begin();
  }
  return *lp.begin();

  int ret1 = -1, ret2 = -1, ret3 = -1, ret4 = -1;
  for (int idx : lp) {
    auto& change_set = changes_list_[idx][address];
    //#ifdef CDebug
    if (change_set.size() == 0) {
      //   LOG(ERROR)<<"get last idx:"<<idx<<" address:"<<address<<" change set
      //   size:"<<change_set.size();
    }
    // assert(change_set.size());
    //#endif
    if (committed_[idx]) {
      // LOG(ERROR)<<"return commitid parent:"<<idx;
      // return idx;
      ret3 = idx;
      //   continue;
    }

    if (change_set.size() == 1) {
      if (change_set.back().state == STORE) {
        //    LOG(ERROR)<<"get from write idx:"<<idx;
        if (ret1 == -1) {
          ret1 = idx;
        } else if (ret1 != 0 &&
                   changes_list_[idx].size() < changes_list_[ret1].size()) {
          ret1 = idx;
        }
      } else {
        if (idx != 0) {
          if (pre_[idx].empty()) {
            //  ret4 = 0;
          }
          // ret4 = *addr_pre_[idx][address].begin();
        }
      }
    }
    if (change_set.size() == 2) {
      //   LOG(ERROR)<<"get from r/write idx:"<<idx;
      if (ret2 == -1) {
        ret2 = idx;
      } else if (ret2 != 0 &&
                 changes_list_[idx].size() < changes_list_[ret2].size()) {
        ret2 = idx;
      }
    }
  }

  if (ret2 != -1) return ret2;
  if (ret1 != -1) return ret1;
  // if(ret3 != -1) return ret3;
  // return -1;
  //  if(ret4 != -1) return ret4;

  for (int idx : lastr_[addr_idx]) {
    auto& change_set = changes_list_[idx][address];
    if (change_set.size() == 1) {
      //  LOG(ERROR)<<"get from ridx:"<<idx;
      return idx;
    }
  }
  return -1;
}

int DXController::GetLastWOnly(const uint256_t& address, int addr_idx) {
  return GetLastW(address, addr_idx);
}

void DXController::Abort(int64_t commit_id) {
  if (aborted_[commit_id]) {
    return;
  }
  aborted_[commit_id] = true;
  // LOG(ERROR)<<"abort :"<<commit_id<<" is finish:"<<finish_[commit_id];
  if (finish_[commit_id]) {
    post_abort_.push_back(commit_id);
  }
}

void DXController::RecursiveAbort(int64_t idx, const uint256_t& addr) {
#ifdef CDebug
  LOG(ERROR) << " check abort idx:" << idx << " addr:" << addr;
#endif
  if (check_[idx].find(addr) != check_[idx].end()) {
    // LOG(ERROR)<<" check abort idx:"<<idx<<" addr:"<<addr<<" repeated";
    return;
  }

  check_[idx].insert(addr);

  bool need_abort = false;

  const auto& cs = changes_list_[idx][addr];
  assert(cs.size());
  if (cs.front().state == LOAD) {
    need_abort = true;
  }

  if (need_abort) {
    check_abort_.insert(idx);
    for (auto it : changes_list_[idx]) {
      const uint256_t& sub_addr = it.first;
      // LOG(ERROR)<<" abort idx:"<<idx<<" from:"<<sub_addr;
      for (int ch : addr_child_[idx][sub_addr]) {
        RecursiveAbort(ch, sub_addr);
      }
    }
  }
}

void DXController::AbortNodeFrom(int64_t idx, const uint256_t& addr) {
#ifdef CDebug
  LOG(ERROR) << " ======================== abort from idx:" << idx
             << " address:" << addr
             << " child size:" << addr_child_[idx].size();
#endif

  if (addr_child_[idx].empty() ||
      addr_child_[idx].find(addr) == addr_child_[idx].end()) {
    return;
  }
  check_abort_.clear();
  check_.clear();
  for (auto c : addr_child_[idx][addr]) {
    RecursiveAbort(c, addr);
  }

  for (auto nd : check_abort_) {
#ifdef CDebug
    LOG(ERROR) << "abort node:" << nd << " from:" << idx;
#endif
    // LOG(ERROR)<<"abort node:"<<nd<<" from:"<<idx<<" addr:"<<addr;
    Abort(nd);
    RemoveNode(nd);
  }
}

void DXController::AbortNode(int64_t idx) {
  // LOG(ERROR)<<" ======================== abort node idx:"<<idx;

  check_abort_.clear();
  check_.clear();

  for (auto it : changes_list_[idx]) {
    auto& change_set = it.second;
    if (change_set.empty() || change_set.back().state == LOAD) {
      continue;
    }

    for (auto c : addr_child_[idx][it.first]) {
      RecursiveAbort(c, it.first);
    }
  }

  check_abort_.insert(idx);
  for (auto nd : check_abort_) {
#ifdef CDebug
    LOG(ERROR) << "abort node:" << nd << " from:" << idx;
#endif
    Abort(nd);
    RemoveNode(nd);
  }
}

int DXController::GetDep(int from, int to) {
  LOG(ERROR) << "get dep:"
             << "fromm:" << from << " to:" << to;
  if (to == from) {
    return 0;
  }

  int md = 0;
  for (int f : pre_[to]) {
    int ret = GetDep(from, f);
    md = std::max(md, ret + 1);
  }
  return md;
}

void DXController::ResetReach(int64_t commit_id) {
  reach_[commit_id].reset();
  reach_[commit_id].set(commit_id, true);
  for (int c : child_[commit_id]) {
    reach_[commit_id] |= reach_[c];
    // LOG(ERROR)<<" get commit :"<<commit_id<<" c:"<<reach_[c];
  }
  // LOG(ERROR)<<"get reach commit:"<<commit_id<<" reach:"<<reach_[commit_id];
}

void DXController::RemoveNode(int64_t commit_id) {
#ifdef CDebug
  LOG(ERROR) << "remove node:" << commit_id;
#endif
  // LOG(ERROR)<<"remove node:"<<commit_id;

  for (int f : pre_[commit_id]) {
    for (int c : child_[commit_id]) {
      child_[f].insert(c);
      pre_[c].insert(f);
#ifdef CDebug
      LOG(ERROR) << "remove node pre:" << f << " child:" << c
                 << " commit id:" << commit_id;
#endif
      assert(c != f);
    }
    // LOG(ERROR)<<"find f:"<<f<<" commit:"<<commit_id;
    auto it = child_[f].find(commit_id);
    if (it != child_[f].end()) {
      child_[f].erase(it);
      // LOG(ERROR)<<"remove child commit id:"<<commit_id<<" from:"<<f;
    }
  }

  for (int c : child_[commit_id]) {
    auto it = pre_[c].find(commit_id);
    if (it != pre_[c].end()) {
      pre_[c].erase(it);
      // LOG(ERROR)<<"remove pre commit id:"<<commit_id<<" from:"<<c<<" current
      // size:"<<pre_[c].size()<<" wait:"<<wait_[c];
      if (pre_[c].empty() && wait_[c]) {
        if (finish_[c] && wait_[c]) {
          pending_check_.push_back(c);
        }
      }
    }
  }
  pre_[commit_id].clear();
  child_[commit_id].clear();
  for (auto& it : changes_list_[commit_id]) {
    const auto& address = it.first;
    int addr_idx = AddressToId(address);
    int addr_idx_e = addr_idx & window_size_;
    bool is_lastr = false, is_lastw = false, is_last = false;
    auto& lrp = lastr_[addr_idx];
    auto& lwp = lastw_[addr_idx];
    if (lrp.find(commit_id) != lrp.end()) {
      is_lastr = true;
      is_last = true;
    }
    if (lwp.find(commit_id) != lwp.end()) {
      is_lastw = true;
      is_last = true;
    }
    // LOG(ERROR)<<"commit id:"<<commit_id<<" address:"<<address<<" addr pre
    // size:"<<addr_pre_[commit_id][address].size()<<" is last:"<<is_last;
    for (int f : addr_pre_[commit_id][address]) {
      //  LOG(ERROR)<<"commit id:"<<commit_id<<" address:"<<address<<" f:"<<f;
      for (int c : addr_child_[commit_id][address]) {
        if (f == 0) {
          root_addr_[addr_idx_e][address].insert(c);
        } else {
          addr_child_[f][address].insert(c);
        }
        addr_pre_[c][address].insert(f);
        // LOG(ERROR)<<"add adr:"<<f<<" "<<c;
      }
      if (f == 0) {
        auto it = root_addr_[addr_idx_e][address].find(commit_id);
        if (it != root_addr_[addr_idx_e][address].end()) {
          root_addr_[addr_idx_e][address].erase(it);
          //   LOG(ERROR)<<"addr :"<<f<<" remove child commit:"<<commit_id<<"
          //   address:"<<address;
        }
      } else {
        auto it = addr_child_[f][address].find(commit_id);
        if (it != addr_child_[f][address].end()) {
          addr_child_[f][address].erase(it);
          //   LOG(ERROR)<<"addr :"<<f<<" remove child commit:"<<commit_id<<"
          //   address:"<<address;
        }
        if (is_last && addr_child_[f][address].empty() &&
            !changes_list_[f][address].empty()) {
          if (changes_list_[f][address].front().state == LOAD) {
            // LOG(ERROR)<<"add new last:"<<f<<" address:"<<address<<"
            // from:"<<commit_id;
            lrp.insert(f);
          }
          if (changes_list_[f][address].back().state == STORE) {
            // LOG(ERROR)<<"add new last:"<<f<<" address:"<<address<<"
            // from:"<<commit_id;
            lwp.insert(f);
          }
          // assert(!changes_list_[f][address].empty());
        }
      }
    }
    for (int c : addr_child_[commit_id][address]) {
      auto it = addr_pre_[c][address].find(commit_id);
      if (it != addr_pre_[c][address].end()) {
        addr_pre_[c][address].erase(it);
        //  LOG(ERROR)<<"addr :"<<c<<" remove pre commit:"<<commit_id<<"
        //  address:"<<address<<" finish?:"<<finish_[c]<<" wait:"<<wait_[c];
        if (finish_[c] && wait_[c]) {
          pending_check_.push_back(c);
        }
      }
    }
    if (is_lastw) {
      lwp.erase(lwp.find(commit_id));
    }
    if (is_lastr) {
      lrp.erase(lrp.find(commit_id));
    }
    if (root_addr_[addr_idx_e].find(address) != root_addr_[addr_idx_e].end()) {
      auto& mp = root_addr_[addr_idx_e][address];
      if (mp.find(commit_id) != mp.end()) {
        mp.erase(mp.find(commit_id));
      }
    }
  }
  addr_pre_[commit_id].clear();
  addr_child_[commit_id].clear();
  changes_list_[commit_id].clear();
  addr_changes_list_[commit_id].clear();
  has_write_[commit_id] = false;

  // LOG(ERROR)<<"remove node done:"<<commit_id;
}

bool DXController::Reach(int from, int to) {
  // return reach_[to][from];
  std::queue<int> q;
  q.push(from);

  std::set<int> v;
  v.insert(from);

  while (!q.empty()) {
    int x = q.front();
    q.pop();
    if (x == to) {
      // LOG(ERROR)<<"access from:"<<from<<" to:"<<to<<" reach
      // size:"<<reach_[from].size();
      // assert(reach_[to][from]==1);
      return true;
    }

    for (int ch : child_[x]) {
      if (v.find(ch) == v.end()) {
        v.insert(ch);
        q.push(ch);
      }
    }
  }
  // assert(reach_[to][from]==false);
  return false;
}

std::set<int> DXController::GetReach(int from) {
  std::queue<int> q;
  q.push(from);

  std::set<int> v;
  v.insert(from);

  while (!q.empty()) {
    int x = q.front();
    q.pop();
    for (int ch : child_[x]) {
      if (v.find(ch) == v.end()) {
        v.insert(ch);
        q.push(ch);
      }
    }
  }
  // assert(reach_[to][from]==false);
  return v;
}

std::set<int> DXController::GetReach(int from, int to) {
  std::queue<int> q;
  q.push(from);

  std::set<int> v;
  v.insert(from);

  while (!q.empty()) {
    int x = q.front();
    q.pop();
    if (x == to) {
      // LOG(ERROR)<<"access from:"<<from<<" to:"<<to<<" reach
      // size:"<<reach_[from].size();
      // assert(reach_[to][from]==1);
      return v;
    }

    for (int ch : child_[x]) {
      if (v.find(ch) == v.end()) {
        v.insert(ch);
        q.push(ch);
      }
    }
  }
  // assert(reach_[to][from]==false);
  return v;
}

bool DXController::PrintReach(int from, int to) {
  // return reach_[to][from];
  std::queue<int> q;
  q.push(from);

  std::set<int> v;
  v.insert(from);

  while (!q.empty()) {
    int x = q.front();
    q.pop();
    LOG(ERROR) << "reach from:" << from << " to:" << x;
    if (x == to) {
      return true;
    }

    for (int ch : child_[x]) {
      if (v.find(ch) == v.end()) {
        v.insert(ch);
        q.push(ch);
      }
    }
  }
  // assert(reach_[to][from]==false);
  return false;
}

bool DXController::ContainRead(int commit_id, const uint256_t& address) {
#ifdef CDebug
// LOG(ERROR)<<"check read commit id:"<<commit_id<<" address:"<<address;
#endif
  if (addr_child_[commit_id].find(address) == addr_child_[commit_id].end()) {
    return false;
  }

  for (int child : addr_child_[commit_id][address]) {
#ifdef CDebug
    //   LOG(ERROR)<<"check child from:"<<commit_id<<" child:"<<child<<"
    //   address:"<<address;
#endif
    auto& change_set = changes_list_[child][address];
    if (change_set.empty()) {
      continue;
    }
    if (change_set.front().state == LOAD) {
#ifdef CDebug
      //    LOG(ERROR)<<" child:"<<child<<" read address:"<<address<<"
      //    from:"<<commit_id;
#endif
      return true;
    }
  }
  return false;
}

void DXController::ConnectSelf(int idx, int last_idx, const uint256_t& address,
                               int addr_idx) {
  std::vector<int> new_f;
  std::vector<int> new_cf;
  auto& lrp = lastr_[addr_idx];
  auto& lwp = lastw_[addr_idx];
#ifdef CDebug
  LOG(ERROR) << "connect idx:" << idx << " last idx:" << last_idx
             << " address:" << address
             << " size:" << addr_pre_[last_idx].size();
#endif
  // LOG(ERROR)<<"connect idx:"<<idx<<" last idx:"<<last_idx<<"
  // address:"<<address<<" size:"<<addr_pre_[last_idx].size();
  for (int f : addr_pre_[last_idx][address]) {
    if (f == 0) {
      int addr_idx_e = addr_idx & window_size_;
      auto& mp = root_addr_[addr_idx_e][address];
      int sz = mp.size();

      // LOG(ERROR)<<"address:"<<address<<" get f:"<<f<<"
      // committed:"<<committed_[f]<<" idxx:"<<idx<<" last idx:"<<last_idx;
      if (sz > 1) {
        for (int cf : mp) {
          if (cf == last_idx) {
            continue;
          }
          if (committed_[cf]) {
            continue;
          }
          new_cf.push_back(cf);
          //   LOG(ERROR)<<"address:"<<address<<" get cf:"<<cf<<" from f:"<<f<<"
          //   idx:"<<idx;
          // assert(cf != f);
        }
        new_f.push_back(f);
        assert(root_addr_[addr_idx_e][address].find(last_idx) !=
               root_addr_[addr_idx_e][address].end());
        root_addr_[addr_idx_e][address].erase(
            root_addr_[addr_idx_e][address].find(last_idx));
      }

    } else {
      auto& mp = addr_child_[f][address];
      int sz = mp.size();

      // LOG(ERROR)<<"address:"<<address<<" get f:"<<f<<"
      // committed:"<<committed_[f]<<" idxx:"<<idx<<" last idx:"<<last_idx;
      if (sz > 1) {
        for (int cf : mp) {
          if (cf == last_idx) {
            continue;
          }
          if (committed_[cf]) {
            continue;
          }
          new_cf.push_back(cf);
          //   LOG(ERROR)<<"address:"<<address<<" get cf:"<<cf<<" from f:"<<f<<"
          //   idx:"<<idx;
          assert(cf != f);
        }
        new_f.push_back(f);
        assert(addr_child_[f][address].find(last_idx) !=
               addr_child_[f][address].end());
        addr_child_[f][address].erase(addr_child_[f][address].find(last_idx));
      }
    }
  }
  if (!new_f.empty()) {
    for (int f : new_f) {
      // if(addr_pre_[f][address].find(last_idx) ==
      // addr_pre_[f][address].end()){
      // LOG(ERROR)<<"last no addr pre:"<<last_idx<<" pre:"<<f<<"
      // address:"<<address;
      //}
      assert(addr_pre_[last_idx][address].find(f) !=
             addr_pre_[last_idx][address].end());
      addr_pre_[last_idx][address].erase(addr_pre_[last_idx][address].find(f));
    }
  }
  if (!new_cf.empty()) {
    std::set<int> r = GetReach(last_idx);
    for (int cf : new_cf) {
      if (r.find(cf) != r.end()) {
// cycle
#ifdef CDebug
        LOG(ERROR) << "have cycle after change:" << last_idx
                   << " address:" << address;
#endif
        Abort(last_idx);
        return;
      }
    }

    for (int cf : new_cf) {
      if (cf == last_idx) {
        continue;
      }
      if (committed_[cf]) {
        continue;
      }
      if (lrp.find(cf) != lrp.end()) {
        lrp.erase(lrp.find(cf));
        // LOG(ERROR)<<" remove last idx:"<<cf<<" address:"<<address;
      }
      if (lwp.find(cf) != lwp.end()) {
        lwp.erase(lwp.find(cf));
        // LOG(ERROR)<<" remove last idx:"<<cf<<" address:"<<address;
      }
#ifdef CDebug
      LOG(ERROR) << "add new pre:" << address << " last id:" << last_idx
                 << " cf:" << cf << " commited:" << committed_[cf];
      assert(cf != last_idx);
#endif
      addr_child_[cf][address].insert(last_idx);
      addr_pre_[last_idx][address].insert(cf);
      child_[cf].insert(last_idx);
      pre_[last_idx].insert(cf);
    }
  }
#ifdef CDebug
// LOG(ERROR)<<"connect :"<<idx<<" last id:"<<last_idx<<" address:"<<address;
#endif
}

bool DXController::Connect(int idx, int last_idx, const uint256_t& address,
                           int addr_idx, bool is_read) {
  int addr_idx_e = addr_idx & window_size_;
  if (last_idx == 0) {
    if (!is_read) {
      ds_ = true;
      lastw_[addr_idx].insert(idx);
    } else {
      lastr_[addr_idx].insert(idx);
    }
    addr_pre_[idx][address].insert(last_idx);
    root_addr_[addr_idx_e][address].insert(idx);
#ifdef CDebug
    LOG(ERROR) << "connect add last:" << idx << " address:" << address
               << " idx:" << addr_idx;
#endif
    // addr_child_[last_idx][addr_idx].insert(idx);
    // assert(addr_child_[last_idx][addr_idx].size()==1);
    return true;
  }

  if (committed_[last_idx]) {
    // LOG(ERROR)<<"connect new node:"<<idx<<" address:"<<address<<" last node
    // committed:"<<last_idx;
    auto& lrp = lastr_[addr_idx];
    auto& lwp = lastw_[addr_idx];
    {
      auto it = lrp.find(last_idx);
      if (it != lrp.end()) {
        lrp.erase(it);
      }
    }
    {
      auto it = lwp.find(last_idx);
      if (it != lwp.end()) {
        lwp.erase(it);
      }
    }
    if (is_read) {
      lrp.insert(idx);
    } else {
      ds_ = true;
      lwp.insert(idx);
    }
    addr_pre_[idx][address].insert(last_idx);
    addr_child_[last_idx][address].insert(idx);
    return true;
  }

  ds_ = true;
  std::vector<int> new_f;
  std::vector<int> new_cf;
  auto& lrp = lastr_[addr_idx];
  auto& lwp = lastw_[addr_idx];
#ifdef CDebug
  LOG(ERROR) << "connect idx:" << idx << " last idx:" << last_idx
             << " address:" << address
             << " size:" << addr_pre_[last_idx].size();
#endif
  // LOG(ERROR)<<"connect idx:"<<idx<<" last idx:"<<last_idx<<"
  // address:"<<address<<" size:"<<addr_pre_[last_idx].size(); LOG(ERROR)<<"
  // size:"<<addr_pre_[last_idx][address].size();
  // assert(addr_pre_[last_idx][address].empty());
  auto ait = addr_pre_[last_idx].find(address);
  if (ait != addr_pre_[last_idx].end()) {
    for (int f : ait->second) {
      if (f == 0) {
        int addr_idx_e = addr_idx & window_size_;
        auto& mp = root_addr_[addr_idx_e][address];
        int sz = mp.size();
        // LOG(ERROR)<<"address:"<<address<<" f:"<<f<<" commit id:"<<idx<<" f
        // size:"<<sz; LOG(ERROR)<<"address:"<<address<<" get f:"<<f<<"
        // committed:"<<committed_[f]<<" idxx:"<<idx<<" last idx:"<<last_idx;
        if (sz > 1) {
          for (int cf : mp) {
            if (cf == last_idx) {
              continue;
            }
            if (committed_[cf]) {
              continue;
            }
            new_cf.push_back(cf);
#ifdef CDebug
            LOG(ERROR) << "address:" << address << " get cf:" << cf
                       << " from f:" << f << " idx:" << idx;
            assert(cf != f);
#endif
          }
          new_f.push_back(f);
          if (mp.find(last_idx) != mp.end()) {
            mp.erase(mp.find(last_idx));
          }
        }

      } else {
        auto& mp = addr_child_[f][address];
        int sz = mp.size();
        // LOG(ERROR)<<"address:"<<address<<" f:"<<f<<" commit id:"<<idx<<" f
        // size:"<<sz; assert(sz==1);
        // LOG(ERROR)<<"address:"<<address<<" get f:"<<f;

        if (sz > 1) {
          for (int cf : mp) {
            if (committed_[cf]) {
              continue;
            }
            if (cf == last_idx) {
              continue;
            }

            // assert(cf != last_idx);
            new_cf.push_back(cf);
            //    LOG(ERROR)<<"address:"<<address<<" get cf:"<<cf;
          }
          new_f.push_back(f);
          if (mp.find(last_idx) != mp.end()) {
            mp.erase(mp.find(last_idx));
          }
        }
      }
    }
  }
  // LOG(ERROR)<<" newf size:"<<new_f.size()<<" new cf size:"<<new_cf.size();
  for (int f : new_f) {
    ait->second.erase(ait->second.find(f));
  }
  pre_[idx].insert(last_idx);
  child_[last_idx].insert(idx);
  if (!new_cf.empty()) {
    std::set<int> r = GetReach(last_idx);
    for (int cf : new_cf) {
      if (r.find(cf) != r.end()) {
        // cycle
#ifdef CDebug
        LOG(ERROR) << "have cycle after change:" << last_idx
                   << " address:" << address;
#endif
        AbortNode(last_idx);
        return false;
      }
    }
    for (int cf : new_cf) {
      if (committed_[cf]) {
        continue;
      }
      if (lrp.find(cf) != lrp.end()) {
        lrp.erase(lrp.find(cf));
      }
      if (lwp.find(cf) != lwp.end()) {
        lwp.erase(lwp.find(cf));
      }
#ifdef CDebug
      //  LOG(ERROR)<<"add new pre:"<<address<<" last id:"<<last_idx<<"
      //  cf:"<<cf;
      assert(last_idx != cf);
#endif
      ait->second.insert(cf);
      addr_child_[cf][address].insert(last_idx);
      addr_pre_[last_idx][address].insert(cf);
      child_[cf].insert(last_idx);
      pre_[last_idx].insert(cf);
    }
  }

  pre_[idx].insert(last_idx);
  child_[last_idx].insert(idx);
#ifdef CDebug
  LOG(ERROR) << "connect :" << idx << " last id:" << last_idx
             << " address:" << address;
#endif

  addr_pre_[idx][address].insert(last_idx);
  addr_child_[last_idx][address].insert(idx);

  if (lrp.find(last_idx) != lrp.end()) {
    lrp.erase(lrp.find(last_idx));
  }
  if (lwp.find(last_idx) != lwp.end()) {
    lwp.erase(lwp.find(last_idx));
  }
  if (is_read) {
    lrp.insert(idx);
  } else {
    lwp.insert(idx);
  }
  return true;
}

bool DXController::AddNewNode(const uint256_t& address, int addr_id,
                              int64_t commit_id, Data& data) {
#ifdef CDebug
  //  LOG(ERROR)<<"add new node:"<<commit_id;
#endif
  if (data.state == STORE) {
    int last_r = GetLastR(address, addr_id);
    //     LOG(ERROR)<<"get last r:"<<last_r<<" commit :"<<commit_id<<"
    //     address:"<<address;
    if (last_r == -1) {
      // new father
      /*
      auto ret = storage_->Load(address, false);
      data.version = ret.second+1;
      */
      data.version = 0;
      Connect(commit_id, 0, address, addr_id, data.state != STORE);
      // assert(ret.second<=1);
    } else {
      data.version = addr_changes_list_[last_r][addr_id].data.version + 1;
      bool ret = Connect(commit_id, last_r, address, addr_id, false);
      if (!ret) {
        LOG(ERROR) << "connect commit:" << commit_id << " to :" << last_r
                   << " fail";
        return ret;
      }
    }
  } else {
    int last_w = GetLastW(address, addr_id);
#ifdef CDebug
    LOG(ERROR) << "get last w:" << last_w << " commit :" << commit_id
               << " address:" << address;
#endif
    if (last_w == -1) {
      bool found = false;
      int addr_idx_e = addr_id & window_size_;
      for (int ch : root_addr_[addr_idx_e][address]) {
        auto& cs = changes_list_[ch][address];
        if (!cs.empty() && cs.front().state == LOAD) {
          data.data = cs.front().data;
          data.version = cs.front().version;
          found = true;
          break;
        }
      }
      if (!found) {
        auto ret = storage_->Load(address, false);
        // LOG(ERROR)<<"load storage:"<<address;
        data.data = ret.first;
        data.version = ret.second;
      }
      // new father
      //   auto ret = storage_->Load(address, false);
      //  data.data = ret.first;
      // data.version = ret.second;
      bool ret = Connect(commit_id, 0, address, addr_id, data.state != STORE);
      assert(ret);
      //  assert(ret.second<=1);
    } else {
      {
        if (Reach(commit_id, last_w)) {
// assert(1==0);
// std::set<int> rs = GetReach(commit_id, last_w);
// cycle?
#ifdef CDebug
          //   LOG(ERROR)<<"commit id:"<<commit_id<<" last w:"<<last_w<<" is in
          //   cycle";
#endif
          // LOG(ERROR)<<"commit id:"<<commit_id<<" last w:"<<last_w<<" is in
          // cycle"<<" address:"<<address;
          AbortNode(commit_id);
          return false;
        } else {
          bool ret;
          if (data.state == STORE) {
            ret = Connect(commit_id, last_w, address, addr_id, false);
          } else {
            ret = Connect(commit_id, last_w, address, addr_id, true);
          }
          if (!ret) {
            // LOG(ERROR)<<"connect commit:"<<commit_id<<" to :"<<last_w<<"
            // fail";
            return ret;
          }
          // cycle?
          // LOG(ERROR)<<"commit id:"<<commit_id<<" last w:"<<last_w<<" is not
          // in cycle";
        }
        if (last_w > 0) {
          data.data = addr_changes_list_[last_w][addr_id].data.data;
          data.version = addr_changes_list_[last_w][addr_id].data.version;
        }
      }
    }
  }

// AddDataToChangeList(changes_list_[commit_id][address], data);
#ifdef CDebug
  // LOG(ERROR)<<"add commit id:"<<commit_id<<"
  // size:"<<changes_list_[commit_id][address].size();
#endif
  return true;
}

int DXController::FindW(int64_t commit_id, const uint256_t& address,
                        int addr_idx) {
  std::queue<int> q;
  q.push(commit_id);

  std::vector<int> v(window_size_ + 1);
  for (int i = 0; i <= window_size_; i++) v[i] = 0;
  v[commit_id] = 1;

  while (!q.empty()) {
    int x = q.front();
    q.pop();
    auto& cs = changes_list_[x][address];
    if (!cs.empty() && x != commit_id) {
      if (cs.back().state == STORE) {
        return x;
      }
    }
    for (int f : addr_pre_[x][address]) {
      if (v[f] == 0) {
        v[f] = 1;
        q.push(f);
      }
    }
  }
  return -1;
}

int DXController::FindR(int64_t commit_id, const uint256_t& address,
                        int addr_idx) {
  std::queue<int> q;
  q.push(commit_id);

  std::vector<int> v(window_size_ + 1);
  for (int i = 0; i <= window_size_; i++) v[i] = 0;
  v[commit_id] = 1;

  while (!q.empty()) {
    int x = q.front();
    q.pop();
    auto& cs = changes_list_[x][address];
    if (!cs.empty()) {
      if (cs.front().state == LOAD) {
        return x;
      }
    }
    for (int f : pre_[x]) {
      if (v[f] == 0) {
        v[f] = 1;
        q.push(f);
      }
    }
  }
  return -1;
}

bool DXController::AddOldNode(const uint256_t& address, int addr_idx,
                              int64_t commit_id, Data& data) {
#ifdef CDebug
// LOG(ERROR)<<"add old node address:"<<address<<" commit id:"<<commit_id;
#endif

  auto& change_set = changes_list_[commit_id][address];
  //#ifdef CDebug
  // LOG(ERROR)<<"change set size:"<<change_set.size()<<" commit
  // id:"<<commit_id<<" address:"<<address; #endif

  bool has_w = false;
  if (change_set.empty()) {
#ifdef CDebug
    //    LOG(ERROR)<<"new address :"<<address<<" commit :"<<commit_id;
#endif
    if (data.state == STORE) {
      int last_r = GetLastR(address, addr_idx);
#ifdef CDebug
      LOG(ERROR) << "get last r:" << last_r << " commit :" << commit_id
                 << " address:" << address;
#endif
      if (last_r == -1) {
        // new father
        /*
        auto ret = storage_->Load(address, false);
        data.version = ret.second+1;
        */
        data.version = 0;
        Connect(commit_id, 0, address, addr_idx, data.state != STORE);
        // assert(ret.second==0);
      } else {
        if (Reach(commit_id, last_r)) {
          // LOG(ERROR)<<"commit id:"<<commit_id<<" last r:"<<last_r<<" is in
          // cycle"; AbortNode(commit_id);
          AbortNode(commit_id);
          /*
          last_r = GetLastR(address, addr_idx);
   //       LOG(ERROR)<<"commit id:"<<commit_id<<" re-get last r:"<<last_r;
          bool ret = Connect(commit_id, last_r, address, addr_idx,
   data.state!=STORE); if(!ret){ LOG(ERROR)<<"connect commit:"<<commit_id<<" to
   :"<<last_r<<" fail"; return ret;
          }
          */
          return false;
        } else {
          // cycle?
          // LOG(ERROR)<<"commit id:"<<commit_id<<" last r:"<<last_r<<" is not
          // in cycle";
          bool ret = Connect(commit_id, last_r, address, addr_idx,
                             data.state != STORE);
          if (!ret) {
            LOG(ERROR) << "connect commit:" << commit_id << " to :" << last_r
                       << " fail";
            return ret;
          }
        }
        data.version = addr_changes_list_[last_r][addr_idx].data.version + 1;
      }
    } else {
      int last_w = GetLastW(address, addr_idx);
#ifdef CDebug
      LOG(ERROR) << "get last w:" << last_w << " commit :" << commit_id
                 << " address:" << address;
#endif
      // LOG(ERROR)<<"get last w:"<<last_w<<" commit :"<<commit_id<<"
      // address:"<<address;
      if (last_w == -1) {
        // new father
        bool found = false;
        int addr_idx_e = addr_idx & window_size_;
        for (int ch : root_addr_[addr_idx_e][address]) {
          auto& cs = changes_list_[ch][address];
          if (!cs.empty() && cs.front().state == LOAD) {
            data.data = cs.front().data;
            data.version = cs.front().version;
            found = true;
            break;
          }
        }
        if (!found) {
          // LOG(ERROR)<<"load storage:"<<address;
          auto ret = storage_->Load(address, false);
          data.data = ret.first;
          data.version = ret.second;
        }
        bool ret =
            Connect(commit_id, 0, address, addr_idx, data.state != STORE);
        assert(ret);
        // assert(ret.second==0);
      } else {
        if (Reach(commit_id, last_w)) {
// assert(1==0);
// std::set<int> rs = GetReach(commit_id, last_w);
// cycle?
#ifdef CDebug
          //   LOG(ERROR)<<"commit id:"<<commit_id<<" last w:"<<last_w<<" is in
          //   cycle";
#endif
          // LOG(ERROR)<<"commit id:"<<commit_id<<" last w:"<<last_w<<" is in
          // cycle"<<" address:"<<address;
          AbortNode(commit_id);
          return false;
        } else {
          bool ret = true;
          if (data.state == STORE) {
            ret = Connect(commit_id, last_w, address, addr_idx, false);
          } else {
            ret = Connect(commit_id, last_w, address, addr_idx, true);
          }
          if (!ret) {
            // LOG(ERROR)<<"connect commit:"<<commit_id<<" to :"<<last_w<<"
            // fail";
            return ret;
          }
// cycle?
#ifdef CDebug
          LOG(ERROR) << "commit id:" << commit_id << " last w:" << last_w
                     << " is not in cycle";
#endif
        }
        if (last_w > 0) {
          data.data = addr_changes_list_[last_w][addr_idx].data.data;
          data.version = addr_changes_list_[last_w][addr_idx].data.version;
        }
      }
    }
  } else {
    if (data.state == STORE) {
      if (change_set.back().state == LOAD) {
        data.version = change_set.back().version + 1;
      } else {
        data.version = change_set.back().version;
      }
    } else {
      data.data = change_set.back().data;
      data.version = change_set.back().version;
    }
    if (data.state == STORE) {
      ConnectSelf(commit_id, commit_id, address, addr_idx);
      if (aborted_[commit_id]) {
        AbortNode(commit_id);
        return false;
      }
      has_w = has_write_[commit_id];
    }
    auto& lrp = lastr_[addr_idx];
    auto& lwp = lastw_[addr_idx];

    if (lrp.find(commit_id) != lrp.end()) {
      if (data.state == STORE) {
        lwp.insert(commit_id);
      }
    } else if (lwp.find(commit_id) != lwp.end()) {
      if (data.state == LOAD) {
        lrp.insert(commit_id);
      }
    }
  }
#ifdef CDebug
  LOG(ERROR) << "has w:" << has_w << " address:" << address
             << " commit::" << commit_id;
#endif
  if (has_w) {
    // LOG(ERROR)<<"try abort idx:"<<commit_id<<" address:"<<address;
    if (data.state == STORE) {
      AbortNodeFrom(commit_id, address);
    }
  } else {
    if (change_set.size() > 0 && data.state == STORE) {
      AbortNodeFrom(commit_id, address);
    }
  }

  //    AddDataToChangeList(change_set, data);
#ifdef CDebug
// LOG(ERROR)<<"add commit id:"<<commit_id<<"
// size:"<<changes_list_[commit_id][address].size();
#endif
  return true;
}

void DXController::AppendPreRecord(const uint256_t& address, int64_t commit_id,
                                   Data& data) {
  int address_id = AddressToId(address);
  bool ret = true;
  {
    std::unique_lock lock(g_mutex_);
    // LOG(ERROR)<<" append commit id:"<<commit_id;
#ifdef CDebug
    // LOG(ERROR)<<"append address:"<<address<<"
    // size:"<<changes_list_[commit_id].size()<<" commit id:"<<commit_id<<"
    // type:"<<data.state<<" abort:"<<aborted_[commit_id];
#endif

    if (aborted_[commit_id]) {
      // LOG(ERROR)<<" append commit id done:"<<commit_id;
      return;
    }
    if (changes_list_[commit_id].size() == 0) {
      // new node
      ret = AddNewNode(address, address_id, commit_id, data);
    } else {
      ret = AddOldNode(address, address_id, commit_id, data);
    }

    if (ret) {
      AddDataToChangeList(changes_list_[commit_id][address], data);
      auto& addrset = addr_changes_list_[commit_id][address_id];
      addrset.type |= data.state;
      addrset.data = data;
      if (data.state == STORE) {
        has_write_[commit_id] = true;
        ds_ = true;
      }
    }

#ifdef DDebug
    for (int i = 1; i < 100; ++i) {
      std::string str;
      for (int f : child_[i]) {
        str += " " + std::to_string(f);
      }
      if (str.empty()) continue;
      LOG(ERROR) << " i:" << i << " pre:" << str;
    }

    for (int i = 1; i < 100; ++i) {
      for (int j = 1; j < 100; ++j) {
        if (i == j) continue;
        if (committed_[i] || committed_[j]) continue;
        if (Reach(i, j) && Reach(j, i)) {
          LOG(ERROR) << "find cycle:"
                     << "<" << i << "," << j << ">";
          PrintReach(i, j);
          PrintReach(j, i);
          assert(1 == 0);
        }
      }
    }
#endif
    /*
    for(int i = 1; i < 500; ++i){
      for(auto it : addr_pre_[i]){
        auto address = it.first;
        for(int c : it.second){
          if(c==0)continue;
          if(addr_child_[c][address].find(i) == addr_child_[c][address].end()){
            LOG(ERROR)<<" id:"<<c<<" not find child:"<<i<<" address:"<<address;
          }
          assert(addr_child_[c][address].find(i) !=
    addr_child_[c][address].end());
        }
      }
      for(auto it : addr_child_[i]){
        auto address = it.first;
        for(int c : it.second){
          assert(addr_pre_[c][address].find(i) != addr_pre_[c][address].end());
        }
      }
      for(int f:pre_[i]){
        for(int c : child_[i]){
          if(c == f){
            LOG(ERROR)<<" id: has cycle:"<<i<<" pre:"<<f<<" child:"<<c;
          }
          assert(c != f);
        }
      }
    }
    */
    // LOG(ERROR)<<" append commit id done:"<<commit_id;
  }
}

// ==========================================

bool DXController::CheckCommit(int64_t commit_id) {
  const auto& change_set = changes_list_[commit_id];
  if (change_set.empty()) {
#ifdef CDebug
    LOG(ERROR) << " no commit id record found:" << commit_id;
#endif
    assert(!change_set.empty());
    return false;
  }

  for (const auto& it : change_set) {
    // LOG(ERROR)<<"op size:"<<it.second.size()<<" commit id:"<<commit_id;
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

bool DXController::CommitUpdates(int64_t commit_id) {
#ifdef CDebug
  LOG(ERROR) << "check commit:" << commit_id << " wait:" << wait_[commit_id]
             << " abort:" << aborted_[commit_id];
#endif
  wait_[commit_id] = false;

  if (aborted_[commit_id]) {
    return false;
  }

  if (committed_[commit_id]) {
    // LOG(ERROR)<<"commit id:"<<commit_id<<" has done";
    assert(1 == 0);
    return true;
  }

  /*
    const auto& change_set = changes_list_[commit_id];
    if(change_set.empty()){
  #ifdef CDebug
      LOG(ERROR)<<" no commit id record found:"<<commit_id;
  #endif
      assert(!change_set.empty());
      return false;
    }
    */

  {
    std::unique_lock lock(g_mutex_);
    // LOG(ERROR)<<" commit id :"<<commit_id;

    if (aborted_[commit_id]) {
      return false;
    }

    const auto& change_set = changes_list_[commit_id];
    if (change_set.empty()) {
#ifdef CDebug
      LOG(ERROR) << " no commit id record found:" << commit_id;
#endif
      assert(!change_set.empty());
      return false;
    }

    for (int x : post_abort_) {
      AddRedo(x);
    }
    post_abort_.clear();

    if (pending_check_.size()) {
      // LOG(ERROR)<<"pending check size:"<<pending_check_.size();
    }
    for (int x : pending_check_) {
      pd_.insert(x);
    }
    pending_check_.clear();

    if (pre_[commit_id].size() > 0) {
      if (pre_[commit_id].size() == 1 && *pre_[commit_id].begin() == 0) {
      } else {
#ifdef CDebug
        for (int f : pre_[commit_id]) {
          LOG(ERROR) << " wait for pre:" << f << " commit id:" << commit_id;
        }
#endif
        wait_[commit_id] = true;
        //   LOG(ERROR)<<" commit id done:"<<commit_id;
        return true;
      }
    }
    // LOG(ERROR)<<" commit id :"<<commit_id;

    for (int child : child_[commit_id]) {
      if (pre_[child].find(commit_id) == pre_[child].end()) {
        LOG(ERROR) << "find child:" << child << " from:" << commit_id
                   << " fail";
      }

      assert(pre_[child].find(commit_id) != pre_[child].end());
      pre_[child].erase(pre_[child].find(commit_id));
#ifdef CDebug
      LOG(ERROR) << "find child:" << child << " from:" << commit_id
                 << " wait:" << wait_[child]
                 << " pre size:" << pre_[child].size();
#endif
      if (pre_[child].empty() && wait_[child]) {
        // LOG(ERROR)<<"get pending commit child:"<<child;
        pd_.insert(child);
      }
    }
    committed_[commit_id] = true;

    /*
          for(const auto& ait: addr_changes_list_[commit_id]){
            int addr_idx = ait.first;
    #ifdef CDebug
              LOG(ERROR)<<"check last:"<<commit_id<<" addr idx:"<<addr_idx;
    #endif
            auto it = last_[addr_idx].find(commit_id);
            if(it != last_[addr_idx].end()){
    #ifdef CDebug
              LOG(ERROR)<<"remove last:"<<commit_id<<" addr idx:"<<addr_idx;
    #endif
              last_[addr_idx].erase(it);
              //LOG(ERROR)<<" commit id pre:"<<commit_id<<" last
    size:"<<last_[addr_idx].size();
            }
          }
          */

    for (auto it : addr_pre_[commit_id]) {
      const auto& address = it.first;
      for (int f : it.second) {
        if (f == 0) {
          int addr_idx = AddressToId(address);
          int addr_idx_e = addr_idx & window_size_;
          if (root_addr_[addr_idx_e].find(address) !=
              root_addr_[addr_idx_e].end()) {
            auto& mp = root_addr_[addr_idx_e][address];
            if (mp.find(commit_id) != mp.end()) {
              mp.erase(mp.find(commit_id));
            }
          }
        }
      }
    }

    //   LOG(ERROR)<<" commit id done:"<<commit_id;
    /*
if(!CheckCommit(commit_id)){
 LOG(ERROR)<<"check commit fail:"<<commit_id;
 assert(1==0);
 return false;
}
*/
  }

  {
    const auto& change_set = changes_list_[commit_id];
    for (const auto& it : change_set) {
      // LOG(ERROR)<<"commit addr:"<<it.first<<" size:"<<it.second.size();
      bool done = false;
      for (int i = it.second.size() - 1; i >= 0 && !done; --i) {
        const auto& op = it.second[i];
        switch (op.state) {
          case LOAD:
            // LOG(ERROR)<<"load";
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

    // LOG(ERROR)<<" commit id done:"<<commit_id;
  }

  done_.push_back(commit_id);
#ifdef CDebug
  LOG(ERROR) << "commit done:" << commit_id;
#endif
  return true;
}

bool DXController::Commit(int64_t commit_id) {
#ifdef CDebug
  LOG(ERROR) << "commit id:" << commit_id;
  assert(!committed_[commit_id]);
#endif

  redo_.clear();
  done_.clear();
  finish_[commit_id] = true;
  is_redo_[commit_id] = false;
  committed_[commit_id] = false;
  commit_time_[commit_id] = GetCurrentTime();
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

  // LOG(ERROR)<<"commit time:"<<GetCurrentTime-time;
  // changes_log_[commit_id]=changes_list_[commit_id];
  // changes_list_[commit_id].clear();
  return true;
}

const std::vector<int64_t>& DXController::GetRedo() { return redo_; }
const std::vector<int64_t>& DXController::GetDone() { return done_; }

uint64_t DXController::GetDelay(int64_t commit_id) {
  return commit_delay_[commit_id];
}

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb

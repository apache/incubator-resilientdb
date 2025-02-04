#pragma once

#include <bitset>
#include <deque>
#include <set>
#include <shared_mutex>
#include <thread>
#include <unordered_set>

#include "platform/common/queue/lock_free_queue.h"
#include "service/contract/executor/x_manager/concurrency_controller.h"
#include "service/contract/executor/x_manager/data_storage.h"

namespace resdb {
namespace contract {
namespace x_manager {

class DXController : public ConcurrencyController {
 public:
  DXController(DataStorage* storage, int window_size);
  virtual ~DXController();

  // ==============================
  virtual void Store(const int64_t commit_id, const uint256_t& key,
                     const uint256_t& value, int version);
  virtual uint256_t Load(const int64_t commit_id, const uint256_t& key,
                         int version);
  bool Remove(const int64_t commit_id, const uint256_t& key, int version);

  // ==============================
  typedef std::map<uint256_t, std::deque<int64_t>> CommitList;

  const std::vector<int64_t>& GetDone();
  const std::vector<int64_t>& GetRedo();

  void PushCommit(int64_t commit_id, const ModifyMap& local_changes_) {}
  bool Commit(int64_t commit_id);

  void StoreInternal(const int64_t commit_id, const uint256_t& key,
                     const uint256_t& value, int version);
  uint256_t LoadInternal(const int64_t commit_id, const uint256_t& key,
                         int version);

  std::unique_ptr<ConcurrencyController::ModifyMap> GetChangeList(
      int64_t commit_id);

  void Clear();
  void Clear(int64_t commit_id);
  uint64_t GetDelay(int64_t commit_id);

 private:
  bool CommitUpdates(int64_t commit_id);

  bool CheckCommit(int64_t commit_id);

  void AppendPreRecord(const uint256_t& address, int64_t commit_id, Data& data);

  int GetLastR(const uint256_t& address, int addr_id);
  int GetLastW(const uint256_t& address, int addr_id);
  int GetLastWOnly(const uint256_t& address, int addr_id);
  int FindW(int64_t commit_id, const uint256_t& address, int addr_id);
  int FindR(int64_t commit_id, const uint256_t& address, int addr_id);

  bool Connect(int idx, int last_idx, const uint256_t& address, int addr_idx,
               bool is_read);
  void ConnectSelf(int idx, int last_idx, const uint256_t& address,
                   int addr_idx);
  bool AddNewNode(const uint256_t& address, int addr_idx, int64_t commit_id,
                  Data& data);
  bool AddOldNode(const uint256_t& address, int addr_idx, int64_t commit_id,
                  Data& data);
  void AddDataToChangeList(std::vector<Data>& change_set, const Data& data);

  void RecursiveAbort(int64_t idx, const uint256_t& address);
  void AbortNodeFrom(int64_t idx, const uint256_t& address);
  void AbortNode(int64_t idx);
  bool ContainRead(int commit_id, const uint256_t& address);

  void RemoveNode(int64_t commit_id);

  void Abort(const int64_t commit_id);
  bool Reach(int from, int to);
  bool PrintReach(int from, int to);
  std::set<int> GetReach(int from, int to);
  std::set<int> GetReach(int from);

  int GetDep(int from, int to);
  void AddRedo(int64_t commit_id);

  int AddressToId(const uint256_t& key);
  uint256_t& GetAddress(int key);

  void ResetReach(int64_t commit_id);
  void Update(const std::vector<int>& commit_id);

 private:
  int window_size_ = 1000;

  struct DataInfo {
    int64_t commit_id;
    int type;
    Data data;
    int version;
    DataInfo() : type(0) {}
    DataInfo(int64_t commit_id, int type, Data& data)
        : commit_id(commit_id), type(type), data(data) {}
  };

  // typedef std::map<int, std::pair<uint256_t, std::vector<Data>>> KeyMap;
  std::vector<ModifyMap> changes_list_;
  std::vector<std::map<int, DataInfo>> addr_changes_list_;
  // std::vector<std::unique_ptr<ModifyMap>> changes_list_;

  std::set<int64_t> pd_;
  std::vector<int64_t> redo_;
  std::vector<int64_t> done_;
  DataStorage* storage_;
  std::vector<bool> wait_;
  std::mutex mutex_[2048], abort_mutex_;
  std::mutex g_mutex_, k_mutex_[2048];
  std::map<uint256_t, std::pair<int, int64_t>> lock_[2048];
  bool aborted_[2048];
  bool is_redo_[2048];
  bool finish_[2048];
  bool committed_[2048];
  bool has_write_[2048];
  std::vector<int64_t> abort_list_;
  std::set<int> pre_[2048];
  std::set<int> child_[2048];
  std::bitset<2048> reach_[2048];

  std::map<uint256_t, std::set<int>> addr_pre_[2048];
  std::map<uint256_t, std::set<int>> addr_child_[2048];
  std::map<uint256_t, std::set<int>> root_addr_[2048];
  std::unordered_map<int, std::set<int>> lastr_, lastw_;

  std::unordered_map<int, std::set<uint256_t>> check_;
  std::set<int> check_abort_;
  std::vector<int> post_abort_, pending_check_;
  std::map<uint256_t, int> key_[2048];
  std::unordered_map<int, uint256_t> akey_;
  std::atomic<int> key_id_;
  bool ds_ = false;
  uint64_t commit_time_[2048], commit_delay_[2048];
};

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb

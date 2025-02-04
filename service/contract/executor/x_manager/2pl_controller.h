#pragma once

#include <deque>
#include <set>
#include <shared_mutex>
#include <thread>

#include "platform/common/queue/lock_free_queue.h"
#include "service/contract/executor/x_manager/concurrency_controller.h"
#include "service/contract/executor/x_manager/data_storage.h"

namespace resdb {
namespace contract {
namespace x_manager {

class TwoPLController : public ConcurrencyController {
 public:
  TwoPLController(DataStorage* storage, int window_size);
  virtual ~TwoPLController();

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

  ModifyMap* GetChangeList(int64_t commit_id);

  void Clear();
  void Clear(int64_t commit_id);

 private:
  // void Clear();

  bool CommitUpdates(int64_t commit_id);

  bool CheckCommit(int64_t commit_id);

  void AppendPreRecord(const uint256_t& address, int64_t commit_id, Data& data);

  void RemovePreRecord(const uint256_t& address, int64_t commit_id);

  enum LockType { READ = 1, WRITE = 2 };

  bool TryLock(const uint256_t& address, int64_t owner, LockType type);
  void ReleaseLock(int64_t commit_id);
  void ReleaseLock(int64_t commit_id, const uint256_t& address);

  void Abort(const int64_t commit_id);

  void AddRedo(int64_t commit_id);
  std::vector<int64_t> FetchAbort();

 private:
  int window_size_ = 1000;

  struct DataInfo {
    int64_t commit_id;
    int type;
    Data data;
    int version;
    DataInfo() {}
    DataInfo(int64_t commit_id, int type, Data& data)
        : commit_id(commit_id), type(type), data(data) {}
  };

  std::vector<ModifyMap> changes_list_;
  // std::vector<std::unique_ptr<ModifyMap>> changes_list_;
  typedef std::map<uint256_t, std::deque<std::unique_ptr<DataInfo>>>
      PreCommitList;
  PreCommitList pre_commit_list_ GUARDED_BY(mutex_);
  std::map<uint256_t, std::unique_ptr<DataInfo>> last_;
  // PreCommitList pre_commit_list_[4096] GUARDED_BY(mutex_);

  std::set<int64_t> pd_;
  std::vector<int64_t> redo_;
  std::vector<int64_t> done_;
  DataStorage* storage_;
  std::vector<bool> wait_;
  std::mutex mutex_[2048], abort_mutex_;
  std::mutex g_mutex_, db_mutex_;
  std::map<uint256_t, std::pair<int, int64_t>> lock_[2048];
  bool aborted_[2048];
  bool is_redo_[2048];
  bool finish_[2048];
  bool committed_[2048];
  std::vector<int64_t> abort_list_;
  std::map<uint256_t, int64_t> lock_table_;
};

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb

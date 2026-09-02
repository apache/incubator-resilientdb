#pragma once

#include <deque>
#include <set>
#include <shared_mutex>
#include <thread>

#include "platform/common/queue/lock_free_queue.h"
#include "service/contract/executor/x_manager/concurrency_controller.h"
#include "service/contract/executor/x_manager/d_storage.h"

namespace resdb {
namespace contract {
namespace x_manager {

class StreamingEController : public ConcurrencyController {
 public:
  StreamingEController(DataStorage* storage, int window_size);
  virtual ~StreamingEController();

  // ==============================
  virtual void Store(const int64_t commit_id, const uint256_t& key,
                     const uint256_t& value, int version);
  virtual uint256_t Load(const int64_t commit_id, const uint256_t& key,
                         int version);
  bool Remove(const int64_t commit_id, const uint256_t& key, int version);

  // ==============================
  typedef std::map<uint256_t, std::deque<int64_t>> CommitList;

  struct DataInfo {
    int64_t commit_id;
    int type;
    Data data;
    int version;
    DataInfo() {}
    DataInfo(int64_t commit_id, int type, Data& data)
        : commit_id(commit_id), type(type), data(data) {}
  };
  typedef std::map<uint256_t, std::deque<std::unique_ptr<DataInfo>>>
      PreCommitList;
  // typedef std::map<uint256_t, std::map<int64_t, std::unique_ptr<DataInfo>> >
  // PreCommitList;

  void GenRedo();
  std::vector<int64_t>& GetRedo();
  std::vector<int64_t>& GetDone();

  void PushCommit(int64_t commit_id, const ModifyMap& local_changes_) {}
  bool Commit(int64_t commit_id);

  void StoreInternal(const int64_t commit_id, const uint256_t& key,
                     const uint256_t& value, int version);
  uint256_t LoadInternal(const int64_t commit_id, const uint256_t& key,
                         int version);
  void SetPrecommitCallback(std::function<void(int64_t)> precommit_callback);

  void Release(int64_t commit_id);
  void Clear(int64_t commit_id);
  bool ExecuteDone(int64_t commit_id);
  bool CheckCommit(int64_t commit_id);

  bool CheckRedo(int64_t commit_id);
  void Clear();

 private:
  void Abort(int64_t commit_id);
  int64_t Release(int64_t commit_id, const uint256_t& address, bool need_abort);

  // void ReleaseCommit(int64_t commit_id);
  void ReleaseCommit(int64_t commit_id, const uint256_t& address);

  bool PreCommit(int64_t commit_id);
  bool PostCommit(int64_t commit_id);

  void RedoConflict(int64_t commit_id);
  bool CheckPreCommit(int64_t commit_id);
  bool CheckFirstFromPreCommit(const uint256_t& address, int64_t commit_id);
  bool CheckFirstFromCommit(const uint256_t& address, int64_t commit_id);

  void Remove(int64_t commit_id);
  void CleanOldData(int64_t commit_id);

  void Rollback(int64_t id, const uint256_t& address);
  void Rollback();

  int64_t RemovePrecommitRecord(const uint256_t& address, int64_t commit_id);
  void RemovePreRecord(const uint256_t& address, int64_t commit_id);

  bool CheckCommit(int64_t commit_id, bool is_pre);
  void CommitDone(int64_t commit_id);
  void RedoCommit(int64_t commit_id, int flag);
  std::set<int64_t> RollCommit(int64_t commit_id);
  std::set<int64_t> AbortCommit(int64_t commit_id, const uint256_t& address);

  void RollBackData(const uint256_t& address, const Data& data);

  void AppendRecord(const uint256_t& address, int64_t commit_id);

  void AppendPreRecord(const uint256_t& address, int64_t commit_id, Data& data,
                       int version);
  void AppendPreRecord(const uint256_t& address, int hash_idx,
                       const std::vector<int64_t>& commit_id);

  void ClearOldData(const int64_t commit_id);
  void AddRedo(int64_t commit_id);
  std::vector<int64_t> FetchAbort();

 private:
  int window_size_ = 1000;

  std::vector<ModifyMap> changes_list_;
  std::vector<std::set<uint256_t>> rechanges_list_;

  CommitList commit_list_[4096];
  int64_t last_commit_id_, current_commit_id_;
  bool commit_[4096];

  PreCommitList pre_commit_list_ GUARDED_BY(mutex_);
  int64_t last_pre_commit_id_;

  std::atomic<int> is_redo_[4096];

  std::vector<int64_t> redo_;

  // std::atomic<bool> is_done_[1024];
  std::vector<int64_t> done_;

  D_Storage* storage_;
  std::mutex mutex_[4096], change_list_mutex_[4096], valid_mutex_[4096],
      rb_mutex_, g_mutex_, abort_mutex_;
  // mutable std::shared_mutex mutex_[4096], change_list_mutex_[4096],
  // valid_mutex_[4096], rb_mutex_;

  bool is_invalid_[4096], is_start_[4096], wait_[4096];
  int version_[4096];
  std::function<void(int64_t)> precommit_callback_;
  std::set<int64_t> forward_[4096], back_[4096];

  typedef std::set<int64_t> RedoList;
  RedoList redo_list_ GUARDED_BY(mutex_);
  void CheckRedo();

  std::vector<int64_t> abort_list_;
  std::set<int64_t> pd_;
  std::set<int64_t> redo_p_;
  std::map<uint256_t, std::set<int64_t>> num_;
  std::map<uint256_t, int> ref_;
};

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb

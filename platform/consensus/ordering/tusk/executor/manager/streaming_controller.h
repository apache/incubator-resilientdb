#pragma once

#include <deque>
#include <set>
#include <shared_mutex>
#include <thread>

#include "platform/common/queue/lock_free_queue.h"
#include "service/contract/executor/manager/concurrency_controller.h"

namespace resdb {
namespace contract {
namespace streaming {

class StreamingController : public ConcurrencyController {
 public:
  StreamingController(DataStorage* storage, int window_size);
  ~StreamingController();

  virtual void PushCommit(int64_t commit_id, const ModifyMap& local_changes_);
  bool Commit(int64_t commit_id);
  std::vector<int64_t>& GetRedo();

  void Clear();

 private:
  bool CommitInternal(int64_t commit_id);
  bool CheckCommit(int64_t commit_id);
  bool CheckFirstCommit(const uint256_t& address, int64_t commit_id);

  const ModifyMap* GetChangeList(int64_t commit_id);
  int64_t Remove(const uint256_t& address, int64_t commit_id, uint64_t v);
  void Remove(int64_t commit_id);

  std::function<void(int64_t)> GetCommitCallBack(int64_t commit_id);
  std::function<void(int64_t, int)> GetRedoCallBack(int64_t commit_id);

  void CommitDone(int64_t commit_id);
  void RedoCommit(int64_t commit_id, int flag);

  int GetHashKey(const uint256_t& address);

  bool IsRead(const uint256_t& address, int64_t commit_id);

 private:
  const int window_size_ = 1000;
  std::atomic<int64_t> last_commit_id_;

  std::vector<ModifyMap> changes_list_;
  std::map<uint256_t, std::deque<int64_t> > commit_list_[1024];
  // std::map<uint256_t, std::set<int64_t> > commit_list_[1024];
  std::map<uint256_t, uint64_t> m_list_;

  std::vector<bool> is_redo_;

  std::vector<int64_t> redo_;
};

}  // namespace streaming
}  // namespace contract
}  // namespace resdb

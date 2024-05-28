#pragma once

#include <set>
#include <deque>
#include <thread>
#include <shared_mutex>

#include "service/contract/executor/paral_sm/concurrency_controller.h"
#include "service/contract/executor/paral_sm/d_storage.h"
#include "platform/common/queue/lock_free_queue.h"

namespace resdb {
namespace contract {
namespace paral_sm {

class XController : public ConcurrencyController {
  public:
    XController(DataStorage * storage, int window_size);
    ~XController();

    virtual void PushCommit(int64_t commit_id, const ModifyMap& local_changes_);
    bool Commit(int64_t commit_id);

    std::vector<int64_t>& GetRedo();
    std::vector<int64_t>& GetDone();

    typedef std::map<uint256_t, std::deque<int64_t> > CommitList;

  private:
    void Clear();


    bool PreCommit(int64_t commit_id);
    bool PreCommitInternal(int64_t commit_id);
    void MergeChangeList(int64_t commit_id);

    bool PostCommit(int64_t commit_id);
    void RedoConflict(int64_t commit_id);

    bool CheckCommit(int64_t commit_id, const CommitList *commit_list, bool is_pre);
    bool CheckPreCommit(int64_t commit_id);

    const ModifyMap * GetChangeList(int64_t commit_id);
    void Remove(int64_t commit_id);


    void CommitDone(int64_t commit_id);
    void RedoCommit(int64_t commit_id, int flag);

    void RollBackData(const uint256_t& address, const Data& data);

    bool IsRead(const uint256_t& address, int64_t commit_id);

  private:
    int window_size_ = 1000;

    std::vector<ModifyMap> changes_list_,rechanges_list_;

    CommitList commit_list_[1024];
    int64_t last_commit_id_, current_commit_id_;

    CommitList pre_commit_list_[1024];
    int64_t last_pre_commit_id_;

    std::atomic<int> is_redo_[1024];
    std::vector<int64_t> redo_;

    //std::atomic<bool> is_done_[1024];
    std::vector<int64_t> done_;

    D_Storage * storage_;
};

}
}
} // namespace resdb

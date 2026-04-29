#pragma once

#include <set>
#include <deque>
#include <thread>
#include <shared_mutex>

#include "service/contract/executor/x_manager/concurrency_controller.h"
#include "service/contract/executor/x_manager/d_storage.h"
#include "platform/common/queue/lock_free_queue.h"

namespace resdb {
namespace contract {
namespace x_manager {

class StreamingEController : public ConcurrencyController {
  public:
    StreamingEController(DataStorage * storage, int window_size);
    virtual ~StreamingEController();

    // ==============================
    virtual void Store(const int64_t commit_id, const uint256_t& key, const uint256_t& value, int version);
    virtual uint256_t Load(const int64_t commit_id, const uint256_t& key, int version);
    bool Remove(const int64_t commit_id, const uint256_t& key, int version);


    // ==============================
    typedef std::map<uint256_t, std::deque<int64_t> > CommitList;
    typedef std::map<uint256_t, std::map<int64_t, Data > > PreCommitList;

    std::vector<int64_t>& GetRedo();
    std::vector<int64_t>& GetDone();

    void PushCommit(int64_t commit_id, const ModifyMap& local_changes_){}
    bool Commit(int64_t commit_id);

    void StoreInternal(const int64_t commit_id, const uint256_t& key, const uint256_t& value, int version);
    uint256_t LoadInternal(const int64_t commit_id, const uint256_t& key, int version);
    void SetPrecommitCallback( std::function<void (int64_t)> precommit_callback);

  private:
    void Clear();


    bool PreCommit(int64_t commit_id);
    bool PostCommit(int64_t commit_id);

    void RedoConflict(int64_t commit_id);
    bool CheckPreCommit(int64_t commit_id);
    bool CheckFirstFromPreCommit(const uint256_t& address, int64_t commit_id);
    bool CheckFirstFromCommit(const uint256_t& address, int64_t commit_id);
    /*
    bool PreCommitInternal(int64_t commit_id);
    void MergeChangeList(int64_t commit_id);


    bool CheckCommit(int64_t commit_id, const CommitList *commit_list, bool is_pre);

    const ModifyMap * GetChangeList(int64_t commit_id);
    */
    void Remove(int64_t commit_id);
    void CleanOldData(int64_t commit_id);

    int64_t RemovePrecommitRecord(const uint256_t& address, int64_t commit_id);

    bool CheckCommit(int64_t commit_id, bool is_pre);
    void CommitDone(int64_t commit_id);
    void RedoCommit(int64_t commit_id, int flag);

/*

    bool IsRead(const uint256_t& address, int64_t commit_id);

    */

    void RollBackData(const uint256_t& address, const Data& data);

    void AppendPreRecord(const uint256_t& address, 
        int64_t commit_id, Data& data, int version);
    void AppendPreRecord(const uint256_t& address, int hash_idx, 
      const std::vector<int64_t>& commit_id);

  private:
    int window_size_ = 1000;

    std::vector<ModifyMap> changes_list_,rechanges_list_;

    CommitList commit_list_[1024];
    int64_t last_commit_id_, current_commit_id_;

    PreCommitList pre_commit_list_[1024] GUARDED_BY(mutex_);
    int64_t last_pre_commit_id_;

    std::atomic<int> is_redo_[1024];
    std::vector<int64_t> redo_;

    //std::atomic<bool> is_done_[1024];
    std::vector<int64_t> done_;

    D_Storage * storage_;
    //std::mutex mutex_[1024];
    mutable std::shared_mutex mutex_[1024];


    std::function<void (int64_t)> precommit_callback_;
};

}
}
} // namespace resdb

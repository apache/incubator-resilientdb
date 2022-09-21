#pragma once
#include <functional>
#include <thread>

#include "common/queue/lock_free_queue.h"
#include "config/resdb_config.h"
#include "execution/system_info.h"
#include "execution/transaction_executor_impl.h"
#include "proto/resdb.pb.h"
#include "statistic/stats.h"

namespace resdb {

// Execute the requests that may contain system information or client requests.
class TransactionExecutor {
 public:
  typedef std::function<void(std::unique_ptr<Request>,
                             std::unique_ptr<BatchClientResponse> resp)>
      PostExecuteFunc;
  typedef std::function<void(Request*)> PreExecuteFunc;
  typedef std::function<void(uint64_t seq)> SeqUpdateNotifyFunc;

  TransactionExecutor(const ResDBConfig& config, PostExecuteFunc post_exec_func,
                      SystemInfo* system_info,
                      std::unique_ptr<TransactionExecutorImpl> executor_impl);
  ~TransactionExecutor();

  int Commit(std::unique_ptr<Request> request);

  // The max seq S that can be executed (have received all the seq before S).
  uint64_t GetMaxPendingExecutedSeq();

  // When a transaction is ready to be executed (have received all the seq
  // before Txn) PreExecute func will be called.
  void SetPreExecuteFunc(PreExecuteFunc func);

  void SetSeqUpdateNotifyFunc(SeqUpdateNotifyFunc func);

 private:
  void Execute(std::unique_ptr<Request> request);
  std::unique_ptr<std::string> DoExecute(const Request& request);
  void OrderMessage();
  void ExecuteMessage();

  void AddNewData(std::unique_ptr<Request> message);
  std::unique_ptr<Request> GetNextData();

  bool IsStop();

  void UpdateMaxExecutedSeq(uint64_t seq);

 protected:
  ResDBConfig config_;

 private:
  std::atomic<uint64_t> next_execute_seq_ = 1;
  PreExecuteFunc pre_exec_func_ = nullptr;
  SeqUpdateNotifyFunc seq_update_notify_func_ = nullptr;
  PostExecuteFunc post_exec_func_ = nullptr;
  SystemInfo* system_info_ = nullptr;
  std::unique_ptr<TransactionExecutorImpl> executor_impl_ = nullptr;
  std::map<uint64_t, std::unique_ptr<Request>> candidates_;
  std::thread ordering_thread_, execute_thread_;
  LockFreeQueue<Request> commit_queue_, execute_queue_;
  std::atomic<bool> stop_;
  Stats* global_stats_ = nullptr;
};

}  // namespace resdb

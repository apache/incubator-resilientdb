/*
 * Copyright (c) 2019-2022 ExpoLab, UC Davis
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once
#include <functional>
#include <thread>

#include "absl/status/statusor.h"
#include "executor/common/transaction_manager.h"
#include "platform/common/queue/lock_free_queue.h"
#include "platform/config/resdb_config.h"
#include "platform/consensus/execution/system_info.h"
#include "platform/proto/resdb.pb.h"
#include "platform/statistic/stats.h"

namespace resdb {

// Execute the requests that may contain system information or user requests.
class TransactionExecutor {
 public:
  typedef std::function<void(std::unique_ptr<Request>,
                             std::unique_ptr<BatchUserResponse> resp)>
      PostExecuteFunc;
  typedef std::function<void(Request*)> PreExecuteFunc;
  typedef std::function<void(uint64_t seq)> SeqUpdateNotifyFunc;

  TransactionExecutor(const ResDBConfig& config, PostExecuteFunc post_exec_func,
                      SystemInfo* system_info,
                      std::unique_ptr<TransactionManager> transaction_manager);
  ~TransactionExecutor();

  void Stop();

  bool NeedResponse();

  int Commit(std::unique_ptr<Request> request);

  // The max seq S that can be executed (have received all the seq before S).
  uint64_t GetMaxPendingExecutedSeq();

  // When a transaction is ready to be executed (have received all the seq
  // before Txn) PreExecute func will be called.
  void SetPreExecuteFunc(PreExecuteFunc func);

  void SetSeqUpdateNotifyFunc(SeqUpdateNotifyFunc func);

  Storage* GetStorage();

  void Execute(std::unique_ptr<Request> request, bool need_execute = true);

  void AddExecuteMessage(std::unique_ptr<Request> message);
  void Prepare(std::unique_ptr<Request> message);

  void RegisterExecute(int64_t seq);
  void WaitForExecute(int64_t seq);
  void FinishExecute(int64_t seq);

  void SetUserFunc(std::function<void(int)> func) { user_func_ = func; }

 private:
  void OnlyExecute(std::unique_ptr<Request> request);

  std::unique_ptr<std::string> DoExecute(const Request& request);
  void OrderMessage();
  void ExecuteMessage();
  void ExecuteMessageOutOfOrder();
  void PrepareMessage();

  void AddNewData(std::unique_ptr<Request> message);
  std::unique_ptr<Request> GetNextData();

  bool IsStop();

  void UpdateMaxExecutedSeq(uint64_t seq);

  void CallBack(const BatchUserRequest& batch_request,
                std::unique_ptr<Request> request,
                std::unique_ptr<BatchUserResponse> resp);

 protected:
  ResDBConfig config_;

 private:
  std::atomic<uint64_t> next_execute_seq_ = 1;
  std::function<void(int)> user_func_ = nullptr;
  PreExecuteFunc pre_exec_func_ = nullptr;
  SeqUpdateNotifyFunc seq_update_notify_func_ = nullptr;
  PostExecuteFunc post_exec_func_ = nullptr;
  SystemInfo* system_info_ = nullptr;
  std::unique_ptr<TransactionManager> transaction_manager_ = nullptr;
  std::map<uint64_t, std::unique_ptr<Request>> candidates_;
  std::thread ordering_thread_, execute_OOO_thread_, gc_thread_;
  std::vector<std::thread> execute_thread_, prepare_thread_;
  LockFreeQueue<Request> commit_queue_, execute_OOO_queue_;
  LockFreeQueue<Request> execute_queue_, prepare_queue_;
  LockFreeQueue<uint64_t> gc_queue_;
  std::atomic<bool> stop_;
  Stats* global_stats_ = nullptr;
  int execute_thread_num_ = 4;

  static const int blucket_num_ = 1024;
  int blucket_[blucket_num_];
  std::condition_variable cv_;
  std::mutex mutex_, f_mutex_[2048], d_mutex_[2048], fd_mutex_[2048];

  std::map<uint64_t, std::unique_ptr<BatchUserRequest>> req_[2048];
  std::unordered_map<
      uint64_t,
      std::unique_ptr<std::vector<std::unique_ptr<google::protobuf::Message>>>>
      data_[2048];
  std::map<uint64_t, int> flag_[2048];
  std::condition_variable data_cv_;

  uint64_t num_, last_group_, start_time_, done_time_, create_time_,
      execute_start_;
};

}  // namespace resdb

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

#include "executor/common/transaction_manager.h"
#include "platform/common/queue/lock_free_queue.h"
#include "platform/config/resdb_config.h"
#include "platform/consensus/execution/system_info.h"
#include "platform/proto/resdb.pb.h"
#include "platform/statistic/stats.h"
#include "platform/consensus/execution/duplicate_manager.h"

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

  void SetDuplicateManager(DuplicateManager* manager);

  Storage* GetStorage();

 private:
  void Execute(std::unique_ptr<Request> request, bool need_execute = true);
  void OnlyExecute(std::unique_ptr<Request> request);

  std::unique_ptr<std::string> DoExecute(const Request& request);
  void OrderMessage();
  void ExecuteMessage();
  void ExecuteMessageOutOfOrder();

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
  std::unique_ptr<TransactionManager> transaction_manager_ = nullptr;
  std::map<uint64_t, std::unique_ptr<Request>> candidates_;
  std::thread ordering_thread_, execute_thread_, execute_OOO_thread_;
  LockFreeQueue<Request> commit_queue_, execute_queue_, execute_OOO_queue_;
  std::atomic<bool> stop_;
  Stats* global_stats_ = nullptr;
  DuplicateManager *duplicate_manager_;
};

}  // namespace resdb

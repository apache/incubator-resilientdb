/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#pragma once
#include <functional>
#include <thread>

#include "executor/common/transaction_manager.h"
#include "platform/common/queue/lock_free_queue.h"
#include "platform/config/resdb_config.h"
#include "platform/consensus/execution/duplicate_manager.h"
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
  DuplicateManager* duplicate_manager_;
};

}  // namespace resdb

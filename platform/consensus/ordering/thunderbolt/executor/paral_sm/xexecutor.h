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

#include <future>
#include <shared_mutex>

#include "absl/status/statusor.h"
#include "eEVM/opcode.h"
#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/thunderbolt/executor/paral_sm/committer_context.h"
#include "platform/consensus/ordering/thunderbolt/executor/paral_sm/contract_committer.h"
#include "platform/consensus/ordering/thunderbolt/executor/paral_sm/contract_executor.h"
#include "platform/consensus/ordering/thunderbolt/executor/paral_sm/global_state.h"
#include "platform/consensus/ordering/thunderbolt/executor/paral_sm/utils.h"
#include "platform/consensus/ordering/thunderbolt/executor/paral_sm/xcontroller.h"
#include "proto/contract/func_params.pb.h"

namespace resdb {
namespace contract {
namespace paral_sm {

class XExecutor : public ContractCommitter {
 public:
  XExecutor(
      DataStorage* storage, GlobalState* global_state, int window_size,
      std::function<void(std::unique_ptr<ExecuteResp>)> call_back = nullptr,
      int worker_num = 2);

  ~XExecutor();

  void SetExecuteCallBack(
      std::function<void(std::unique_ptr<ExecuteResp>)>) override;

  void AsyncExecContract(std::vector<ContractExecuteInfo>& request) override;

  absl::StatusOr<std::string> ExecContract(const Address& caller_address,
                                           const Address& contract_address,
                                           const std::string& func_addr,
                                           const Params& func_param,
                                           EVMState* state);

  std::vector<std::unique_ptr<ExecuteResp>> ExecContract(
      std::vector<ContractExecuteInfo>& execute_info) {
    return {};
  }

 private:
  void AddTask(int64_t commit_id, std::unique_ptr<ExecutionContext> comtext);
  void RemoveTask(int64_t commit_id);
  void ResponseProcess();
  ExecutionContext* GetTaskContext(int64_t commit_id);

  bool WaitNext();
  bool WaitAll();

  void CallBack(uint64_t commit_id);

 private:
  std::unique_ptr<XController> controller_;
  std::unique_ptr<ContractExecutor> executor_;
  DataStorage* storage_;
  GlobalState* gs_;
  std::vector<std::thread> workers_;
  std::thread response_;
  std::atomic<bool> is_stop_;
  std::atomic<uint64_t> first_id_, last_id_, id_;

  LockFreeQueue<ExecutionContext> request_queue_;
  LockFreeQueue<ExecutionContext> resp_queue_;
  std::vector<std::unique_ptr<ContractExecutor>> resp_list_;
  std::vector<bool> is_done_;

  std::map<int64_t, std::unique_ptr<ExecutionContext>> context_list_;

  const int worker_num_;
  int window_size_;
  std::function<void(std::unique_ptr<ExecuteResp>)> call_back_;
  std::condition_variable cv_;
  std::mutex mutex_;
};

}  // namespace paral_sm
}  // namespace contract
}  // namespace resdb

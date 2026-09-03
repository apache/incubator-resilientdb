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
#include "platform/consensus/ordering/thunderbolt/executor/common/utils.h"
#include "platform/consensus/ordering/thunderbolt/executor/manager/committer_context.h"
#include "platform/consensus/ordering/thunderbolt/executor/manager/contract_committer.h"
#include "platform/consensus/ordering/thunderbolt/executor/manager/contract_executor.h"
#include "platform/consensus/ordering/thunderbolt/executor/manager/global_state.h"
#include "platform/consensus/ordering/thunderbolt/executor/manager/x_controller.h"
#include "proto/contract/func_params.pb.h"

namespace resdb {
namespace contract {

struct ExecutionState {
  std::atomic<int> commit_time;
  int redo_time = 0;
};

class XCommitter : public ContractCommitter {
 public:
  XCommitter(DataStorage* storage, GlobalState* global_state,
             int worker_num = 2);

  ~XCommitter();

  std::vector<std::unique_ptr<ExecuteResp>> ExecContract(
      std::vector<ContractExecuteInfo>& request) override;

  absl::StatusOr<std::string> ExecContract(const Address& caller_address,
                                           const Address& contract_address,
                                           const std::string& func_addr,
                                           const Params& func_param,
                                           EVMState* state);

 private:
  void AddTask(int64_t commit_id, std::unique_ptr<ExecutionContext> comtext);
  void RemoveTask(int64_t commit_id);
  ExecutionContext* GetTaskContext(int64_t commit_id);

 private:
  std::unique_ptr<XController> controller_;
  std::unique_ptr<ContractExecutor> executor_;
  DataStorage* storage_;
  GlobalState* gs_;
  std::vector<std::thread> workers_;
  std::atomic<bool> is_stop_;

  LockFreeQueue<ExecutionContext*> request_queue_;
  LockFreeQueue<ExecuteResp> resp_queue_;

  std::map<int64_t, std::unique_ptr<ExecutionContext>> context_list_;

  const int worker_num_;
  ExecutionState execution_state_;
};

}  // namespace contract
}  // namespace resdb

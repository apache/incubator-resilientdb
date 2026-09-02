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

#include <future>
#include <shared_mutex>

#include "absl/status/statusor.h"
#include "eEVM/opcode.h"
#include "platform/common/queue/lock_free_queue.h"
#include "service/contract/executor/common/utils.h"
#include "service/contract/executor/manager/committer_context.h"
#include "service/contract/executor/manager/contract_committer.h"
#include "service/contract/executor/manager/contract_executor.h"
#include "service/contract/executor/manager/global_state.h"
#include "service/contract/executor/manager/sequential_cc_controller.h"
#include "service/contract/proto/func_params.pb.h"

namespace resdb {
namespace contract {

/*
class ExecutionContext {
public:
  ExecutionContext(const ContractExecuteInfo & info ) ;
  const ContractExecuteInfo * GetContractExecuteInfo() const;

  void SetRedo();
  bool IsRedo();

  void SetResult(std::unique_ptr<ExecuteResp> result);
  std::unique_ptr<ExecuteResp> FetchResult();

  private:
    bool is_redo_ = false;
    std::unique_ptr<ExecuteResp> result_;
    std::unique_ptr<ContractExecuteInfo> info_;
};
*/

struct ExecutionState {
  std::atomic<int> commit_time;
  int redo_time = 0;
};

class SequentialConcurrencyCommitter : public ContractCommitter {
 public:
  SequentialConcurrencyCommitter(DataStorage* storage,
                                 GlobalState* global_state, int worker_num = 2);

  ~SequentialConcurrencyCommitter();

  std::vector<std::unique_ptr<ExecuteResp>> ExecContract(
      std::vector<ContractExecuteInfo>& request) override;

  absl::StatusOr<std::string> ExecContract(const Address& caller_address,
                                           const Address& contract_address,
                                           const std::string& func_addr,
                                           const Params& func_param,
                                           EVMState* state);

  ExecutionState* GetExecutionState();

 private:
  void AddTask(int64_t commit_id, std::unique_ptr<ExecutionContext> comtext);
  void RemoveTask(int64_t commit_id);
  ExecutionContext* GetTaskContext(int64_t commit_id);

  void CommitCallBack(int64_t commit_id);
  void RedoCallBack(int64_t commit_id, int flag);

 private:
  std::unique_ptr<SequentialCCController> controller_;
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

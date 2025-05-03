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

#include <shared_mutex>
#include <future>

#include "absl/status/statusor.h"
#include "eEVM/opcode.h"
#include "platform/common/queue/lock_free_queue.h"
#include "service/contract/executor/x_manager/global_state.h"
#include "service/contract/executor/x_manager/contract_committer.h"
#include "service/contract/executor/x_manager/contract_executor.h"
#include "service/contract/executor/x_manager/committer_context.h"
#include "service/contract/executor/x_manager/utils.h"
#include "service/contract/proto/func_params.pb.h"

namespace resdb {
namespace contract {
namespace x_manager {

class SeqCommitter : public ContractCommitter {
 public:
  SeqCommitter(
    DataStorage * storage,
    GlobalState * global_state, 
    int window_size,
    int worker_num = 2);

    ~SeqCommitter();

  //void SetExecuteCallBack(std::function<void (std::unique_ptr<ExecuteResp>)> ) override;

  void AsyncExecContract(std::vector<ContractExecuteInfo>& request) override;

  absl::StatusOr<std::string> ExecContract(
      const Address& caller_address, const Address& contract_address,
      const std::string& func_addr,
      const Params& func_param, EVMState * state);

  std::vector<std::unique_ptr<ExecuteResp>> ExecContract(std::vector<ContractExecuteInfo>& execute_info); 

private:
  void AddTask(int64_t commit_id, std::unique_ptr<ExecutionContext> comtext);
  void RemoveTask(int64_t commit_id);
  void ResponseProcess();
  ExecutionContext* GetTaskContext(int64_t commit_id);

  bool WaitNext();
  bool WaitAll();

  void CallBack(uint64_t commit_id);

 private:
  GlobalState* gs_;
  std::vector<std::thread> workers_;
  std::thread response_;
  std::atomic<bool> is_stop_;
  std::unique_ptr<ContractExecutor> executor_;

  std::vector<std::unique_ptr<ExecuteResp>> resp_list_;

  const int worker_num_;
  int window_size_;
};

}
}  // namespace contract
}  // namespace resdb

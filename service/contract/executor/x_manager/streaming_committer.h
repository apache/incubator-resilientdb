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
#include <queue>
#include <shared_mutex>

#include "absl/status/statusor.h"
#include "eEVM/opcode.h"
#include "platform/common/queue/lock_free_queue.h"
#include "service/contract/executor/x_manager/committer_context.h"
#include "service/contract/executor/x_manager/contract_committer.h"
#include "service/contract/executor/x_manager/contract_executor.h"
#include "service/contract/executor/x_manager/global_state.h"
#include "service/contract/executor/x_manager/streaming_controller.h"
#include "service/contract/executor/x_manager/utils.h"
#include "service/contract/proto/func_params.pb.h"

namespace resdb {
namespace contract {
namespace x_manager {

class StreamingCommitter : public ContractCommitter {
 public:
  StreamingCommitter(DataStorage* storage, GlobalState* global_state,
                     std::function<void(int)> call_back = nullptr,
                     int worker_num = 2);

  ~StreamingCommitter();

  void SetExecuteCallBack(std::function<void(int)>);

  void AsyncExecContract(
      int exe_id,
      std::vector<std::pair<ContractExecuteInfo, int64_t>>& requests);

  std::vector<std::unique_ptr<ExecuteResp>> ExecContract(
      std::vector<ContractExecuteInfo>& request) override {
    return std::vector<std::unique_ptr<ExecuteResp>>();
  }

  absl::StatusOr<std::string> ExecContract(const Address& caller_address,
                                           const Address& contract_address,
                                           const std::string& func_addr,
                                           const Params& func_param,
                                           EVMState* state);

 private:
  void AddTask(int64_t commit_id, std::unique_ptr<ExecutionContext> comtext);
  void RemoveTask(int64_t commit_id);
  void ResponseProcess();
  ExecutionContext* GetTask(int64_t commit_id);

  void CallBack(uint64_t exe_id);

  void RemoveNode(int idx);
  void AddEdge(int s_id, int e_idx);
  void Notify(int idx);

 private:
  std::unique_ptr<StreamingController> controller_;
  std::unique_ptr<ContractExecutor> executor_;
  DataStorage* storage_;
  GlobalState* gs_;
  std::vector<std::thread> workers_;
  std::thread response_;
  std::atomic<bool> is_stop_;
  std::atomic<uint64_t> id_;

  LockFreeQueue<ExecutionContext*> request_queue_;
  LockFreeQueue<ExecuteResp> resp_queue_;

  std::map<int64_t, std::unique_ptr<ExecutionContext>> context_list_;

  const int worker_num_;
  std::function<void(int)> call_back_;
  std::mutex mutex_;

  std::mutex g_mutex_, req_mutex_;
  std::map<int, std::vector<int>> g_;
  std::map<int, int> din_, req_id_, resp_list_;
  std::map<int, int64_t> groups_;
  std::map<int, std::queue<int>> group_;
};

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb

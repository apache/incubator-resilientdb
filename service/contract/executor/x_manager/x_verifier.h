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
#include "service/contract/executor/x_manager/committer_context.h"
#include "service/contract/executor/x_manager/contract_committer.h"
#include "service/contract/executor/x_manager/contract_verifier.h"
#include "service/contract/executor/x_manager/global_state.h"
#include "service/contract/executor/x_manager/utils.h"
#include "service/contract/executor/x_manager/v_controller.h"
#include "service/contract/proto/func_params.pb.h"

namespace resdb {
namespace contract {
namespace x_manager {

class XVerifier : public ContractVerifier {
 public:
  XVerifier(DataStorage* storage, GlobalState* global_state,
            int worker_num = 2);

  ~XVerifier();

  bool RWSEqual(const ModifyMap& a, const ModifyMap& b);

  bool VerifyContract(std::map<int, VerifyInfoType>& ranklist) override;

 private:
  DataStorage* storage_;
  GlobalState* gs_;
  std::vector<std::thread> workers_;
  std::unique_ptr<VController> controller_;
  std::atomic<bool> is_stop_;

  LockFreeQueue<int> x_request_queue_;
  LockFreeQueue<ExecutionContext> request_queue_;
  LockFreeQueue<ExecuteResp> resp_queue_;
  int worker_num_;

  std::unique_ptr<ContractExecuteInfo> request_[20000];
  // std::unique_ptr<ContractExecuteInfo> request_[2048];
};

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb

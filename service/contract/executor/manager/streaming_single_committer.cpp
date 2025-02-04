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

#include "service/contract/executor/manager/streaming_single_committer.h"

#include <future>
#include <queue>

#include "common/utils/utils.h"
#include "eEVM/processor.h"
#include "glog/logging.h"

namespace resdb {
namespace contract {
namespace streaming {

StreamingSingleCommitter::StreamingSingleCommitter(
    DataStorage* storage, GlobalState* global_state, int window_size,
    std::function<void(std::unique_ptr<ExecuteResp>)> call_back, int worker_num)
    : storage_(storage), gs_(global_state), call_back_(call_back) {
  executor_ = std::make_unique<ContractExecutor>();

  id_ = 1;
}

void StreamingSingleCommitter::SetExecuteCallBack(
    std::function<void(std::unique_ptr<ExecuteResp>)> func) {
  call_back_ = std::move(func);
}

StreamingSingleCommitter::~StreamingSingleCommitter() {}

void StreamingSingleCommitter::Execute(const ContractExecuteInfo& request) {
  std::unique_ptr<ExecuteResp> resp = std::make_unique<ExecuteResp>();
  auto ret = ExecContract(request.caller_address, request.contract_address,
                          request.func_addr, request.func_params, gs_);
  resp->state = ret.status();
  resp->contract_address = request.contract_address;
  resp->commit_id = request.commit_id;
  resp->user_id = request.user_id;
  if (ret.ok()) {
    resp->ret = 0;
    resp->result = *ret;
    // LOG(ERROR)<<"commit :"<<resp->commit_id;
  } else {
    LOG(ERROR) << "commit :" << resp->commit_id << " fail";
    resp->ret = -1;
    assert(resp->ret >= 0);
  }
  if (call_back_) {
    call_back_(std::move(resp));
  }
}

void StreamingSingleCommitter::AsyncExecContract(
    std::vector<ContractExecuteInfo>& requests) {
  for (auto& request : requests) {
    request.commit_id = id_++;
    Execute(request);
  }

  return;
}

absl::StatusOr<std::string> StreamingSingleCommitter::ExecContract(
    const Address& caller_address, const Address& contract_address,
    const std::string& func_addr, const Params& func_param, EVMState* state) {
  return executor_->ExecContract(caller_address, contract_address, func_addr,
                                 func_param, state);
}

}  // namespace streaming
}  // namespace contract
}  // namespace resdb

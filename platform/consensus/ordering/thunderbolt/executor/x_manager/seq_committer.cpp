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

#include "service/contract/executor/x_manager/seq_committer.h"

#include <future>
#include <queue>

#include "common/utils/utils.h"
#include "eEVM/exception.h"
#include "eEVM/processor.h"
#include "glog/logging.h"
#include "service/contract/executor/x_manager/executor_state.h"

//#define Debug

namespace resdb {
namespace contract {
namespace x_manager {

SeqCommitter::SeqCommitter(DataStorage* storage, GlobalState* global_state,
                           int window_size, int worker_num)
    : gs_(global_state), worker_num_(worker_num), window_size_(window_size) {
  executor_ = std::make_unique<ContractExecutor>();
}

SeqCommitter::~SeqCommitter() {
  // LOG(ERROR)<<"desp";
  is_stop_ = true;
}

void SeqCommitter::AsyncExecContract(
    std::vector<ContractExecuteInfo>& requests) {
  return;
}

std::vector<std::unique_ptr<ExecuteResp>> SeqCommitter::ExecContract(
    std::vector<ContractExecuteInfo>& requests) {
  std::vector<std::unique_ptr<ExecuteResp>> resp_list;
  for (auto& request : requests) {
    auto ret = ExecContract(request.caller_address, request.contract_address,
                            request.func_addr, request.func_params, gs_);
    std::unique_ptr<ExecuteResp> resp = std::make_unique<ExecuteResp>();
    resp->contract_address = request.contract_address;
    resp->commit_id = request.commit_id;
    resp->user_id = request.user_id;
    resp->ret = 0;
    resp->result = *ret;
    resp_list.push_back(std::move(resp));
  }
  return resp_list;
}

absl::StatusOr<std::string> SeqCommitter::ExecContract(
    const Address& caller_address, const Address& contract_address,
    const std::string& func_addr, const Params& func_param, EVMState* state) {
  return executor_->ExecContract(caller_address, contract_address, func_addr,
                                 func_param, state);
}

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb

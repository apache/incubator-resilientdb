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

#include "service/contract/executor/manager/test_committer.h"

#include "glog/logging.h"
//#include "eEVM/processor.h"

namespace resdb {
namespace contract {

TestCommitter::TestCommitter(DataStorage* storage, GlobalState* global_state)
    : gs_(global_state) {
  controller_ = std::make_unique<TestController>(storage);
  executor_ = std::make_unique<ContractExecutor>();
}

TestCommitter::~TestCommitter() {}

std::vector<std::unique_ptr<ExecuteResp>> TestCommitter::ExecContract(
    const std::vector<ContractExecuteInfo>& requests) {
  std::vector<std::unique_ptr<ExecuteResp>> resp_list;
  for (const auto& request : requests) {
    std::unique_ptr<ExecuteResp> resp = std::make_unique<ExecuteResp>();
    // auto start_time = GetCurrentTime();
    auto ret = ExecContract(request.caller_address, request.contract_address,
                            request.func_addr, request.func_params, gs_);
    resp->state = ret.status();
    if (ret.ok()) {
      resp->ret = 0;
      resp->result = *ret;
    } else {
      LOG(ERROR) << "exec fail";
      resp->ret = -1;
    }
    resp->contract_address = request.contract_address;
    resp->commit_id = request.commit_id;

    resp_list.push_back(std::move(resp));
  }
  return resp_list;
}

absl::StatusOr<std::string> TestCommitter::ExecContract(
    const Address& caller_address, const Address& contract_address,
    const std::string& func_addr, const Params& func_param, EVMState* state) {
  return executor_->ExecContract(caller_address, contract_address, func_addr,
                                 func_param, state);
}

}  // namespace contract
}  // namespace resdb

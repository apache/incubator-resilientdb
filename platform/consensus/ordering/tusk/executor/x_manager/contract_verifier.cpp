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

#include "service/contract/executor/x_manager/contract_verifier.h"

namespace resdb {
namespace contract {
namespace x_manager {


ContractVerifier:: ContractVerifier(){
  executor_ = std::make_unique<ContractExecutor>();
}

ContractVerifier::~ContractVerifier(){
}

std::vector<std::unique_ptr<ExecuteResp>> ContractVerifier::ExecContract(std::vector<ContractExecuteInfo>& request ){
    return std::vector<std::unique_ptr<ExecuteResp>>();
}

absl::StatusOr<std::string> ContractVerifier::ExecContract(
      const Address& caller_address, const Address& contract_address,
      const std::string& func_addr,
      const Params& func_param, EVMState * state) {
  return executor_->ExecContract(caller_address, contract_address, func_addr, func_param, state);  
}

}
}  // namespace contract
}  // namespace resdb


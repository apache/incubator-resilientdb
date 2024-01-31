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

#include <thread>

#include "service/contract/executor/manager/contract_executor.h"
#include "service/contract/executor/common/contract_execute_info.h"
#include "service/contract/proto/func_params.pb.h"

#include "absl/status/statusor.h"
#include "eEVM/opcode.h"
#include "eEVM/address.h"

namespace resdb {
namespace contract {

/*
struct ContractExecuteInfo {
  eevm::Address caller_address;
  eevm::Address contract_address;
  std::string func_addr;
  Params func_params;
  int64_t commit_id;
  uint64_t user_id;
  ContractExecuteInfo(){}
  ContractExecuteInfo(
    eevm::Address caller_address,
    eevm::Address contract_address,
    std::string func_addr,
    Params func_params,
    int64_t commit_id): caller_address(caller_address), contract_address(contract_address), func_addr(func_addr), func_params(func_params), commit_id(commit_id){}
};
*/

class ContractCommitter {
 public:
  ContractCommitter() = default;
  virtual ~ContractCommitter() = default;

  virtual std::vector<std::unique_ptr<ExecuteResp>> ExecContract(std::vector<ContractExecuteInfo>& request) = 0;

  virtual void AsyncExecContract(std::vector<ContractExecuteInfo>& request){};

  virtual absl::StatusOr<std::string> ExecContract(
      const Address& caller_address, const Address& contract_address,
      const std::string& func_addr,
      const Params& func_param, EVMState * state) = 0;
  
  virtual void SetExecuteCallBack(std::function<void (std::unique_ptr<ExecuteResp>)> ){};
};

}  // namespace contract
}  // namespace resdb

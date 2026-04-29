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

#include "absl/status/statusor.h"
#include "eEVM/opcode.h"
#include "service/contract/executor/x_manager/contract_committer.h"
#include "service/contract/executor/x_manager/contract_deployer.h"
#include "service/contract/executor/x_manager/global_state.h"
#include "service/contract/executor/x_manager/utils.h"
#include "service/contract/proto/func_params.pb.h"

namespace resdb {
namespace contract {
namespace x_manager {

class ContractManager {
 public:
  enum Options {
    Streaming = 1,
  };
  ContractManager(std::unique_ptr<DataStorage> storage, 
      int worker_num = 2, Options op = Streaming );

 public:
  Address DeployContract(const Address& owner_address,
                         const DeployInfo& deploy_info);

  absl::StatusOr<eevm::AccountState> GetContract(const Address& address);

  absl::StatusOr<std::string> ExecContract(const Address& caller_address,
                                           const Address& contract_address,
                                           const Params& func_param);

  std::vector<std::unique_ptr<ExecuteResp>> ExecContract(std::vector<ContractExecuteInfo>& execute_info);

  void AsyncExecContract(std::vector<ContractExecuteInfo>& execute_info);

  void SetExecuteCallBack(std::function<void (std::unique_ptr<ExecuteResp> resp)> func);

 private:
  std::string GetFuncAddress(const Address& contract_address,
                             const std::string& func_name);
  void SetFuncAddress(const Address& contract_address, const FuncInfo& func);

 private:
  std::unique_ptr<DataStorage> storage_;
  std::unique_ptr<GlobalState> gs_;
  std::unique_ptr<ContractCommitter> committer_;
  std::unique_ptr<ContractDeployer> deployer_;
};

}
}  // namespace contract
}  // namespace resdb

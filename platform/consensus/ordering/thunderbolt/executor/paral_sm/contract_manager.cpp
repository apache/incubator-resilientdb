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

#include "service/contract/executor/x_manager/contract_manager.h"

#include <glog/logging.h>

#include "eEVM/processor.h"
#include "service/contract/executor/x_manager/address_manager.h"
#include "service/contract/executor/x_manager/streaming_e_committer.h"

namespace resdb {
namespace contract {
namespace x_manager {

ContractManager::ContractManager(std::unique_ptr<DataStorage> storage,
                                 int worker_num, Options op) {
  storage_ = std::move(storage);
  gs_ = std::make_unique<GlobalState>(storage_.get());

  committer_ = std::make_unique<StreamingECommitter>(storage_.get(), gs_.get(),
                                                     500, nullptr, worker_num);

  deployer_ = std::make_unique<ContractDeployer>(committer_.get(), gs_.get());
}

void ContractManager::SetExecuteCallBack(
    std::function<void(std::unique_ptr<ExecuteResp> resp)> func) {
  committer_->SetExecuteCallBack(std::move(func));
}

Address ContractManager::DeployContract(const Address& owner_address,
                                        const DeployInfo& deploy_info) {
  return deployer_->DeployContract(owner_address, deploy_info);
}

absl::StatusOr<eevm::AccountState> ContractManager::GetContract(
    const Address& address) {
  return deployer_->GetContract(address);
}

std::vector<std::unique_ptr<ExecuteResp>> ContractManager::ExecContract(
    std::vector<ContractExecuteInfo>& execute_info) {
  for (int i = 0; i < execute_info.size(); ++i) {
    std::string func_addr =
        deployer_->GetFuncAddress(execute_info[i].contract_address,
                                  execute_info[i].func_params.func_name());
    if (func_addr.empty()) {
      LOG(ERROR) << "no fouction:" << execute_info[i].func_params.func_name();
      execute_info[i].contract_address = 0;
      continue;
    }
    execute_info[i].func_addr = func_addr;
    execute_info[i].commit_id = i + 1;
  }
  return committer_->ExecContract(execute_info);
}

void ContractManager::AsyncExecContract(
    std::vector<ContractExecuteInfo>& execute_info) {
  for (int i = 0; i < execute_info.size(); ++i) {
    std::string func_addr =
        deployer_->GetFuncAddress(execute_info[i].contract_address,
                                  execute_info[i].func_params.func_name());
    if (func_addr.empty()) {
      LOG(ERROR) << "no fouction:" << execute_info[i].func_params.func_name();
      execute_info[i].contract_address = 0;
      continue;
    }
    execute_info[i].func_addr = func_addr;
    execute_info[i].commit_id = i + 1;
  }
  committer_->AsyncExecContract(execute_info);
}

absl::StatusOr<std::string> ContractManager::ExecContract(
    const Address& caller_address, const Address& contract_address,
    const Params& func_param) {
  std::string func_addr =
      deployer_->GetFuncAddress(contract_address, func_param.func_name());
  if (func_addr.empty()) {
    LOG(ERROR) << "no fouction:" << func_param.func_name();
    return absl::InvalidArgumentError("Func not exist.");
  }

  absl::StatusOr<std::string> result = committer_->ExecContract(
      caller_address, contract_address, func_addr, func_param, gs_.get());
  if (result.ok()) {
    return *result;
  }
  return result.status();
}

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb

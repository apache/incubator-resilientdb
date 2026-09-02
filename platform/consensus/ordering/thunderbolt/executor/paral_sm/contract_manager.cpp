/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include "platform/consensus/ordering/thunderbolt/executor/x_manager/contract_manager.h"

#include <glog/logging.h>

#include "eEVM/processor.h"
#include "platform/consensus/ordering/thunderbolt/executor/x_manager/address_manager.h"
#include "platform/consensus/ordering/thunderbolt/executor/x_manager/streaming_e_committer.h"

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

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
#pragma once

#include <thread>

#include "absl/status/statusor.h"
#include "eEVM/address.h"
#include "eEVM/opcode.h"
#include "platform/consensus/ordering/thunderbolt/executor/common/contract_execute_info.h"
#include "platform/consensus/ordering/thunderbolt/executor/manager/contract_executor.h"
#include "proto/contract/func_params.pb.h"

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
    int64_t commit_id): caller_address(caller_address),
contract_address(contract_address), func_addr(func_addr),
func_params(func_params), commit_id(commit_id){}
};
*/

class ContractCommitter {
 public:
  ContractCommitter() = default;
  virtual ~ContractCommitter() = default;

  virtual std::vector<std::unique_ptr<ExecuteResp>> ExecContract(
      std::vector<ContractExecuteInfo>& request) = 0;

  virtual void AsyncExecContract(std::vector<ContractExecuteInfo>& request){};

  virtual absl::StatusOr<std::string> ExecContract(
      const Address& caller_address, const Address& contract_address,
      const std::string& func_addr, const Params& func_param,
      EVMState* state) = 0;

  virtual void SetExecuteCallBack(
      std::function<void(std::unique_ptr<ExecuteResp>)>){};
};

}  // namespace contract
}  // namespace resdb

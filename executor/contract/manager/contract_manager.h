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

#include "absl/status/statusor.h"
#include "eEVM/opcode.h"
#include "eEVM/simple/simpleglobalstate.h"
#include "executor/contract/manager/utils.h"
#include "proto/contract/func_params.pb.h"

namespace resdb {
namespace contract {

class ContractManager {
 public:
  ContractManager();

 public:
  Address DeployContract(const Address& owner_address,
                         const DeployInfo& deploy_info);

  absl::StatusOr<eevm::AccountState> GetContract(const Address& address);

  absl::StatusOr<std::string> ExecContract(const Address& caller_address,
                                           const Address& contract_address,
                                           const Params& func_param);

 private:
  std::string GetFuncAddress(const Address& contract_address,
                             const std::string& func_name);
  void SetFuncAddress(const Address& contract_address, const FuncInfo& func);

  absl::StatusOr<std::vector<uint8_t>> Execute(
      const Address& owner_address, const Address& contract_address,
      const std::vector<uint8_t>& func_para);

 private:
  std::unique_ptr<eevm::SimpleGlobalState> gs_;
  std::map<Address, std::map<std::string, std::string>> func_address_;
};

}  // namespace contract
}  // namespace resdb

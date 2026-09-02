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

#include "platform/consensus/ordering/thunderbolt/executor/paral_sm/contract_committer.h"
#include "platform/consensus/ordering/thunderbolt/executor/paral_sm/global_state.h"
#include "proto/contract/func_params.pb.h"

namespace resdb {
namespace contract {
namespace paral_sm {

class ContractDeployer {
 public:
  ContractDeployer(ContractCommitter* committer, GlobalState* gs);

 public:
  Address DeployContract(const Address& owner_address,
                         const DeployInfo& deploy_info);

  Address DeployContract(const Address& owner_address,
                         const nlohmann::json& contract_json,
                         const std::vector<uint256_t>& init_params);

  absl::StatusOr<eevm::AccountState> GetContract(const Address& address);
  std::string GetFuncAddress(const Address& contract_address,
                             const std::string& func_name);

 private:
  void SetFuncAddress(const Address& contract_address, const FuncInfo& func);

 private:
  ContractCommitter* committer_;
  GlobalState* gs_;
  std::map<Address, std::map<std::string, std::string>> func_address_;
};

}  // namespace paral_sm
}  // namespace contract
}  // namespace resdb

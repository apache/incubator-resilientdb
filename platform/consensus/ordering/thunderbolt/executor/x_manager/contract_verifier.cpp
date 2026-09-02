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
#include "platform/consensus/ordering/thunderbolt/executor/x_manager/contract_verifier.h"

namespace resdb {
namespace contract {
namespace x_manager {

ContractVerifier::ContractVerifier() {
  executor_ = std::make_unique<ContractExecutor>();
}

ContractVerifier::~ContractVerifier() {}

std::vector<std::unique_ptr<ExecuteResp>> ContractVerifier::ExecContract(
    std::vector<ContractExecuteInfo>& request) {
  return std::vector<std::unique_ptr<ExecuteResp>>();
}

absl::StatusOr<std::string> ContractVerifier::ExecContract(
    const Address& caller_address, const Address& contract_address,
    const std::string& func_addr, const Params& func_param, EVMState* state) {
  return executor_->ExecContract(caller_address, contract_address, func_addr,
                                 func_param, state);
}

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb

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
#include "eEVM/opcode.h"
#include "platform/consensus/ordering/thunderbolt/executor/common/contract_execute_info.h"
#include "platform/consensus/ordering/thunderbolt/executor/common/utils.h"
#include "platform/consensus/ordering/thunderbolt/executor/manager/concurrency_controller.h"
#include "platform/consensus/ordering/thunderbolt/executor/manager/evm_state.h"
#include "proto/contract/func_params.pb.h"

namespace resdb {
namespace contract {

/*
struct ExecuteResp {
  int ret;
  absl::Status state;
  int64_t commit_id;
  Address contract_address;
  ConcurrencyController::ModifyMap rws;
  std::string result;
  int retry_time = 0;
  uint64_t user_id = 0;
  double runtime = 0;
};
*/

class ContractExecutor {
 public:
  ContractExecutor() = default;

  ~ContractExecutor() = default;

  absl::StatusOr<std::string> ExecContract(const Address& caller_address,
                                           const Address& contract_address,
                                           const std::string& func_addr,
                                           const Params& func_param,
                                           EVMState* state);

  absl::StatusOr<std::vector<uint8_t>> Execute(
      const Address& owner_address, const Address& contract_address,
      const std::vector<uint8_t>& func_params, EVMState* state);
};

}  // namespace contract
}  // namespace resdb

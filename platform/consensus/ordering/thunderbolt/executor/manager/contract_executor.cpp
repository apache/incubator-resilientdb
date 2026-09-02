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
#include "platform/consensus/ordering/thunderbolt/executor/manager/contract_executor.h"

//#include "common/utils/utils.h"

#include "eEVM/processor.h"
#include "glog/logging.h"

namespace resdb {
namespace contract {

void AppendArgToInput(std::vector<uint8_t>& code, const uint256_t& arg) {
  const auto pre_size = code.size();
  code.resize(pre_size + 32u);
  eevm::to_big_endian(arg, code.data() + pre_size);
}

void AppendArgToInput(std::vector<uint8_t>& code, const std::string& arg) {
  AppendArgToInput(code, eevm::to_uint256(arg));
}

absl::StatusOr<std::string> ContractExecutor::ExecContract(
    const Address& caller_address, const Address& contract_address,
    const std::string& func_addr, const Params& func_param, EVMState* state) {
  std::vector<uint8_t> inputs;
  if (!func_addr.empty()) {
    inputs = eevm::to_bytes(func_addr);
  }
  for (const std::string& param : func_param.param()) {
    AppendArgToInput(inputs, param);
  }

  auto result = Execute(caller_address, contract_address, inputs, state);

  if (result.ok()) {
    return eevm::to_hex_string(*result);
  } else {
    LOG(ERROR) << "execute fail:" << result.status();
  }
  return result.status();
}

absl::StatusOr<std::vector<uint8_t>> ContractExecutor::Execute(
    const Address& caller_address, const Address& contract_address,
    const std::vector<uint8_t>& input, EVMState* state) {
  // Ignore any logs produced by this transaction
  eevm::NullLogHandler ignore;
  eevm::Transaction tx(caller_address, ignore);

  // Record a trace to aid debugging
  eevm::Trace tr;
  eevm::Processor p(*state);

  // Run the transaction
  try {
    const auto exec_result =
        p.run(tx, caller_address, state->get(contract_address), input, 0u, &tr);

    if (exec_result.er != eevm::ExitReason::returned) {
      // Print the trace if nothing was returned
      if (exec_result.er == eevm::ExitReason::threw) {
        return absl::InternalError(
            fmt::format("Execution error: {}", exec_result.exmsg));
      }
      return absl::InternalError("Deployment did not return");
    }
    return exec_result.output;
  } catch (...) {
    return absl::InternalError(fmt::format("Execution error:"));
  }
}

}  // namespace contract
}  // namespace resdb

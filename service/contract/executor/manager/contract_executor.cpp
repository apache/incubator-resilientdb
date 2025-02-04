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

#include "service/contract/executor/manager/contract_executor.h"

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

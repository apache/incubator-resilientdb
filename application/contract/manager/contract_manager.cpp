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

#include "application/contract/manager/contract_manager.h"

#include <glog/logging.h>

#include "application/contract/manager/address_manager.h"
#include "eEVM/processor.h"

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

ContractManager::ContractManager() {
  gs_ = std::make_unique<eevm::SimpleGlobalState>();
}

std::string ContractManager::GetFuncAddress(const Address& contract_address,
                                            const std::string& func_name) {
  return func_address_[contract_address][func_name];
}

void ContractManager::SetFuncAddress(const Address& contract_address,
                                     const FuncInfo& func) {
  func_address_[contract_address][func.func_name()] = func.hash();
}

Address ContractManager::DeployContract(const Address& owner_address,
                                        const DeployInfo& deploy_info) {
  const auto contract_address =
      AddressManager::CreateContractAddress(owner_address);

  auto contract_constructor = eevm::to_bytes(deploy_info.contract_bin());
  for (const std::string& param : deploy_info.init_param()) {
    AppendArgToInput(contract_constructor, param);
  }

  try {
    auto contract = gs_->create(contract_address, 0u, contract_constructor);

    auto result = Execute(owner_address, contract_address, {});
    if (result.ok()) {
      // set the initialized class context code.
      contract.acc.set_code(std::move(*result));

      for (const auto& info : deploy_info.func_info()) {
        SetFuncAddress(contract_address, info);
      }
      return contract.acc.get_address();
    } else {
      gs_->remove(contract_address);
      return 0;
    }
  } catch (...) {
    LOG(ERROR) << "Deploy throw expection";
    return 0;
  }
}

absl::StatusOr<eevm::AccountState> ContractManager::GetContract(
    const Address& address) {
  if (!gs_->exists(address)) {
    return absl::InvalidArgumentError("Contract not exist.");
  }

  return gs_->get(address);
}

absl::StatusOr<std::string> ContractManager::ExecContract(
    const Address& caller_address, const Address& contract_address,
    const Params& func_param) {
  std::string func_addr =
      GetFuncAddress(contract_address, func_param.func_name());
  if (func_addr.empty()) {
    LOG(ERROR) << "no fouction:" << func_param.func_name();
    return absl::InvalidArgumentError("Func not exist.");
  }

  std::vector<uint8_t> inputs = eevm::to_bytes(func_addr);
  for (const std::string& param : func_param.param()) {
    AppendArgToInput(inputs, param);
  }

  auto result = Execute(caller_address, contract_address, inputs);

  if (result.ok()) {
    return eevm::to_hex_string(*result);
  }
  return result.status();
}

absl::StatusOr<std::vector<uint8_t>> ContractManager::Execute(
    const Address& caller_address, const Address& contract_address,
    const std::vector<uint8_t>& input) {
  // Ignore any logs produced by this transaction
  eevm::NullLogHandler ignore;
  eevm::Transaction tx(caller_address, ignore);

  // Record a trace to aid debugging
  eevm::Trace tr;
  eevm::Processor p(*gs_);

  // Run the transaction
  try {
    const auto exec_result =
        p.run(tx, caller_address, gs_->get(contract_address), input, 0u, &tr);

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

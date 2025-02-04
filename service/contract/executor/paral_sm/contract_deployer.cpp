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

#include "service/contract/executor/x_manager/contract_deployer.h"

#include <glog/logging.h>

#include "eEVM/processor.h"
#include "service/contract/executor/x_manager/address_manager.h"

namespace resdb {
namespace contract {
namespace x_manager {
namespace {

std::string U256ToString(uint256_t v) { return eevm::to_hex_string(v); }

void AppendArgToInput(std::vector<uint8_t>& code, const uint256_t& arg) {
  const auto pre_size = code.size();
  code.resize(pre_size + 32u);
  eevm::to_big_endian(arg, code.data() + pre_size);
  LOG(ERROR) << "add arg:" << arg;
}

void AppendArgToInput(std::vector<uint8_t>& code, const std::string& arg) {
  AppendArgToInput(code, eevm::to_uint256(arg));
}

}  // namespace

ContractDeployer::ContractDeployer(ContractCommitter* committer,
                                   GlobalState* gs)
    : committer_(committer), gs_(gs) {}

std::string ContractDeployer::GetFuncAddress(const Address& contract_address,
                                             const std::string& func_name) {
  return func_address_[contract_address][func_name];
}

void ContractDeployer::SetFuncAddress(const Address& contract_address,
                                      const FuncInfo& func) {
  func_address_[contract_address][func.func_name()] = func.hash();
}

Address ContractDeployer::DeployContract(const Address& owner_address,
                                         const DeployInfo& deploy_info) {
  const auto contract_address =
      AddressManager::CreateContractAddress(owner_address);

  auto contract_constructor = eevm::to_bytes(deploy_info.contract_bin());
  for (const std::string& param : deploy_info.init_param()) {
    AppendArgToInput(contract_constructor, param);
  }

  try {
    auto contract = gs_->create(contract_address, 0u, contract_constructor);
    absl::StatusOr<std::string> result =
        committer_->ExecContract(owner_address, contract_address, "", {}, gs_);

    if (result.ok()) {
      // set the initialized class context code.
      contract.acc.set_code(eevm::to_bytes(*result));

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

Address ContractDeployer::DeployContract(
    const Address& owner_address, const nlohmann::json& contract_json,
    const std::vector<uint256_t>& init_params) {
  DeployInfo deploy_info;
  deploy_info.set_contract_bin(contract_json["bin"]);

  for (auto& func : contract_json["hashes"].items()) {
    FuncInfo* new_func = deploy_info.add_func_info();
    new_func->set_func_name(func.key());
    new_func->set_hash(func.value());
  }

  for (const uint256_t& param : init_params) {
    deploy_info.add_init_param(U256ToString(param));
  }

  return DeployContract(owner_address, deploy_info);
}

absl::StatusOr<eevm::AccountState> ContractDeployer::GetContract(
    const Address& address) {
  if (!gs_->Exists(address)) {
    return absl::InvalidArgumentError("Contract not exist.");
  }

  return gs_->get(address);
}

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb

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

#pragma once

#include "service/contract/executor/x_manager/contract_committer.h"
#include "service/contract/executor/x_manager/global_state.h"
#include "service/contract/proto/func_params.pb.h"

namespace resdb {
namespace contract {
namespace x_manager {

class ContractDeployer {
 public:
  ContractDeployer(ContractCommitter * committer, GlobalState * gs);

 public:
  Address DeployContract(const Address& owner_address,
                         const DeployInfo& deploy_info);
  bool DeployContract(const Address& owner_address,
                         const DeployInfo& deploy_info, const Address& contract_address);

  Address DeployContract(const Address& owner_address,
      const nlohmann::json& contract_json, const std::vector<uint256_t>& init_params);
  bool DeployContract(const Address& owner_address,
      const nlohmann::json& contract_json, const std::vector<uint256_t>& init_params, const Address& contract_address);

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

}  // namespace contract
}
}  // namespace resdb

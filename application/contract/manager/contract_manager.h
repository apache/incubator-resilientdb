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

#include "absl/status/statusor.h"
#include "application/contract/manager/utils.h"
#include "application/contract/proto/func_params.pb.h"
#include "eEVM/opcode.h"
#include "eEVM/simple/simpleglobalstate.h"

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

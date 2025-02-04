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

#include "service/contract/executor/x_manager/address_manager.h"
#include "service/contract/executor/x_manager/contract_manager.h"
#include "service/contract/proto/func_params.pb.h"
#include "service/contract/proto/rpc.pb.h"

namespace resdb {
namespace contract {

class ExecutionHelper {
 public:
  static bool DeployWithAddress(
      const resdb::contract::Request& request,
      resdb::contract::x_manager::AddressManager* address_manager,
      resdb::contract::x_manager::ContractManager* manager);

  static absl::StatusOr<Contract> Deploy(
      const resdb::contract::Request& request,
      resdb::contract::x_manager::AddressManager* address_manager,
      resdb::contract::x_manager::ContractManager* contract_manager);

  static std::unique_ptr<ContractExecuteInfo> GetContractInfo(
      const resdb::contract::Request& request,
      resdb::contract::x_manager::AddressManager* address_manager);

  static absl::StatusOr<Contract> Deploy(
      const resdb::contract::Request& request);
  static absl::StatusOr<Account> CreateAccount(
      resdb::contract::x_manager::AddressManager* address_manager);
  static absl::StatusOr<Account> CreateAccountWithAddress(
      const std::string& address,
      resdb::contract::x_manager::AddressManager* address_manager);

  static bool DeployWithAddress(
      const resdb::contract::Request& request,
      resdb::contract::x_manager::AddressManager* address_manager);
  static bool CreateAccount(
      const std::string& address,
      resdb::contract::x_manager::AddressManager* address_manager);

  static std::unique_ptr<ModifyMap> GetRWSList(const ResultInfo& request);
};

}  // namespace contract
}  // namespace resdb

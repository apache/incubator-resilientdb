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

#include "application/contract/service/contract_executor.h"

#include <glog/logging.h>

namespace resdb {
namespace contract {

ContractExecutor::ContractExecutor(void)
    : contract_manager_(std::make_unique<ContractManager>()),
      address_manager_(std::make_unique<AddressManager>()) {}

std::unique_ptr<std::string> ContractExecutor::ExecuteData(
    const std::string& client_request) {
  Request request;
  Response response;

  if (!request.ParseFromString(client_request)) {
    LOG(ERROR) << "parse data fail";
    return nullptr;
  }

  int ret = 0;
  if (request.cmd() == Request::CREATE_ACCOUNT) {
    absl::StatusOr<Account> account_or = CreateAccount();
    if (account_or.ok()) {
      response.mutable_account()->Swap(&(*account_or));
    } else {
      ret = -1;
    }
  } else if (request.cmd() == Request::DEPLOY) {
    absl::StatusOr<Contract> contract_or = Deploy(request);
    if (contract_or.ok()) {
      response.mutable_contract()->Swap(&(*contract_or));
    } else {
      ret = -1;
    }
  } else if (request.cmd() == Request::EXECUTE) {
    auto res_or = Execute(request);
    if (res_or.ok()) {
      response.set_res(*res_or);
    } else {
      ret = -1;
    }
  }

  response.set_ret(ret);

  std::unique_ptr<std::string> resp_str = std::make_unique<std::string>();
  if (!response.SerializeToString(resp_str.get())) {
    return nullptr;
  }

  return resp_str;
}

absl::StatusOr<Account> ContractExecutor::CreateAccount() {
  std::string address =
      AddressManager::AddressToHex(address_manager_->CreateRandomAddress());
  Account account;
  account.set_address(address);
  return account;
}

absl::StatusOr<Contract> ContractExecutor::Deploy(const Request& request) {
  Address caller_address =
      AddressManager::HexToAddress(request.caller_address());
  if (!address_manager_->Exist(caller_address)) {
    LOG(ERROR) << "caller doesn't have an account";
    return absl::InvalidArgumentError("Account not exist.");
  }

  Address contract_address =
      contract_manager_->DeployContract(caller_address, request.deploy_info());

  if (contract_address > 0) {
    Contract contract;
    contract.set_owner_address(request.caller_address());
    contract.set_contract_address(
        AddressManager::AddressToHex(contract_address));
    contract.set_contract_name(request.deploy_info().contract_name());
    return contract;
  }
  return absl::InternalError("Deploy Contract fail.");
}

absl::StatusOr<std::string> ContractExecutor::Execute(const Request& request) {
  Address caller_address =
      AddressManager::HexToAddress(request.caller_address());
  if (!address_manager_->Exist(caller_address)) {
    LOG(ERROR) << "caller doesn't have an account";
    return absl::InvalidArgumentError("Account not exist.");
  }

  return contract_manager_->ExecContract(
      caller_address, AddressManager::HexToAddress(request.contract_address()),
      request.func_params());
}

}  // namespace contract
}  // namespace resdb

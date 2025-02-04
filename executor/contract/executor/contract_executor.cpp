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

#include "executor/contract/executor/contract_executor.h"

#include <glog/logging.h>

namespace resdb {
namespace contract {

ContractTransactionManager::ContractTransactionManager(void)
    : contract_manager_(std::make_unique<ContractManager>()),
      address_manager_(std::make_unique<AddressManager>()) {}

std::unique_ptr<BatchUserResponse> ContractTransactionManager::ExecuteBatch(
    const BatchUserRequest& request) {
  return ProcessRequest(request);
}

std::unique_ptr<BatchUserResponse> ContractTransactionManager::ProcessRequest(
    const resdb::BatchUserRequest& batch_request) {
  std::unique_ptr<BatchUserResponse> batch_response =
      std::make_unique<BatchUserResponse>();
  std::vector<ContractExecuteInfo> contract_list;

  for (int i = 0; i < batch_request.user_requests().size(); ++i) {
    auto& sub_request = batch_request.user_requests(i);
    const std::string& client_request = sub_request.request().data();

    resdb::contract::Request request;
    if (!request.ParseFromString(client_request)) {
      LOG(ERROR) << "parse data fail";
      continue;
    }

    resdb::contract::Response response;
    if (request.cmd() == Request::CREATE_ACCOUNT) {
      LOG(ERROR) << "create account:" << request.caller_address();
      absl::StatusOr<Account> account_or =
          CreateAccountWithAddress(request.caller_address());
      if (account_or.ok()) {
        response.mutable_account()->Swap(&(*account_or));
      } else {
        response.set_ret(-1);
      }

      std::string str;
      response.SerializeToString(&str);
      *batch_response->add_response() = str;

      continue;
    } else if (request.cmd() == Request::DEPLOY) {
      absl::StatusOr<Contract> contract_or = Deploy(request);
      if (contract_or.ok()) {
        response.mutable_contract()->Swap(&(*contract_or));
      } else {
        response.set_ret(-1);
      }
      std::string str;
      response.SerializeToString(&str);
      *batch_response->add_response() = str;
      continue;
    } else if (request.cmd() == Request::EXECUTE) {
      contract_list.push_back(GetContractInfo(request));
      contract_list.back().user_id = i;
    }
  }

  if (contract_list.size() > 0) {
    std::map<int, std::unique_ptr<ExecuteResp>> resp_list;

    std::vector<std::unique_ptr<ExecuteResp>> ret =
        manager_->ExecuteMulti(contract_list);

    for (auto& resp : ret) {
      resdb::contract::Response response;
      // response.set_res(resp_list[i]->result);
      std::string str;
      response.SerializeToString(&str);
      //*batch_response->add_response() = str;
      resp_list[resp->user_id] = str;
    }

    for (auto it : resp_list) {
      *batch_response->add_response() = it->second;
    }
  }
  return batch_response;
}

absl::StatusOr<Account> ContractTransactionManager::CreateAccountWithAddress(
    const std::string& address) {
  if (address.empty()) {
    return CreateAccount();
  }
  address_manager_->CreateAddress(eevm::to_uint256(address));
  Account account;
  account.set_address(address);
  return account;
}

absl::StatusOr<Account> ContractTransactionManager::CreateAccount() {
  std::string address =
      AddressManager::AddressToHex(address_manager_->CreateRandomAddress());
  Account account;
  account.set_address(address);
  return account;
}

absl::StatusOr<Contract> ContractTransactionManager::Deploy(
    const Request& request) {
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

absl::StatusOr<std::string> ContractTransactionManager::Execute(
    const Request& request) {
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

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

#include "executor/contract/executor/contract_executor.h"

#include <glog/logging.h>

namespace resdb {
namespace contract {

ContractTransactionManager::ContractTransactionManager(void)
    : contract_manager_(std::make_unique<ContractManager>()),
      address_manager_(std::make_unique<AddressManager>()) {}

std::unique_ptr<std::string> ContractTransactionManager::ExecuteData(
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
  } else if (request.cmd() == Request::ADD_ADDRESS) {  // New command handling
    absl::Status status = AddAddress(request);
    if (!status.ok()) {
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

absl::StatusOr<Account> ContractTransactionManager::CreateAccount() {
  std::string address =
      AddressManager::AddressToHex(address_manager_->CreateRandomAddress());
  Account account;
  account.set_address(address);
  return account;
}

absl::Status ContractTransactionManager::AddAddress(const Request& request) {
  Address address = AddressManager::HexToAddress(request.external_address());
  address_manager_->AddExternalAddress(address);
  return absl::OkStatus();
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

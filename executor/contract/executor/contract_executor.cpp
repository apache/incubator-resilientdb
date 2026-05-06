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

#include "eEVM/util.h"

namespace resdb {
namespace contract {

ContractTransactionManager::ContractTransactionManager(Storage* storage)
    : contract_manager_(std::make_unique<ContractManager>(storage)),
      address_manager_(std::make_unique<AddressManager>(storage)) {}

std::unique_ptr<std::string> ContractTransactionManager::ExecuteData(
    const std::string& client_request) {
  Request request;
  Response response;

  if (!request.ParseFromString(client_request)) {
    LOG(ERROR) << "parse data fail";
    return nullptr;
  }
  int ret = 0;
  if (request.cmd() == contract::Request::CREATE_ACCOUNT) {
    absl::StatusOr<Account> account_or = CreateAccount();
    if (account_or.ok()) {
      response.mutable_account()->Swap(&(*account_or));
      LOG(ERROR) << " create count:" << response.account().DebugString();
    } else {
      ret = -1;
    }
  } else if (request.cmd() == contract::Request::DEPLOY) {
    absl::StatusOr<Contract> contract_or = Deploy(request);
    if (contract_or.ok()) {
      response.mutable_contract()->Swap(&(*contract_or));
    } else {
      ret = -1;
    }
  } else if (request.cmd() == contract::Request::EXECUTE) {
    auto res_or = Execute(request);
    if (res_or.ok()) {
      response.set_res(*res_or);
    } else {
      ret = -1;
    }
  } else if (request.cmd() == resdb::contract::Request::GETBALANCE) {
    auto res_or = GetBalance(request);
    if (res_or.ok()) {
      response.set_res(*res_or);
    } else {
      ret = -1;
    }

  } else if (request.cmd() == resdb::contract::Request::SETBALANCE) {
    auto res_or = SetBalance(request);
    if (res_or.ok()) {
      response.set_res("1");
    } else {
      ret = -1;
    }
  } else if (request.cmd() == resdb::contract::Request::TRANSFER_ROK) {
    auto res_or = TransferRoK(request);
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

absl::StatusOr<std::string> ContractTransactionManager::GetBalance(
    const Request& request) {
  Address account = AddressManager::HexToAddress(request.account());
  return contract_manager_->GetBalance(account);
}

absl::StatusOr<std::string> ContractTransactionManager::SetBalance(
    const Request& request) {
  Address account = AddressManager::HexToAddress(request.account());
  Address balance = AddressManager::HexToAddress(request.balance());
  int ret = contract_manager_->SetBalance(account, balance);
  return std::to_string(ret);
}

absl::StatusOr<std::string> ContractTransactionManager::TransferRoK(
    const Request& request) {
  if (!request.has_from_account() || !request.has_to_account() ||
      !request.has_amount() || request.from_account().empty() ||
      request.to_account().empty() || request.amount().empty()) {
    return absl::InvalidArgumentError(
        "from_account, to_account, and amount (hex) are required.");
  }

  Address from = AddressManager::HexToAddress(request.from_account());
  Address to = AddressManager::HexToAddress(request.to_account());
  if (!address_manager_->Exist(from)) {
    return absl::InvalidArgumentError("From account not exist.");
  }
  if (!address_manager_->Exist(to)) {
    return absl::InvalidArgumentError("To account not exist.");
  }
  if (from == to) {
    return absl::InvalidArgumentError("From and to must differ.");
  }

  uint256_t amount = 0;
  try {
    amount = eevm::to_uint256(request.amount());
  } catch (...) {
    return absl::InvalidArgumentError("Invalid amount.");
  }
  if (amount == 0) {
    return absl::InvalidArgumentError("Amount must be positive.");
  }

  std::string from_bal_str = contract_manager_->GetBalance(from);
  std::string to_bal_str = contract_manager_->GetBalance(to);
  uint256_t from_bal = 0;
  uint256_t to_bal = 0;
  try {
    if (!from_bal_str.empty()) {
      from_bal = eevm::to_uint256(from_bal_str);
    }
    if (!to_bal_str.empty()) {
      to_bal = eevm::to_uint256(to_bal_str);
    }
  } catch (...) {
    return absl::InvalidArgumentError("Invalid persisted balance encoding.");
  }

  if (from_bal < amount) {
    return absl::InvalidArgumentError("Insufficient RoK balance.");
  }

  uint256_t new_from = from_bal - amount;
  uint256_t new_to = to_bal + amount;
  if (new_to < to_bal) {
    return absl::InvalidArgumentError("Transfer would overflow recipient balance.");
  }

  if (contract_manager_->SetBalance(from, new_from) != 0 ||
      contract_manager_->SetBalance(to, new_to) != 0) {
    return absl::InternalError("Failed to persist RoK transfer.");
  }
  return std::string("1");
}

}  // namespace contract
}  // namespace resdb

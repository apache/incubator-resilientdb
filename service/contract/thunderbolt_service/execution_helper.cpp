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
#include "service/contract/thunderbolt_service/execution_helper.h"

#include "common/utils/utils.h"
#include "glog/logging.h"

namespace resdb {
namespace contract {

using resdb::contract::Request;
using resdb::contract::x_manager::AddressManager;
using resdb::contract::x_manager::ContractManager;

absl::StatusOr<Account> ExecutionHelper::CreateAccount(
    AddressManager* address_manager) {
  std::string address =
      AddressManager::AddressToHex(address_manager->CreateRandomAddress());
  Account account;
  account.set_address(address);
  return account;
}

absl::StatusOr<Account> ExecutionHelper::CreateAccountWithAddress(
    const std::string& address, AddressManager* address_manager) {
  if (address.empty()) {
    return CreateAccount(address_manager);
  }
  address_manager->CreateAddress(eevm::to_uint256(address));
  Account account;
  account.set_address(address);
  LOG(ERROR) << " add account:" << address;
  return account;
}

bool ExecutionHelper::CreateAccount(const std::string& address,
                                    AddressManager* address_manager) {
  return address_manager->CreateAddress(eevm::to_uint256(address));
}

std::unique_ptr<ContractExecuteInfo> ExecutionHelper::GetContractInfo(
    const Request& request, AddressManager* address_manager) {
  Address caller_address =
      AddressManager::HexToAddress(request.caller_address());
  if (!address_manager->Exist(caller_address)) {
    LOG(ERROR) << "caller doesn't have an account:" << caller_address;
    // return absl::InvalidArgumentError("Account not exist.");
    assert(1 == 0);
  }
  // LOG(ERROR)<<"caller:"<<eevm::to_hex_string(caller_address)<<"
  // contract:"<<request.contract_address();

  return std::make_unique<ContractExecuteInfo>(
      caller_address, AddressManager::HexToAddress(request.contract_address()),
      "", request.func_params(), 0);
}

bool ExecutionHelper::DeployWithAddress(const resdb::contract::Request& request,
                                        AddressManager* address_manager,
                                        ContractManager* manager) {
  Address caller_address =
      AddressManager::HexToAddress(request.caller_address());
  if (!address_manager->Exist(caller_address)) {
    LOG(ERROR) << "caller doesn't have an account";
    return false;
  }

  Address contract_address = AddressManager::HexToAddress(
      request.resp().contract().contract_address());

  return manager->DeployContract(caller_address, request.deploy_info(),
                                 contract_address);
}

absl::StatusOr<Contract> ExecutionHelper::Deploy(
    const resdb::contract::Request& request, AddressManager* address_manager,
    ContractManager* contract_manager) {
  Address caller_address =
      AddressManager::HexToAddress(request.caller_address());
  if (!address_manager->Exist(caller_address)) {
    LOG(ERROR) << "caller doesn't have an account";
    return absl::InvalidArgumentError("Account not exist.");
  }

  Address contract_address;
  if (request.contract_address().empty()) {
    contract_address =
        contract_manager->DeployContract(caller_address, request.deploy_info());
  } else {
    contract_address = AddressManager::HexToAddress(request.contract_address());
    LOG(ERROR) << "deploy contract:" << caller_address
               << " contract:" << contract_address;
    if (!contract_manager->DeployContract(caller_address, request.deploy_info(),
                                          contract_address)) {
      LOG(ERROR) << "deploy contract fail";
      return absl::InvalidArgumentError("Contract invalid.");
    }
  }

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

std::unique_ptr<ModifyMap> ExecutionHelper::GetRWSList(
    const ResultInfo& request) {
  std::unique_ptr<ModifyMap> rws_list = std::make_unique<ModifyMap>();
  for (const auto& rws : request.rws()) {
    uint8_t tmp[32] = {};
    memcpy(tmp, rws.address().c_str(), 32);
    Address address = intx::le::load<uint256_t>(tmp);

    memcpy(tmp, rws.value().c_str(), 32);
    uint256_t v = intx::le::load<uint256_t>(tmp);
    // Address v = eevm::to_uint256(rws.value());
    // LOG(ERROR)<<"get address:"<<address<<" value:"<<v<<"
    // version:"<<rws.version()<<" type:"<<rws.type();

    (*rws_list)[address].push_back(
        Data(rws.type() == RWS::READ ? LOAD : STORE, v, rws.version()));
  }
  // LOG(ERROR)<<" get wrs list size:"<<rws_list->size();
  return rws_list;
}

}  // namespace contract
}  // namespace resdb

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

#include "interface/contract/contract_client.h"

#include <glog/logging.h>

#include <fstream>
#include <nlohmann/json.hpp>

#include "proto/contract/rpc.pb.h"

namespace resdb {
namespace contract {

ContractClient::ContractClient(const ResDBConfig& config)
    : TransactionConstructor(config) {}

absl::StatusOr<Account> ContractClient::CreateAccount() {
  Request request;
  Response response;
  request.set_cmd(Request::CREATE_ACCOUNT);
  int ret = SendRequest(request, &response);
  if (ret != 0 || response.ret() != 0) {
    return absl::InternalError("Account not exist.");
  }
  return response.account();
}

absl::Status ContractClient::AddExternalAddress(
    const std::string& external_address) {
  Request request;
  Response response;
  request.set_cmd(Request::ADD_ADDRESS);
  request.set_external_address(external_address);
  int ret = SendRequest(request, &response);
  if (ret != 0 || response.ret() != 0) {
    return absl::InternalError("Add address failed.");
  }
  return absl::OkStatus();
}

absl::StatusOr<Contract> ContractClient::DeployContract(
    const std::string& caller_address, const std::string& contract_name,
    const std::string& contract_path,
    const std::vector<std::string>& init_params) {
  std::ifstream contract_fstream(contract_path);
  if (!contract_fstream) {
    LOG(ERROR) << "could not find contract definition. file:" << contract_path
               << " name:" << contract_name;
    return absl::InvalidArgumentError("Contract not exist.");
  }

  nlohmann::json contracts_definition = nlohmann::json::parse(contract_fstream);

  const auto all_contracts = contracts_definition["contracts"];

  const std::string contract_code = all_contracts[contract_name]["bin"];
  if (contract_code.empty()) {
    LOG(ERROR) << "could not find contract definition. file:" << contract_path
               << " name:" << contract_name;
    return absl::InvalidArgumentError("Contract not exist.");
  }

  const auto func_hashes = all_contracts[contract_name]["hashes"];

  DeployInfo deploy_info;
  deploy_info.set_contract_bin(contract_code);
  deploy_info.set_contract_name(contract_name);

  for (auto& func : func_hashes.items()) {
    FuncInfo* new_func = deploy_info.add_func_info();
    new_func->set_func_name(func.key());
    new_func->set_hash(func.value());
  }

  for (const std::string& param : init_params) {
    deploy_info.add_init_param(param);
  }

  Request request;
  Response response;
  request.set_caller_address(caller_address);
  *request.mutable_deploy_info() = deploy_info;

  request.set_cmd(Request::DEPLOY);
  LOG(ERROR) << "send request:" << request.DebugString();
  int ret = SendRequest(request, &response);
  if (ret != 0 || response.ret() != 0) {
    return absl::InternalError("Deploy contract fail.");
  }
  return response.contract();
}

absl::StatusOr<std::string> ContractClient::ExecuteContract(
    const std::string& caller_address, const std::string& contract_address,
    const std::string& func_name, const std::vector<std::string>& func_params) {
  Request request;
  Response response;
  request.set_caller_address(caller_address);
  request.set_contract_address(contract_address);

  request.mutable_func_params()->set_func_name(func_name);
  for (const std::string& param : func_params) {
    request.mutable_func_params()->add_param(param);
  }

  request.set_cmd(Request::EXECUTE);
  LOG(ERROR) << "send request:" << request.DebugString();
  int ret = SendRequest(request, &response);
  if (ret != 0 || response.ret() != 0) {
    return absl::InternalError("Deploy contract fail.");
  }
  return response.res();
}

}  // namespace contract
}  // namespace resdb

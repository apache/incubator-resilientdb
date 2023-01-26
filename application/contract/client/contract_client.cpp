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

#include "application/contract/client/contract_client.h"

#include <glog/logging.h>

#include <fstream>
#include <nlohmann/json.hpp>

#include "application/contract/proto/rpc.pb.h"

namespace resdb {
namespace contract {

ContractClient::ContractClient(const ResDBConfig& config)
    : ResDBUserClient(config) {}

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

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

#include "service/contract/benchmark/ycsb/db/contract_executor.h"

#include <fmt/core.h>
#include <glog/logging.h>

#include <fstream>
#include <future>
#include <nlohmann/json.hpp>

#include "common/utils/utils.h"
#include "service/contract/executor/common/utils.h"
#include "service/contract/proto/func_params.pb.h"

namespace resdb {
namespace contract {
namespace benchmark {

ContractExecutor ::ContractExecutor(ExecutorType type, bool use_leveldb,
                                    int worker_num)
    : type_(type), use_leveldb_(use_leveldb), worker_num_(worker_num) {
  LOG(INFO) << "user leveldb:" << use_leveldb << " worker:" << worker_num;
}

void ContractExecutor::Init() {
  executor_ = GetExecutor();
  InitContract(executor_.get());
}

std::string ContractExecutor::GetContractPath() {
  const std::string data_dir =
      std::string(getenv("PWD")) + "/" + "service/contract/benchmark/";
  return data_dir + "data/ycsb.json";
}

std::string ContractExecutor::GetContractDef() { return "contracts"; }

std::string ContractExecutor::GetContractName() { return "ycsb.sol:ycsb"; }

Address ContractExecutor::GetAccountAddress() {
  assert(account_list_.size() > 0);
  return account_list_[0];
}

Address ContractExecutor::GetContractAddress() {
  assert(contract_list_.size() > 0);
  return contract_list_[0];
}

void ContractExecutor::InitContract(BenchmarkExecutor* executor) {
  std::string contract_path = GetContractPath();

  std::ifstream contract_fstream(contract_path, std::ifstream::in);
  if (!contract_fstream) {
    throw std::runtime_error(fmt::format(
        "Unable to open contract definition file {}", contract_path));
  }

  nlohmann::json definition = nlohmann::json::parse(contract_fstream);
  nlohmann::json contracts_json_ = definition[GetContractDef()];
  std::string contract_name = GetContractName();
  std::string contract_code = contracts_json_[contract_name]["bin"];
  nlohmann::json func_hashes = contracts_json_[contract_name]["hashes"];

  // deploy
  DeployInfo deploy_info;
  deploy_info.set_contract_bin(contract_code);
  deploy_info.set_contract_name(contract_name);

  for (auto& func : func_hashes.items()) {
    FuncInfo* new_func = deploy_info.add_func_info();
    new_func->set_func_name(func.key());
    new_func->set_hash(func.value());
    LOG(ERROR) << "func:" << func.key();
  }

  {
    Address account = executor->CreateAccount();
    account_list_.push_back(account);
    Address contract_addr = executor->Deploy(account, deploy_info);
    contract_list_.push_back(contract_addr);
  }
  LOG(ERROR) << "deploy done";
}

std::vector<std::unique_ptr<ExecuteResp>> ContractExecutor::Execute(
    std::vector<ContractExecuteInfo>& info) {
  return executor_->ExecuteContract(info);
}

}  // namespace benchmark
}  // namespace contract
}  // namespace resdb

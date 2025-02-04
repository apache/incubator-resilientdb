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

#include <fmt/core.h>

#include <fstream>
#include <future>

#include "common/utils/utils.h"
#include "glog/logging.h"
#include "service/contract/executor/x_manager/address_manager.h"
#include "service/contract/executor/x_manager/contract_manager.h"

namespace resdb {
namespace contract {
namespace x_manager {

class Executor {
 public:
  Executor(int worker_num = 2) {
    auto tmp_storage = std::make_unique<DataStorage>();
    storage_ = tmp_storage.get();
    manager_ = std::make_unique<ContractManager>(
        std::move(tmp_storage), worker_num, ContractManager::None);
  }

  Address CreateAccount() { return AddressManager().CreateRandomAddress(); }

  Address Deploy(const Address& address, DeployInfo deploy_info) {
    Address contract_address = manager_->DeployContract(address, deploy_info);
    return contract_address;
  }

  absl::StatusOr<std::string> ExecuteOne(const Address& caller_address,
                                         const Address& contract_address,
                                         const Params& params) {
    return manager_->ExecContract(caller_address, contract_address, params);
  }

  std::vector<std::unique_ptr<ExecuteResp>> ExecuteMulti(
      std::vector<ContractExecuteInfo>& info) {
    return manager_->ExecContract(info);
  }

  uint256_t Get(uint256_t key) { return storage_->Load(key).first; }

 protected:
  DataStorage* storage_;
  std::unique_ptr<ContractManager> manager_;
};

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb

using namespace resdb;
using namespace resdb::contract;
using namespace resdb::contract::x_manager;

std::vector<Address> account_list, contract_list;
std::vector<Address> user_list;

void Init(int user_num, int contract_num, Executor* executor) {
  contract_list.clear();
  account_list.clear();
  user_list.clear();

  const std::string data_dir =
      std::string(getenv("PWD")) + "/" + "service/contract/benchmark/";
  std::string contract_path = data_dir + "data/tpcc.json";

  std::ifstream contract_fstream(contract_path, std::ifstream::in);
  if (!contract_fstream) {
    throw std::runtime_error(fmt::format(
        "Unable to open contract definition file {}", contract_path));
  }

  nlohmann::json definition = nlohmann::json::parse(contract_fstream);
  nlohmann::json contracts_json_ = definition["contracts"];
  std::string contract_name = "tpcc.sol:TPCC";
  std::string contract_code = contracts_json_[contract_name]["bin"];
  nlohmann::json func_hashes = contracts_json_[contract_name]["hashes"];

  // deploy
  DeployInfo deploy_info;
  deploy_info.set_contract_bin(contract_code);
  deploy_info.set_contract_name(contract_name);

  for (auto& func : func_hashes.items()) {
    FuncInfo* new_func = deploy_info.add_func_info();
    LOG(ERROR) << " deploy func:" << func.key();
    new_func->set_func_name(func.key());
    new_func->set_hash(func.value());
  }

  {
    for (int i = 0; i < user_num; ++i) {
      Address account = executor->CreateAccount();
      user_list.push_back(account);
    }
  }
  {
    for (int i = 0; i < contract_num; ++i) {
      Address account = executor->CreateAccount();
      account_list.push_back(account);

      Address contract_addr = executor->Deploy(account, deploy_info);
      // LOG(ERROR)<<"contract address:"<<contract_addr;
      contract_list.push_back(contract_addr);
    }
  }
}

Params GetFuncParams() {
  Params func_params;
  func_params.set_func_name("payment(address,address,address,uint256)");
  std::string addr = AddressManager::AddressToHex(user_list[0]);
  func_params.add_param(addr);
  func_params.add_param(addr);
  func_params.add_param(addr);
  func_params.add_param(std::to_string(rand() % 1000));
  return func_params;
}

void test() {
  resdb::contract::x_manager::Executor pexecutor(1);
  Init(/*user_num=*/1, /*contract_num=*/1, &pexecutor);

  Params func_params = GetFuncParams();
  auto ret =
      pexecutor.ExecuteOne(account_list[0], contract_list[0], func_params);
  LOG(ERROR) << " execute ret:" << ret.ok();
  assert(ret.ok());
}

int main() {
  srand(time(0));

  test();
}

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

#include "service/contract/executor/manager/contract_deployer.h"

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <fstream>

#include "service/contract/executor/manager/address_manager.h"
#include "service/contract/executor/manager/test_committer.h"

namespace resdb {
namespace contract {
namespace {

using ::testing::Test;

const std::string test_dir = std::string(getenv("TEST_SRCDIR")) + "/" +
                             std::string(getenv("TEST_WORKSPACE")) +
                             "/service/contract/executor/manager/";

Address get_random_address() { return AddressManager().CreateRandomAddress(); }

std::string U256ToString(uint256_t v) { return eevm::to_hex_string(v); }

class ContractDeployerTest : public Test {
 public:
  ContractDeployerTest() : owner_address_(get_random_address()) {

    storage_ = std::make_unique<DataStorage>();
    gs_ = std::make_unique<GlobalState>(storage_.get());
    execotor_ = std::make_unique<TestCommitter>(storage_.get(), gs_.get());



    LOG(ERROR)<<"owner:"<<owner_address_;
    std::string contract_path = test_dir + "test_data/contract.json";

    std::ifstream contract_fstream(contract_path);
    if (!contract_fstream) {
      throw std::runtime_error(fmt::format(
          "Unable to open contract definition file {}", contract_path));
    }

    const auto contracts_definition = nlohmann::json::parse(contract_fstream);
    const auto all_contracts = contracts_definition["contracts"];
    const auto contract_code = all_contracts["ERC20.sol:ERC20Token"];
    contract_json_ = contract_code;
  }

 protected:
  Address owner_address_;
  nlohmann::json contract_json_;

  std::unique_ptr<DataStorage> storage_;
  std::unique_ptr<GlobalState> gs_;
  std::unique_ptr<ContractCommitter>execotor_;
};

TEST_F(ContractDeployerTest, NoContract) {
  ContractDeployer deployer(execotor_.get(), gs_.get());
  auto account = deployer.GetContract(1234);
  EXPECT_FALSE(account.ok());
}

TEST_F(ContractDeployerTest, DeployContract) {
  ContractDeployer deployer(execotor_.get(), gs_.get());

  DeployInfo deploy_info;
  deploy_info.set_contract_bin(contract_json_["bin"]);
  for (auto& func : contract_json_["hashes"].items()) {
    FuncInfo* new_func = deploy_info.add_func_info();
    new_func->set_func_name(func.key());
    new_func->set_hash(func.value());
  }

  deploy_info.add_init_param(U256ToString(1000));

  Address contract_address =
      deployer.DeployContract(owner_address_, deploy_info);
  EXPECT_GT(contract_address, 0);
  auto account = deployer.GetContract(contract_address);
  EXPECT_TRUE(account.ok());
}

TEST_F(ContractDeployerTest, DeployContractFromJson) {
  ContractDeployer deployer(execotor_.get(), gs_.get());

  Address contract_address =
      deployer.DeployContract(owner_address_, contract_json_, {1000});
  EXPECT_GT(contract_address, 0);
  auto account = deployer.GetContract(contract_address);
  EXPECT_TRUE(account.ok());
}



}  // namespace
}  // namespace contract
}  // namespace resdb

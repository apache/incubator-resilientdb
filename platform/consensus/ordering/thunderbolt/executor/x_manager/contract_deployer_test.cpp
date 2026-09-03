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
#include "platform/consensus/ordering/thunderbolt/executor/x_manager/contract_deployer.h"

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <fstream>

#include "platform/consensus/ordering/thunderbolt/executor/x_manager/address_manager.h"
#include "platform/consensus/ordering/thunderbolt/executor/x_manager/test_committer.h"

namespace resdb {
namespace contract {
using namespace x_manager;
namespace {

using ::testing::Test;

const std::string test_dir = std::string(getenv("TEST_SRCDIR")) + "/" +
                             std::string(getenv("TEST_WORKSPACE")) +
                             "/platform/consensus/ordering/thunderbolt/executor/manager/";

Address get_random_address() { return AddressManager().CreateRandomAddress(); }

std::string U256ToString(uint256_t v) { return eevm::to_hex_string(v); }

class ContractDeployerTest : public Test {
 public:
  ContractDeployerTest() : owner_address_(get_random_address()) {
    storage_ = std::make_unique<DataStorage>();
    gs_ = std::make_unique<GlobalState>(storage_.get());
    execotor_ = std::make_unique<TestCommitter>(storage_.get(), gs_.get());

    LOG(ERROR) << "owner:" << owner_address_;
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
  std::unique_ptr<ContractCommitter> execotor_;
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

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

#include "executor/contract/manager/contract_manager.h"

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <fstream>

#include "executor/contract/manager/address_manager.h"

namespace resdb {
namespace contract {
namespace {

using ::testing::Test;

const std::string test_dir = std::string(getenv("TEST_SRCDIR")) + "/" +
                             std::string(getenv("TEST_WORKSPACE")) +
                             "/executor/contract/manager/";

Address get_random_address() { return AddressManager().CreateRandomAddress(); }

std::string U256ToString(uint256_t v) { return eevm::to_hex_string(v); }
uint256_t HexToInt(const std::string& v) { return eevm::to_uint256(v); }

class ContractManagerTest : public Test {
 public:
  ContractManagerTest() : owner_address_(get_random_address()) {
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
};

TEST_F(ContractManagerTest, NoContract) {
  ContractManager manager;
  auto account = manager.GetContract(1234);
  EXPECT_FALSE(account.ok());
}

TEST_F(ContractManagerTest, DeployContract) {
  ContractManager manager;

  DeployInfo deploy_info;
  deploy_info.set_contract_bin(contract_json_["bin"]);
  for (auto& func : contract_json_["hashes"].items()) {
    FuncInfo* new_func = deploy_info.add_func_info();
    new_func->set_func_name(func.key());
    new_func->set_hash(func.value());
  }

  deploy_info.add_init_param(U256ToString(1000));

  Address contract_address =
      manager.DeployContract(owner_address_, deploy_info);
  EXPECT_GT(contract_address, 0);
  auto account = manager.GetContract(contract_address);
  EXPECT_TRUE(account.ok());
}

TEST_F(ContractManagerTest, InitContract) {
  ContractManager manager;

  DeployInfo deploy_info;
  deploy_info.set_contract_bin(contract_json_["bin"]);
  for (auto& func : contract_json_["hashes"].items()) {
    FuncInfo* new_func = deploy_info.add_func_info();
    new_func->set_func_name(func.key());
    new_func->set_hash(func.value());
  }

  deploy_info.add_init_param(U256ToString(1000));

  Address contract_address =
      manager.DeployContract(owner_address_, deploy_info);
  EXPECT_GT(contract_address, 0);
  auto account = manager.GetContract(contract_address);
  EXPECT_TRUE(account.ok());

  Address caller = get_random_address();
  Params func_params;
  func_params.set_func_name("totalSupply()");
  auto result = manager.ExecContract(caller, contract_address, func_params);
  EXPECT_EQ(HexToInt(*result), 1000);
}

TEST_F(ContractManagerTest, ExecContract) {
  ContractManager manager;

  DeployInfo deploy_info;
  deploy_info.set_contract_bin(contract_json_["bin"]);
  for (auto& func : contract_json_["hashes"].items()) {
    FuncInfo* new_func = deploy_info.add_func_info();
    new_func->set_func_name(func.key());
    new_func->set_hash(func.value());
  }

  deploy_info.add_init_param(U256ToString(1000));

  Address contract_address =
      manager.DeployContract(owner_address_, deploy_info);
  EXPECT_GT(contract_address, 0);
  auto account = manager.GetContract(contract_address);
  EXPECT_TRUE(account.ok());

  Address transfer_receiver = get_random_address();
  Address transfer_receiver2 = get_random_address();

  // owner 1000
  {
    Params func_params;
    func_params.set_func_name("balanceOf(address)");
    func_params.add_param(U256ToString(owner_address_));

    auto result =
        manager.ExecContract(owner_address_, contract_address, func_params);
    EXPECT_EQ(HexToInt(*result), 1000);
  }

  // receiver 0
  {
    Params func_params;
    func_params.set_func_name("balanceOf(address)");
    func_params.add_param(U256ToString(transfer_receiver));

    auto result =
        manager.ExecContract(owner_address_, contract_address, func_params);
    EXPECT_EQ(HexToInt(*result), 0);
  }

  // transfer 400 to receiver
  {
    Params func_params;
    func_params.set_func_name("transfer(address,uint256)");
    func_params.add_param(U256ToString(transfer_receiver));
    func_params.add_param(U256ToString(400));

    auto result =
        manager.ExecContract(owner_address_, contract_address, func_params);
    EXPECT_EQ(HexToInt(*result), 1);
  }

  // receiver 400
  {
    Address caller = get_random_address();
    Params func_params;
    func_params.set_func_name("balanceOf(address)");
    func_params.add_param(U256ToString(transfer_receiver));

    auto result = manager.ExecContract(caller, contract_address, func_params);
    EXPECT_EQ(HexToInt(*result), 400);
  }
  // owner 600
  {
    Address caller = get_random_address();
    Params func_params;
    func_params.set_func_name("balanceOf(address)");
    func_params.add_param(U256ToString(owner_address_));

    auto result = manager.ExecContract(caller, contract_address, func_params);
    EXPECT_EQ(HexToInt(*result), 600);
  }

  // transfer 200 to receiver2 from receiver
  {
    Params func_params;
    func_params.set_func_name("approve(address,uint256)");
    func_params.add_param(U256ToString(transfer_receiver));
    func_params.add_param(U256ToString(200));

    auto result =
        manager.ExecContract(transfer_receiver, contract_address, func_params);
    EXPECT_EQ(HexToInt(*result), 1);
  }
  {
    Params func_params;
    func_params.set_func_name("transferFrom(address,address,uint256)");
    func_params.add_param(U256ToString(transfer_receiver));
    func_params.add_param(U256ToString(transfer_receiver2));
    func_params.add_param(U256ToString(200));

    auto result =
        manager.ExecContract(transfer_receiver, contract_address, func_params);
    EXPECT_EQ(HexToInt(*result), 1);
  }
}

TEST_F(ContractManagerTest, NoFunc) {
  ContractManager manager;

  DeployInfo deploy_info;
  deploy_info.set_contract_bin(contract_json_["bin"]);
  for (auto& func : contract_json_["hashes"].items()) {
    FuncInfo* new_func = deploy_info.add_func_info();
    new_func->set_func_name(func.key());
    new_func->set_hash(func.value());
  }

  deploy_info.add_init_param(U256ToString(1000));

  Address contract_address =
      manager.DeployContract(owner_address_, deploy_info);
  EXPECT_GT(contract_address, 0);
  auto account = manager.GetContract(contract_address);
  EXPECT_TRUE(account.ok());

  // owner 1000
  {
    Params func_params;
    func_params.set_func_name("balanceOf()");
    func_params.add_param(U256ToString(owner_address_));

    auto result =
        manager.ExecContract(owner_address_, contract_address, func_params);
    EXPECT_FALSE(result.ok());
  }
}

}  // namespace
}  // namespace contract
}  // namespace resdb

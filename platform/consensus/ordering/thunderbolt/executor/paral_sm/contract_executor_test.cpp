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
#include <glog/logging.h>
#include <gtest/gtest.h>

#include <fstream>

#include "platform/consensus/ordering/thunderbolt/executor/paral_sm/address_manager.h"
#include "platform/consensus/ordering/thunderbolt/executor/paral_sm/contract_manager.h"
#include "platform/consensus/ordering/thunderbolt/executor/paral_sm/multi_contract_executor.h"

namespace resdb {
namespace contract {
namespace {

using ::testing::Test;

const std::string test_dir = std::string(getenv("TEST_SRCDIR")) + "/" +
                             std::string(getenv("TEST_WORKSPACE")) +
                             "/platform/consensus/ordering/thunderbolt/executor/paral_sm/";

Address get_random_address() { return AddressManager().CreateRandomAddress(); }

std::string U256ToString(uint256_t v) { return eevm::to_hex_string(v); }
uint256_t HexToInt(const std::string& v) { return eevm::to_uint256(v); }

uint256_t GetAddressHash(uint256_t address) {
  std::vector<uint8_t> code;
  code.resize(64u);
  eevm::to_big_endian(address, code.data());
  code[63] = 1;

  uint8_t h[32];
  eevm::keccak_256(code.data(), static_cast<unsigned int>(code.size()), h);
  return eevm::from_big_endian(h, sizeof(h));
}

class MultiContractExecutorTest : public Test {
 public:
  MultiContractExecutorTest() : owner_address_(get_random_address()) {
    std::string contract_path = test_dir + "test_data/contract.json";
    LOG(ERROR) << "test dir:" << test_dir;

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

TEST_F(MultiContractExecutorTest, ExecContract) {
  std::unique_ptr<DataStorage> storage = std::make_unique<DataStorage>();
  DataStorage* storage_ptr = storage.get();
  ContractManager manager(std::move(storage));

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
  LOG(ERROR) << "owner address:" << owner_address_;
  LOG(ERROR) << "receive 1:" << transfer_receiver;
  LOG(ERROR) << "receive 2:" << transfer_receiver2;

  uint256_t owner_key = GetAddressHash(owner_address_);
  uint256_t transfer_key = GetAddressHash(transfer_receiver);
  uint256_t transfer2_key = GetAddressHash(transfer_receiver2);

  // owner 1000
  {
    Params func_params;
    func_params.set_func_name("balanceOf(address)");
    func_params.add_param(U256ToString(owner_address_));

    auto result =
        manager.ExecContract(owner_address_, contract_address, func_params);
    LOG(ERROR) << "result:" << *result;
    EXPECT_EQ(HexToInt(*result), 1000);

    EXPECT_EQ(storage_ptr->Load(owner_key).first, 1000);
  }

  /*
    // receiver 0
    {
      Params func_params;
      func_params.set_func_name("balanceOf(address)");
      func_params.add_param(U256ToString(transfer_receiver));

      auto result =
          manager.ExecContract(owner_address_, contract_address, func_params);
      EXPECT_EQ(HexToInt(*result), 0);

      EXPECT_EQ(storage_ptr->Load(transfer_key).first, 0);
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

      EXPECT_EQ(storage_ptr->Load(transfer_key).first, 400);
      EXPECT_EQ(storage_ptr->Load(owner_key).first, 600);
    }

    // owner 600
    {
      Address caller = get_random_address();
      Params func_params;
      func_params.set_func_name("balanceOf(address)");
      func_params.add_param(U256ToString(owner_address_));

      auto result = manager.ExecContract(caller, contract_address, func_params);
      EXPECT_EQ(HexToInt(*result), 600);

      EXPECT_EQ(storage_ptr->Load(transfer_key).first, 400);
      EXPECT_EQ(storage_ptr->Load(owner_key).first, 600);
    }

    // transfer 200 to receiver2 from receiver
    {
      Params func_params;
      func_params.set_func_name("approve(address,uint256)");
      func_params.add_param(U256ToString(transfer_receiver));
      func_params.add_param(U256ToString(100));

      auto result =
          manager.ExecContract(transfer_receiver, contract_address,
    func_params); EXPECT_EQ(HexToInt(*result), 1);

      EXPECT_EQ(storage_ptr->Load(transfer2_key).first, 0);
      EXPECT_EQ(storage_ptr->Load(transfer_key).first, 400);
      EXPECT_EQ(storage_ptr->Load(owner_key).first, 600);
    }
    {
      Params func_params;
      func_params.set_func_name("transferFrom(address,address,uint256)");
      func_params.add_param(U256ToString(transfer_receiver));
      func_params.add_param(U256ToString(transfer_receiver2));
      func_params.add_param(U256ToString(100));

      auto result =
          manager.ExecContract(transfer_receiver, contract_address,
    func_params); EXPECT_EQ(HexToInt(*result), 1);

      EXPECT_EQ(storage_ptr->Load(transfer2_key).first, 100);
      EXPECT_EQ(storage_ptr->Load(transfer_key).first, 300);
      EXPECT_EQ(storage_ptr->Load(owner_key).first, 600);
    }
    */
}
/*

TEST_F(MultiContractExecutorTest, ExecMultiContractNoConflict) {

  std::unique_ptr<DataStorage> storage = std::make_unique<DataStorage>();
  DataStorage * storage_ptr = storage.get();
  ContractManager manager(std::move(storage));

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
  Address transfer_receiver3 = get_random_address();

  uint256_t owner_key = GetAddressHash(owner_address_);
  uint256_t transfer_key = GetAddressHash(transfer_receiver);
  uint256_t transfer2_key = GetAddressHash(transfer_receiver2);
  uint256_t transfer3_key = GetAddressHash(transfer_receiver3);

  {
    Params func_params;
    func_params.set_func_name("transfer(address,uint256)");
    func_params.add_param(U256ToString(transfer_receiver));
    func_params.add_param(U256ToString(400));

    auto result =
        manager.ExecContract(owner_address_, contract_address, func_params);
    EXPECT_EQ(HexToInt(*result), 1);

    EXPECT_EQ(storage_ptr->Load(transfer_key).first, 400);
    EXPECT_EQ(storage_ptr->Load(owner_key).first, 600);
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

  std::vector<ContractExecuteInfo> info;
  {
    Params func_params;
    func_params.set_func_name("transfer(address,uint256)");
    func_params.add_param(U256ToString(transfer_receiver3));
    func_params.add_param(U256ToString(100));

    info.push_back(ContractExecuteInfo(owner_address_, contract_address, "",
func_params, 0));
  }

  {
    Params func_params;
    func_params.set_func_name("transferFrom(address,address,uint256)");
    func_params.add_param(U256ToString(transfer_receiver));
    func_params.add_param(U256ToString(transfer_receiver2));
    func_params.add_param(U256ToString(200));

    info.push_back(ContractExecuteInfo(transfer_receiver, contract_address, "",
func_params, 0));
  }
  std::vector<std::unique_ptr<ExecuteResp>> resp = manager.ExecContract(info);
  EXPECT_EQ(storage_ptr->Load(transfer2_key).first, 200);
  EXPECT_EQ(storage_ptr->Load(transfer3_key).first, 100);

  LOG(ERROR)<<"resp size:"<<resp.size();
  EXPECT_EQ(resp.size(), 2);
  EXPECT_EQ(resp[0]->ret, 0);
  EXPECT_EQ(resp[1]->ret, 0);

  EXPECT_EQ(HexToInt(resp[0]->result), 1);
  EXPECT_EQ(HexToInt(resp[1]->result), 1);
}

TEST_F(MultiContractExecutorTest, ExecMultiContractHaveConflict) {

  std::unique_ptr<DataStorage> storage = std::make_unique<DataStorage>();
  DataStorage * storage_ptr = storage.get();
  ContractManager manager(std::move(storage));

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
  Address transfer_receiver3 = get_random_address();

  uint256_t owner_key = GetAddressHash(owner_address_);
  uint256_t transfer_key = GetAddressHash(transfer_receiver);
  uint256_t transfer2_key = GetAddressHash(transfer_receiver2);
  uint256_t transfer3_key = GetAddressHash(transfer_receiver3);

  std::vector<ContractExecuteInfo> info;
  {
    Params func_params;
    func_params.set_func_name("transfer(address,uint256)");
    func_params.add_param(U256ToString(transfer_receiver));
    func_params.add_param(U256ToString(100));

    info.push_back(ContractExecuteInfo(owner_address_, contract_address, "",
func_params, 0));
  }
  {
    Params func_params;
    func_params.set_func_name("transfer(address,uint256)");
    func_params.add_param(U256ToString(transfer_receiver2));
    func_params.add_param(U256ToString(100));

    info.push_back(ContractExecuteInfo(owner_address_, contract_address, "",
func_params, 0));
  }

  std::vector<std::unique_ptr<ExecuteResp>> resp = manager.ExecContract(info);
  EXPECT_EQ(storage_ptr->Load(owner_key).first, 800);
  EXPECT_EQ(storage_ptr->Load(transfer_key).first, 100);
  EXPECT_EQ(storage_ptr->Load(transfer2_key).first, 100);

  LOG(ERROR)<<"resp size:"<<resp.size();
  EXPECT_EQ(resp.size(), 2);
  EXPECT_EQ(resp[0]->ret, 0);
  EXPECT_EQ(resp[1]->ret, 0);

  EXPECT_EQ(HexToInt(resp[0]->result), 1);
  EXPECT_EQ(HexToInt(resp[1]->result), 1);
}
*/

}  // namespace
}  // namespace contract
}  // namespace resdb

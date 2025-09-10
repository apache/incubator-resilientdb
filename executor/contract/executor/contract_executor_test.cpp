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
#include <gtest/gtest.h>

#include <fstream>

namespace resdb {
namespace contract {
namespace {

using ::testing::Test;

const std::string test_dir = std::string(getenv("TEST_SRCDIR")) + "/" +
                             std::string(getenv("TEST_WORKSPACE")) +
                             "/service/contract/executor/service/";

std::string ToString(const Request& request) {
  std::string ret;
  request.SerializeToString(&ret);
  return ret;
}

class ContractTransactionManagerTest : public Test {
 public:
  ContractTransactionManagerTest() {
    std::string contract_path = test_dir + "test_data/contract.json";

    std::ifstream contract_fstream(contract_path);
    if (!contract_fstream) {
      throw std::runtime_error(fmt::format(
          "Unable to open contract definition file {}", contract_path));
    }

    nlohmann::json definition = nlohmann::json::parse(contract_fstream);
    contracts_json_ = definition["contracts"];
  }

  Account CreateAccount() {
    Request request;
    Response response;

    request.set_cmd(Request::CREATE_ACCOUNT);
    std::unique_ptr<std::string> ret = executor_.ExecuteData(ToString(request));
    EXPECT_TRUE(ret != nullptr);
    response.ParseFromString(*ret);
    return response.account();
  }

  absl::StatusOr<Contract> Deploy(const Account& account,
                                  DeployInfo deploy_info) {
    Request request;
    Response response;

    request.set_caller_address(account.address());
    *request.mutable_deploy_info() = deploy_info;
    request.set_cmd(Request::DEPLOY);

    std::unique_ptr<std::string> ret = executor_.ExecuteData(ToString(request));
    EXPECT_TRUE(ret != nullptr);

    response.ParseFromString(*ret);
    if (response.ret() == 0) {
      return response.contract();
    } else {
      return absl::InternalError("DeployFail.");
    }
  }

  absl::StatusOr<uint256_t> Execute(const std::string& caller_address,
                                    const std::string& contract_address,
                                    const Params& params) {
    Request request;
    Response response;

    request.set_caller_address(caller_address);
    request.set_contract_address(contract_address);
    request.set_cmd(Request::EXECUTE);
    *request.mutable_func_params() = params;

    std::unique_ptr<std::string> ret = executor_.ExecuteData(ToString(request));
    EXPECT_TRUE(ret != nullptr);

    response.ParseFromString(*ret);

    if (response.ret() == 0) {
      return eevm::to_uint256(response.res());
    } else {
      return absl::InternalError("DeployFail.");
    }
  }

 protected:
  nlohmann::json contracts_json_;
  ContractTransactionManager executor_;
};

TEST_F(ContractTransactionManagerTest, ExecContract) {
  // create an account.
  Account account = CreateAccount();
  EXPECT_FALSE(account.address().empty());

  std::string contract_name = "ERC20.sol:ERC20Token";
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
  }
  deploy_info.add_init_param("1000");

  absl::StatusOr<Contract> contract_or = Deploy(account, deploy_info);
  EXPECT_TRUE(contract_or.ok());
  Contract contract = *contract_or;

  // query owner should return 1000
  {
    Params func_params;
    func_params.set_func_name("balanceOf(address)");
    func_params.add_param(account.address());

    auto result =
        Execute(account.address(), contract.contract_address(), func_params);
    EXPECT_EQ(*result, 0x3e8);
  }

  Account transfer_receiver = CreateAccount();
  EXPECT_FALSE(account.address().empty());
  // receiver 0
  {
    Params func_params;
    func_params.set_func_name("balanceOf(address)");
    func_params.add_param(transfer_receiver.address());

    auto result =
        Execute(account.address(), contract.contract_address(), func_params);
    EXPECT_EQ(*result, 0);
  }

  // transfer 400 to receiver
  {
    Params func_params;
    func_params.set_func_name("transfer(address,uint256)");
    func_params.add_param(transfer_receiver.address());
    func_params.add_param("400");

    auto result =
        Execute(account.address(), contract.contract_address(), func_params);
    EXPECT_EQ(*result, 1);
  }

  // query owner should return 600
  {
    Params func_params;
    func_params.set_func_name("balanceOf(address)");
    func_params.add_param(account.address());

    auto result =
        Execute(account.address(), contract.contract_address(), func_params);
    EXPECT_EQ(*result, 600);
  }

  // receiver 400
  {
    Params func_params;
    func_params.set_func_name("balanceOf(address)");
    func_params.add_param(transfer_receiver.address());

    auto result =
        Execute(account.address(), contract.contract_address(), func_params);
    EXPECT_EQ(*result, 400);
  }
}

TEST_F(ContractTransactionManagerTest, DeployFail) {
  // create an account.
  Account account = CreateAccount();
  EXPECT_FALSE(account.address().empty());

  std::string contract_name = "ERC20.sol:ERC20Token";
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
  }
  // deploy_info.add_init_param("1000");

  absl::StatusOr<Contract> contract_or = Deploy(account, deploy_info);
  EXPECT_FALSE(contract_or.ok());
}

TEST_F(ContractTransactionManagerTest, NoFunc) {
  // create an account.
  Account account = CreateAccount();
  EXPECT_FALSE(account.address().empty());

  std::string contract_name = "ERC20.sol:ERC20Token";
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
  }
  deploy_info.add_init_param("1000");

  absl::StatusOr<Contract> contract_or = Deploy(account, deploy_info);
  EXPECT_TRUE(contract_or.ok());
  Contract contract = *contract_or;

  {
    Params func_params;
    func_params.set_func_name("balanceOf()");
    func_params.add_param(account.address());

    auto result =
        Execute(account.address(), contract.contract_address(), func_params);
    EXPECT_FALSE(result.ok());
  }
}

}  // namespace
}  // namespace contract
}  // namespace resdb

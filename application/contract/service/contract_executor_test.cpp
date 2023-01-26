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

#include "application/contract/service/contract_executor.h"

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <fstream>

namespace resdb {
namespace contract {
namespace {

using ::testing::Test;

const std::string test_dir = std::string(getenv("TEST_SRCDIR")) + "/" +
                             std::string(getenv("TEST_WORKSPACE")) +
                             "/application/contract/service/";

std::string ToString(const Request& request) {
  std::string ret;
  request.SerializeToString(&ret);
  return ret;
}

class ContractExecutorTest : public Test {
 public:
  ContractExecutorTest() {
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
  ContractExecutor executor_;
};

TEST_F(ContractExecutorTest, ExecContract) {
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

TEST_F(ContractExecutorTest, DeployFail) {
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

TEST_F(ContractExecutorTest, NoFunc) {
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

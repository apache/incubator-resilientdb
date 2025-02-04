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

#include "service/contract/executor/manager/x_verifier.h"

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <fstream>

#include "service/contract/executor/manager/address_manager.h"
#include "service/contract/executor/manager/contract_deployer.h"
#include "service/contract/executor/manager/global_state.h"
#include "service/contract/executor/manager/mock_data_storage.h"
#include "service/contract/executor/manager/x_committer.h"
#include "service/contract/proto/func_params.pb.h"

namespace resdb {
namespace contract {
namespace {

using ::testing::Invoke;
using ::testing::Test;

const std::string test_dir = std::string(getenv("TEST_SRCDIR")) + "/" +
                             std::string(getenv("TEST_WORKSPACE")) +
                             "/service/contract/executor/manager/";

Address get_random_address() { return AddressManager().CreateRandomAddress(); }

std::string U256ToString(uint256_t v) { return eevm::to_hex_string(v); }
uint256_t HexToInt(const std::string& v) { return eevm::to_uint256(v); }

class VVerifierTest : public Test {
 public:
  VVerifierTest() : owner_address_(get_random_address()) {
    std::string contract_path = test_dir + "test_data/kv.json";

    std::ifstream contract_fstream(contract_path);
    if (!contract_fstream) {
      throw std::runtime_error(fmt::format(
          "Unable to open contract definition file {}", contract_path));
    }

    const auto contracts_definition = nlohmann::json::parse(contract_fstream);
    const auto all_contracts = contracts_definition["contracts"];
    const auto contract_code = all_contracts["kv.sol:KV"];
    storage_ = std::make_unique<MockStorage>();
    v_storage_ = std::make_unique<DataStorage>();
    contract_json_ = contract_code;

    EXPECT_CALL(*storage_, Load)
        .WillRepeatedly(Invoke(
            [&](const uint256_t& address, bool) { return data_[address]; }));

    EXPECT_CALL(*storage_, Store)
        .WillRepeatedly(
            Invoke([&](const uint256_t& key, const uint256_t& value, bool) {
              int v = data_[key].second;
              LOG(ERROR) << "store:" << key << " v:" << v;
              ;
              data_[key] = std::make_pair(value, v + 1);
              return v + 1;
            }));

    EXPECT_CALL(*storage_, GetVersion)
        .WillRepeatedly(Invoke(
            [&](const uint256_t& key, bool) { return data_[key].second; }));

    Init();
  }

  void Init() {
    gs_ = std::make_unique<GlobalState>(storage_.get());

    committer_ = std::make_unique<XCommitter>(storage_.get(), gs_.get());

    contract_address_ = AddressManager::CreateContractAddress(owner_address_);
    deployer_ = std::make_unique<ContractDeployer>(committer_.get(), gs_.get());
    contract_address_ =
        deployer_->DeployContract(owner_address_, contract_json_, {1000});

    v_gs_ = std::make_unique<GlobalState>(v_storage_.get());

    verifier_ = std::make_unique<XVerifier>(v_storage_.get(), v_gs_.get());

    v_deployer_ =
        std::make_unique<ContractDeployer>(verifier_.get(), v_gs_.get());
    v_deployer_->DeployContract(owner_address_, contract_json_, {1000},
                                contract_address_);
  }

  std::vector<std::unique_ptr<ExecuteResp>> ExecContract(
      std::vector<ContractExecuteInfo>& execute_info) {
    for (int i = 0; i < execute_info.size(); ++i) {
      std::string func_addr =
          deployer_->GetFuncAddress(execute_info[i].contract_address,
                                    execute_info[i].func_params.func_name());
      if (func_addr.empty()) {
        LOG(ERROR) << "no fouction:" << execute_info[i].func_params.func_name();
        execute_info[i].contract_address = 0;
        continue;
      }
      execute_info[i].func_addr = func_addr;
      execute_info[i].commit_id = i + 1;
    }
    return committer_->ExecContract(execute_info);
  }

 protected:
  Address owner_address_;
  Address contract_address_;
  nlohmann::json contract_json_;
  std::unique_ptr<MockStorage> storage_;
  std::unique_ptr<DataStorage> v_storage_;
  std::unique_ptr<GlobalState> gs_, v_gs_;
  std::unique_ptr<XCommitter> committer_;
  std::unique_ptr<XVerifier> verifier_;
  std::map<uint256_t, std::pair<uint256_t, int>> data_;
  std::unique_ptr<ContractDeployer> deployer_, v_deployer_;
};

TEST_F(VVerifierTest, TwoTxnNoConflict) {
  Address transfer_receiver = get_random_address();
  LOG(ERROR) << "start";
  // owner 1000
  std::vector<ContractExecuteInfo> info;
  {
    Params func_params;
    func_params.set_func_name("set(address,uint256)");
    func_params.add_param(U256ToString(transfer_receiver));
    func_params.add_param(U256ToString(100));
    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "",
                                       func_params, 0));
  }

  {
    Params func_params;
    func_params.set_func_name("set(address,uint256)");
    func_params.add_param(U256ToString(transfer_receiver));
    func_params.add_param(U256ToString(200));

    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "",
                                       func_params, 0));
  }

  {
    std::vector<std::unique_ptr<ExecuteResp>> resp = ExecContract(info);
    EXPECT_EQ(resp.size(), 2);

    std::vector<ContractExecuteInfo> new_info;
    std::vector<ConcurrencyController ::ModifyMap> rws_list;
    for (int i = 0; i < resp.size(); ++i) {
      rws_list.push_back(resp[i]->rws);
      for (int j = 0; j < info.size(); ++j) {
        if (info[j].commit_id == resp[i]->commit_id) {
          new_info.push_back(info[j]);
        }
      }
    }

    EXPECT_TRUE(verifier_->VerifyContract(new_info, rws_list));
  }
  LOG(ERROR) << "done";
}

// a->b
TEST_F(VVerifierTest, TwoTxnConflict) {
  Address transfer_receiver = get_random_address();
  Address transfer_receiver2 = get_random_address();

  // owner 1000
  std::vector<ContractExecuteInfo> info;
  {
    Params func_params;
    func_params.set_func_name("set(address,uint256)");
    func_params.add_param(U256ToString(transfer_receiver));
    func_params.add_param(U256ToString(100));

    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "",
                                       func_params, 0));
  }
  {
    Params func_params;
    func_params.set_func_name("set(address,uint256)");
    func_params.add_param(U256ToString(transfer_receiver2));
    func_params.add_param(U256ToString(100));

    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "",
                                       func_params, 0));
  }

  {
    std::vector<std::unique_ptr<ExecuteResp>> resp = ExecContract(info);
    EXPECT_EQ(resp.size(), 2);

    std::vector<ContractExecuteInfo> new_info;
    std::vector<ConcurrencyController ::ModifyMap> rws_list;
    for (int i = 0; i < resp.size(); ++i) {
      rws_list.push_back(resp[i]->rws);
      for (int j = 0; j < info.size(); ++j) {
        if (info[j].commit_id == resp[i]->commit_id) {
          new_info.push_back(info[j]);
        }
      }
    }

    EXPECT_TRUE(verifier_->VerifyContract(new_info, rws_list));
  }
}

}  // namespace
}  // namespace contract
}  // namespace resdb

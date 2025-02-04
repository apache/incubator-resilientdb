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

#include "service/contract/executor/x_manager/e_committer.h"

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <fstream>

#include "service/contract/executor/x_manager/address_manager.h"
#include "service/contract/executor/x_manager/contract_deployer.h"
#include "service/contract/executor/x_manager/global_state.h"
#include "service/contract/executor/x_manager/mock_data_storage.h"
#include "service/contract/proto/func_params.pb.h"

namespace resdb {
namespace contract {
namespace x_manager {
namespace {

using ::testing::Invoke;
using ::testing::Test;

const std::string test_dir = std::string(getenv("TEST_SRCDIR")) + "/" +
                             std::string(getenv("TEST_WORKSPACE")) +
                             "/service/contract/executor/manager/";

Address get_random_address() { return AddressManager().CreateRandomAddress(); }

std::string U256ToString(uint256_t v) { return eevm::to_hex_string(v); }
uint256_t HexToInt(const std::string& v) { return eevm::to_uint256(v); }

class ECommitterTest : public Test {
 public:
  ECommitterTest() : owner_address_(get_random_address()) {
    std::string contract_path = test_dir + "test_data/contract.json";

    std::ifstream contract_fstream(contract_path);
    if (!contract_fstream) {
      throw std::runtime_error(fmt::format(
          "Unable to open contract definition file {}", contract_path));
    }

    const auto contracts_definition = nlohmann::json::parse(contract_fstream);
    const auto all_contracts = contracts_definition["contracts"];
    const auto contract_code = all_contracts["ERC20.sol:ERC20Token"];
    storage_ = std::make_unique<MockStorage>();
    contract_json_ = contract_code;

    EXPECT_CALL(*storage_, Load)
        .WillRepeatedly(Invoke([&](const uint256_t& address, bool) {
          LOG(ERROR) << "load:" << address << " value:" << data_[address].first;
          return data_[address];
        }));

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

    committer_ = std::make_unique<ECommitter>(storage_.get(), gs_.get(), 1024);

    contract_address_ = AddressManager::CreateContractAddress(owner_address_);
    deployer_ = std::make_unique<ContractDeployer>(committer_.get(), gs_.get());
    contract_address_ =
        deployer_->DeployContract(owner_address_, contract_json_, {1000});
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
  std::unique_ptr<GlobalState> gs_;
  std::unique_ptr<ECommitter> committer_;
  std::map<uint256_t, std::pair<uint256_t, int>> data_;
  std::unique_ptr<ContractDeployer> deployer_;
};

TEST_F(ECommitterTest, ExecContract) {
  // owner 1000
  std::vector<ContractExecuteInfo> info;
  {
    Params func_params;
    func_params.set_func_name("balanceOf(address)");
    func_params.add_param(U256ToString(owner_address_));
    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "",
                                       func_params, 0));
  }

  {
    std::vector<std::unique_ptr<ExecuteResp>> resp = ExecContract(info);
    EXPECT_EQ(resp.size(), 1);
    EXPECT_EQ(resp[0]->ret, 0);
  }
}

// get a
// a->b
TEST_F(ECommitterTest, TwoTxnNoConflict) {
  Address transfer_receiver = get_random_address();

  // owner 1000
  std::vector<ContractExecuteInfo> info;
  {
    Params func_params;
    func_params.set_func_name("balanceOf(address)");
    func_params.add_param(U256ToString(owner_address_));
    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "",
                                       func_params, 0));
  }
  {
    Params func_params;
    func_params.set_func_name("balanceOf(address)");
    func_params.add_param(U256ToString(transfer_receiver));
    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "",
                                       func_params, 0));
  }

  {
    std::vector<std::unique_ptr<ExecuteResp>> resp = ExecContract(info);
    EXPECT_EQ(resp.size(), 2);

    for (int i = 0; i < 2; ++i) {
      EXPECT_EQ(resp[i]->ret, 0);
      if (resp[i]->commit_id == 0) {
        EXPECT_EQ(HexToInt(resp[i]->result), 1000);
      } else {
        EXPECT_EQ(HexToInt(resp[i]->result), 0);
      }
    }
  }
}

TEST_F(ECommitterTest, TwoTxnConflict) {
  Address transfer_receiver = get_random_address();

  // owner 1000
  std::vector<ContractExecuteInfo> info;
  {
    Params func_params;
    func_params.set_func_name("balanceOf(address)");
    func_params.add_param(U256ToString(owner_address_));
    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "",
                                       func_params, 0));
  }

  {
    Params func_params;
    func_params.set_func_name("transfer(address,uint256)");
    func_params.add_param(U256ToString(transfer_receiver));
    func_params.add_param(U256ToString(100));

    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "",
                                       func_params, 0));
  }

  {
    std::vector<std::unique_ptr<ExecuteResp>> resp = ExecContract(info);
    EXPECT_EQ(resp.size(), 2);
    for (int i = 0; i < 2; ++i) {
      if (resp[i]->commit_id == 1) {
        EXPECT_EQ(resp[i]->ret, 0);
        EXPECT_EQ(HexToInt(resp[i]->result), 1);
      }
    }
  }
}

/*
// a->b
TEST_F(ECommitterTest, TwoTxnConflict1) {
  Address transfer_receiver = get_random_address();
  Address transfer_receiver2 = get_random_address();

  bool start = false;
  std::map<uint256_t, int> load_time;
  EXPECT_CALL(*storage_, Load).WillRepeatedly(Invoke([&](const uint256_t&
address) { if(start){ load_time[address]++;
      }
      return data_[address];
  }));

  EXPECT_CALL(*storage_, Store).WillRepeatedly(Invoke([&](const uint256_t& key,
const uint256_t& value,bool) { if(start) { bool done = false; while(!done){
          for(auto it : load_time){
            if(it.second>1){
              done = true;
              break;
            }
          }
        }
      }
      int v = data_[key].second;
      data_[key] = std::make_pair(value, v+1);
  }));

  Init();
  start = true;

  // owner 1000
  std::vector<ContractExecuteInfo> info;
  {
    Params func_params;
    func_params.set_func_name("transfer(address,uint256)");
    func_params.add_param(U256ToString(transfer_receiver));
    func_params.add_param(U256ToString(100));

    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "",
func_params, 0));
  }
  {
    Params func_params;
    func_params.set_func_name("transfer(address,uint256)");
    func_params.add_param(U256ToString(transfer_receiver2));
    func_params.add_param(U256ToString(100));

    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "",
func_params, 0));
  }

  {
    std::vector<std::unique_ptr<ExecuteResp>> resp = ExecContract(info);
    EXPECT_EQ(resp.size(), 2);
    EXPECT_EQ(resp[0]->ret, 0);
    EXPECT_EQ(HexToInt(resp[0]->result), 1);
    EXPECT_EQ(resp[1]->ret, 0);
    EXPECT_EQ(HexToInt(resp[1]->result), 1);
  }
  //EXPECT_EQ(committer_->GetExecutionState()->commit_time,2);
  //EXPECT_EQ(committer_->GetExecutionState()->redo_time,1);
}
*/

}  // namespace
}  // namespace x_manager
}  // namespace contract
}  // namespace resdb

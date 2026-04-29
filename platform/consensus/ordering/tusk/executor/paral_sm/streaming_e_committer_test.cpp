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

#include "service/contract/executor/x_manager/streaming_e_committer.h"

#include <fstream>

#include "service/contract/executor/x_manager/mock_d_storage.h"
#include "service/contract/executor/x_manager/mock_e_controller.h"
#include "service/contract/executor/x_manager/global_state.h"
#include "service/contract/executor/x_manager/contract_deployer.h"
#include "service/contract/executor/x_manager/address_manager.h"
#include "service/contract/proto/func_params.pb.h"

#include <glog/logging.h>
#include <gtest/gtest.h>


namespace resdb {
namespace contract {
namespace x_manager {
namespace {

using ::testing::Test;
using ::testing::Invoke;

const std::string test_dir = "/home/ubuntu/nexres//service/contract/executor/manager/";
//const std::string test_dir = std::string(getenv("TEST_SRCDIR")) + "/" +
 //                            std::string(getenv("TEST_WORKSPACE")) +
  //                           "/service/contract/executor/manager/";

Address get_random_address() { return AddressManager().CreateRandomAddress(); }

std::string U256ToString(uint256_t v) { return eevm::to_hex_string(v); }
uint256_t HexToInt(const std::string& v) { return eevm::to_uint256(v); }

class StreamingECommitterTest : public Test {
 public:
  StreamingECommitterTest() : owner_address_(get_random_address()) {
    std::string contract_path = test_dir + "test_data/kv.json";

    std::ifstream contract_fstream(contract_path);
    if (!contract_fstream) {
      throw std::runtime_error(fmt::format(
          "Unable to open contract definition file {}", contract_path));
    }

    const auto contracts_definition = nlohmann::json::parse(contract_fstream);
    const auto all_contracts = contracts_definition["contracts"];
    const auto contract_code = all_contracts["kv.sol:KV"];
    storage_ = std::make_unique<MockDStorage>();
    contract_json_ = contract_code;

    EXPECT_CALL(*storage_, Load).WillRepeatedly(Invoke([&](const uint256_t& address, bool is_local) {
        if(data_.find(address) == data_.end()){
            data_[address] = g_data_[address];
        }
        auto ret = is_local? data_[address]: g_data_[address];
        LOG(ERROR)<<"load:"<<address<<" is local:"<<is_local<<" version:"<<ret.second;
        return ret;
    }));

    EXPECT_CALL(*storage_, Store).WillRepeatedly(Invoke([&](const uint256_t& key, const uint256_t& value, bool is_local) {
      //LOG(ERROR)<<"store:"<<key<<" is local:"<<is_local;
      if(is_local){
          int v = data_[key].second;
          data_[key] = std::make_pair(value, v+1);
      }
      else {
          int v = g_data_[key].second;
          g_data_[key] = std::make_pair(value, v+1);
      }
        return 1;
    }));

    EXPECT_CALL(*storage_, StoreWithVersion).WillRepeatedly(Invoke([&](
            const uint256_t& key, const uint256_t& value, int version, bool is_local) {
        LOG(ERROR)<<"store:"<<key<<" is local:"<<is_local<<" version:"<<version<<" data:"<<value;
        if(is_local){
            data_[key] = std::make_pair(value, version);
        }
        else {
            g_data_[key] = std::make_pair(value, version);
        }
        return 1;
    }));

    EXPECT_CALL(*storage_, GetVersion).WillRepeatedly(Invoke([&](const uint256_t& key, bool is_local) {
        if(data_.find(key) == data_.end()){
            data_[key] = g_data_[key];
        }
        auto ret = is_local? data_[key].second: g_data_[key].second;
        //LOG(ERROR)<<"version :"<<key<<" is local:"<<is_local<<" ver:"<<ret;
        return ret;
    }));

  EXPECT_CALL(*storage_, Reset).WillRepeatedly(Invoke([&](const uint256_t& key, const uint256_t& value, int64_t version, bool is_local) {
      LOG(ERROR)<<"reset key:"<<key<<" value:"<<value<<" version:"<<version<<" is local:"<<is_local;
      if(is_local){
        data_[key] = std::make_pair(value, version);
      }
      else {
        g_data_[key] = std::make_pair(value, version);
      }
  }));


    Init();
  }

  void Init() {
    g_data_.clear();
    data_.clear();
    gs_ = std::make_unique<GlobalState>(storage_.get());

    auto controller = std::make_unique<MockEController>(storage_.get(), 10);
    controller_ = controller.get();

    EXPECT_CALL(*controller_, Load).WillRepeatedly(Invoke([&](
      const int64_t commit_id, const uint256_t& address, int version) {
            return controller_->LoadInternal(commit_id, address, version);
    }));

    EXPECT_CALL(*controller_, Store).WillRepeatedly(Invoke([&](
      const int64_t commit_id, const uint256_t& key, const uint256_t& value, int version) {
          return controller_->StoreInternal(commit_id, key, value, version);
    }));


    committer_ = std::make_unique<StreamingECommitter>(storage_.get(), gs_.get(), 10);
    committer_->SetController(std::move(controller));

    contract_address_ = AddressManager::CreateContractAddress(owner_address_);
    deployer_ = std::make_unique<ContractDeployer>(committer_.get(), gs_.get());
    contract_address_ = deployer_->DeployContract(owner_address_, contract_json_, {1000});
  }

  void ExecContract(std::vector<ContractExecuteInfo>& execute_info) {
    for(int i = 0; i < execute_info.size();++i){
      std::string func_addr =
        deployer_->GetFuncAddress(execute_info[i].contract_address, execute_info[i].func_params.func_name());
      if (func_addr.empty()) {
        LOG(ERROR) << "no fouction:" << execute_info[i].func_params.func_name();
        execute_info[i].contract_address = 0;
        continue;
      }
      execute_info[i].func_addr = func_addr;
      execute_info[i].commit_id = i+1;
    }
    committer_->AsyncExecContract(execute_info);
  }

 protected:
  Address owner_address_;
  Address contract_address_;
  nlohmann::json contract_json_;
  std::unique_ptr<MockDStorage> storage_;
  std::unique_ptr<GlobalState> gs_;
  std::unique_ptr<StreamingECommitter>committer_;
  MockEController* controller_;
  std::map<uint256_t, std::pair<uint256_t,int>> data_, g_data_;
  std::unique_ptr<ContractDeployer> deployer_;
};

TEST_F(StreamingECommitterTest, ExecContract) {
  // owner 1000
  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();
  std::vector<ContractExecuteInfo> info;
  {
    Params func_params;
    func_params.set_func_name("get(address)");
    func_params.add_param(U256ToString(owner_address_));
    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "", func_params, 0));
  }

  committer_->SetExecuteCallBack([&](std::unique_ptr<ExecuteResp> resp){
      LOG(ERROR)<<"get resp:"<<resp->commit_id;
      done.set_value(true);
  });

  {
    ExecContract(info);
  }
  done_future.get();
  uint256_t owner_key = AddressManager::AddressToSHAKey(owner_address_);
  //LOG(ERROR)<<"owner key:"<<owner_key<<" data:"<<data_[owner_key].first;
  EXPECT_EQ(data_[owner_key].first, 1000);
}

TEST_F(StreamingECommitterTest, ExecNoConflictContract) {

  // owner 1000
  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();
  std::vector<ContractExecuteInfo> info;

  Address transfer_receiver = get_random_address();
  Address transfer_receiver2 = get_random_address();

  {
    Params func_params;
    func_params.set_func_name("transfer(address,address,uint256)");
    func_params.add_param(U256ToString(owner_address_));
    func_params.add_param(U256ToString(transfer_receiver));
    func_params.add_param(U256ToString(100));

    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "", func_params, 0));
  }
  {
    Params func_params;
    func_params.set_func_name("transfer(address,address,uint256)");
    func_params.add_param(U256ToString(owner_address_));
    func_params.add_param(U256ToString(transfer_receiver2));
    func_params.add_param(U256ToString(100));

    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "", func_params, 0));
  }


  std::set<int> done_list;
  committer_->SetExecuteCallBack([&](std::unique_ptr<ExecuteResp> resp){
      assert(resp != nullptr);
      LOG(ERROR)<<"get resp:"<<resp->commit_id;
      done_list.insert(resp->commit_id);
      if(done_list.size()==2){
        done.set_value(true);
      }
  });

  {
    ExecContract(info);
  }
  done_future.get();

  uint256_t owner_key = AddressManager::AddressToSHAKey(owner_address_);
  uint256_t recer = AddressManager::AddressToSHAKey(transfer_receiver);
  uint256_t recer2 = AddressManager::AddressToSHAKey(transfer_receiver2);
  EXPECT_EQ(data_[owner_key].first, 800);
  EXPECT_EQ(data_[recer].first, 100);
  EXPECT_EQ(data_[recer2].first, 100);
}

TEST_F(StreamingECommitterTest, ExecConflictContract) {
  // owner 1000
  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();
  std::vector<ContractExecuteInfo> info;

  std::set<int64_t> ids;
  EXPECT_CALL(*controller_, Load).WillRepeatedly(Invoke([&](
      const int64_t commit_id, const uint256_t& address, int version) {
      if(commit_id==1){
        while(ids.find(2) == ids.end()){
          sleep(1);
        }
      }
      ids.insert(commit_id);
      return controller_->LoadInternal(commit_id, address, version);
  }));

  
  Address transfer_receiver = get_random_address();
  Address transfer_receiver2 = get_random_address();

  {
    Params func_params;
    func_params.set_func_name("transfer(address,address,uint256)");
    func_params.add_param(U256ToString(owner_address_));
    func_params.add_param(U256ToString(transfer_receiver));
    func_params.add_param(U256ToString(100));

    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "", func_params, 0));
  }
  {
    Params func_params;
    func_params.set_func_name("transfer(address,address,uint256)");
    func_params.add_param(U256ToString(owner_address_));
    func_params.add_param(U256ToString(transfer_receiver2));
    func_params.add_param(U256ToString(100));

    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "", func_params, 0));
  }


  std::set<int> done_list;
  committer_->SetExecuteCallBack([&](std::unique_ptr<ExecuteResp> resp){
      LOG(ERROR)<<"get resp:"<<resp->commit_id;
      done_list.insert(resp->commit_id);
      if(done_list.size()==2){
        done.set_value(true);
      }
  });

  {
    ExecContract(info);
  }
  done_future.get();

  uint256_t owner_key = AddressManager::AddressToSHAKey(owner_address_);
  uint256_t recer = AddressManager::AddressToSHAKey(transfer_receiver);
  uint256_t recer2 = AddressManager::AddressToSHAKey(transfer_receiver2);
  EXPECT_EQ(data_[owner_key].first, 800);
  EXPECT_EQ(data_[recer].first, 100);
  EXPECT_EQ(data_[recer2].first, 100);

}

TEST_F(StreamingECommitterTest, ExecConflictContractAhead2) {
  // owner 1000
  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();
  std::vector<ContractExecuteInfo> info;

  std::map<int64_t, int> ids;
  EXPECT_CALL(*controller_, Load).WillRepeatedly(Invoke([&](
      const int64_t commit_id, const uint256_t& address, int version) {
      if(commit_id==1){
        while(ids.find(2) == ids.end()){
          sleep(1);
        }
      }
      LOG(ERROR)<<"======== load :"<<commit_id<<" version:"<<version;
      ids[commit_id] = version;
      return controller_->LoadInternal(commit_id, address, version);
  }));

  Address transfer_receiver = get_random_address();
  Address transfer_receiver2 = get_random_address();
  Address transfer_receiver3 = get_random_address();

  {
    Params func_params;
    func_params.set_func_name("transfer(address,address,uint256)");
    func_params.add_param(U256ToString(owner_address_));
    func_params.add_param(U256ToString(transfer_receiver));
    func_params.add_param(U256ToString(100));

    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "", func_params, 0));
  }
  {
    Params func_params;
    func_params.set_func_name("transfer(address,address,uint256)");
    func_params.add_param(U256ToString(owner_address_));
    func_params.add_param(U256ToString(transfer_receiver2));
    func_params.add_param(U256ToString(100));

    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "", func_params, 0));
  }

  {
    Params func_params;
    func_params.set_func_name("get(address)");
    func_params.add_param(U256ToString(transfer_receiver3));
    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "", func_params, 0));
  }

  std::set<int> done_list;
  committer_->SetExecuteCallBack([&](std::unique_ptr<ExecuteResp> resp){
      LOG(ERROR)<<"get resp:"<<resp->commit_id;
      done_list.insert(resp->commit_id);
      if(done_list.size()==3){
        done.set_value(true);
      }
  });

  {
    ExecContract(info);
  }
  done_future.get();
  EXPECT_EQ(ids[2], 1);
  EXPECT_EQ(ids[1], 0);
  EXPECT_EQ(ids[3], 0);


  uint256_t owner_key = AddressManager::AddressToSHAKey(owner_address_);
  uint256_t recer = AddressManager::AddressToSHAKey(transfer_receiver);
  uint256_t recer2 = AddressManager::AddressToSHAKey(transfer_receiver2);
  EXPECT_EQ(data_[owner_key].first, 800);
  EXPECT_EQ(data_[recer].first, 100);
  EXPECT_EQ(data_[recer2].first, 100);

}

TEST_F(StreamingECommitterTest, ExecConflictContractAhead) {
  // owner 1000
  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();
  std::vector<ContractExecuteInfo> info;

  std::map<int64_t, int> ids;
  EXPECT_CALL(*controller_, Load).WillRepeatedly(Invoke([&](
      const int64_t commit_id, const uint256_t& address, int version) {
      if(commit_id==1){
        while(ids.find(2) == ids.end()){
          sleep(1);
        }
      }
      LOG(ERROR)<<"======== load :"<<commit_id<<" version:"<<version;
      ids[commit_id] = version;
      return controller_->LoadInternal(commit_id, address, version);
  }));


  Address transfer_receiver = get_random_address();
  Address transfer_receiver2 = get_random_address();
  Address transfer_receiver3 = get_random_address();

  {
    Params func_params;
    func_params.set_func_name("transferif(address,address,address,uint256)");
    func_params.add_param(U256ToString(owner_address_));
    func_params.add_param(U256ToString(transfer_receiver));
    func_params.add_param(U256ToString(transfer_receiver2));
    func_params.add_param(U256ToString(300));

    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "", func_params, 0));
  }
  {
    Params func_params;
    func_params.set_func_name("transferif(address,address,address,uint256)");
    func_params.add_param(U256ToString(owner_address_));
    func_params.add_param(U256ToString(transfer_receiver));
    func_params.add_param(U256ToString(transfer_receiver2));
    func_params.add_param(U256ToString(300));

    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "", func_params, 0));
  }

  {
    Params func_params;
    func_params.set_func_name("set(address,uint256)");
    func_params.add_param(U256ToString(transfer_receiver3));
    func_params.add_param(U256ToString(500));
    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "", func_params, 0));
  }

  std::set<int> done_list;
  committer_->SetExecuteCallBack([&](std::unique_ptr<ExecuteResp> resp){
      LOG(ERROR)<<"get resp:"<<resp->commit_id;
      done_list.insert(resp->commit_id);
      if(done_list.size()==3){
        done.set_value(true);
      }
  });

  {
    ExecContract(info);
  }
  done_future.get();

  uint256_t owner_key = AddressManager::AddressToSHAKey(owner_address_);
  uint256_t recer = AddressManager::AddressToSHAKey(transfer_receiver);
  uint256_t recer2 = AddressManager::AddressToSHAKey(transfer_receiver2);
  uint256_t recer3 = AddressManager::AddressToSHAKey(transfer_receiver3);
  EXPECT_EQ(data_[owner_key].first, 400);
  EXPECT_EQ(data_[recer].first, 300);
  EXPECT_EQ(data_[recer2].first, 300);
  EXPECT_EQ(data_[recer3].first, 500);
}

TEST_F(StreamingECommitterTest, ExecConflictPreCommitContract) {
  // owner 1000
  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();
  std::vector<ContractExecuteInfo> info;

  std::map<int64_t, int> ids;
  EXPECT_CALL(*controller_, Load).WillRepeatedly(Invoke([&](
      const int64_t commit_id, const uint256_t& address, int version) {
      if(commit_id==1){
        while(ids.find(2) == ids.end()){
          sleep(1);
        }
      }
      LOG(ERROR)<<"======== load :"<<commit_id<<" version:"<<version;
      ids[commit_id] = version;
      return controller_->LoadInternal(commit_id, address, version);
  }));

  Address transfer_receiver = get_random_address();
  Address transfer_receiver2 = get_random_address();
  Address transfer_receiver3 = get_random_address();

  {
    Params func_params;
    func_params.set_func_name("transferif(address,address,address,uint256)");
    func_params.add_param(U256ToString(owner_address_));
    func_params.add_param(U256ToString(transfer_receiver));
    func_params.add_param(U256ToString(transfer_receiver2));
    func_params.add_param(U256ToString(300));

    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "", func_params, 0));
  }
  {
    Params func_params;
    func_params.set_func_name("transferif(address,address,address,uint256)");
    func_params.add_param(U256ToString(owner_address_));
    func_params.add_param(U256ToString(transfer_receiver));
    func_params.add_param(U256ToString(transfer_receiver2));
    func_params.add_param(U256ToString(300));

    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "", func_params, 0));
  }
  {
    Params func_params;
    func_params.set_func_name("set(address,uint256)");
    func_params.add_param(U256ToString(transfer_receiver2));
    func_params.add_param(U256ToString(500));
    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "", func_params, 0));
  }

  std::set<int> done_list;
  committer_->SetExecuteCallBack([&](std::unique_ptr<ExecuteResp> resp){
      LOG(ERROR)<<"get resp:"<<resp->commit_id;
      done_list.insert(resp->commit_id);
      if(done_list.size()==3){
        done.set_value(true);
      }
  });

  {
    ExecContract(info);
  }
  done_future.get();

  uint256_t owner_key = AddressManager::AddressToSHAKey(owner_address_);
  uint256_t recer = AddressManager::AddressToSHAKey(transfer_receiver);
  uint256_t recer2 = AddressManager::AddressToSHAKey(transfer_receiver2);
  uint256_t recer3 = AddressManager::AddressToSHAKey(transfer_receiver3);
  EXPECT_EQ(data_[owner_key].first, 400);
  EXPECT_EQ(data_[recer].first, 300);
  EXPECT_EQ(data_[recer2].first, 800);
}

TEST_F(StreamingECommitterTest, ExecConflictPreCommitContract2) {
  // owner 1000
  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();
  std::vector<ContractExecuteInfo> info;

  bool start = false;
  std::map<uint256_t, int> load_time;
  EXPECT_CALL(*storage_, Load).WillRepeatedly(Invoke([&](const uint256_t& address, bool is_local) {
      if(is_local){
        if(data_.find(address) == data_.end()){
        data_[address] = g_data_[address];
        }
      }
      if(is_local){
        load_time[address]++;
      }
      auto ret = is_local? data_[address]: g_data_[address];
      LOG(ERROR)<<"load add:"<<address<<" is local:"<<is_local<<" ret:"<<ret.first<<" ver:"<<ret.second;
      return ret;
  }));

  EXPECT_CALL(*storage_, Store).WillRepeatedly(Invoke([&](const uint256_t& key, const uint256_t& value, bool is_local) {
      LOG(ERROR)<<"store key:"<<key<<" value:"<<value<<" is local:"<<is_local;
      if(start && is_local) {
        bool done = false;
        while(!done){
          for(auto it : load_time){
            if(it.second>1){
              done = true;
              break;
            }
          }
        }
      }

      if(is_local){
        int v = data_[key].second;
        data_[key] = std::make_pair(value, v+1);
      }
      else {
        int v = g_data_[key].second;
        g_data_[key] = std::make_pair(value, v+1);
      }
      return 1;
  }));


  Address transfer_receiver = get_random_address();
  Address transfer_receiver2 = get_random_address();
  Address transfer_receiver3 = get_random_address();

  {
    Params func_params;
    func_params.set_func_name("transferif(address,address,address,uint256)");
    func_params.add_param(U256ToString(owner_address_));
    func_params.add_param(U256ToString(transfer_receiver));
    func_params.add_param(U256ToString(transfer_receiver2));
    func_params.add_param(U256ToString(300));

    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "", func_params, 0));
  }
  {
    Params func_params;
    func_params.set_func_name("transferif(address,address,address,uint256)");
    func_params.add_param(U256ToString(owner_address_));
    func_params.add_param(U256ToString(transfer_receiver));
    func_params.add_param(U256ToString(transfer_receiver2));
    func_params.add_param(U256ToString(300));

    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "", func_params, 0));
  }
  {
    Params func_params;
    func_params.set_func_name("set(address,uint256)");
    func_params.add_param(U256ToString(owner_address_));
    func_params.add_param(U256ToString(500));
    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "", func_params, 0));
  }

  std::set<int> done_list;
  committer_->SetExecuteCallBack([&](std::unique_ptr<ExecuteResp> resp){
      LOG(ERROR)<<"get resp:"<<resp->commit_id;
      done_list.insert(resp->commit_id);
      if(done_list.size()==3){
        done.set_value(true);
      }
  });

  start = true;
  {
    ExecContract(info);
  }
  done_future.get();

  uint256_t owner_key = AddressManager::AddressToSHAKey(owner_address_);
  uint256_t recer = AddressManager::AddressToSHAKey(transfer_receiver);
  uint256_t recer2 = AddressManager::AddressToSHAKey(transfer_receiver2);
  uint256_t recer3 = AddressManager::AddressToSHAKey(transfer_receiver3);
  EXPECT_EQ(data_[owner_key].first, 900);
  EXPECT_EQ(data_[recer].first, 300);
  EXPECT_EQ(data_[recer2].first, 300);
}

TEST_F(StreamingECommitterTest, ExecConflictCommitContract) {
  // owner 1000
  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();
  std::vector<ContractExecuteInfo> info;


  bool start = false;
  std::map<uint256_t, int> load_time;
  EXPECT_CALL(*storage_, Load).WillRepeatedly(Invoke([&](const uint256_t& address, bool is_local) {
      if(is_local){
        if(data_.find(address) == data_.end()){
        data_[address] = g_data_[address];
        }
      }
      if(is_local){
        load_time[address]++;
      }
      auto ret = is_local? data_[address]: g_data_[address];
      LOG(ERROR)<<"load add:"<<address<<" is local:"<<is_local<<" ret:"<<ret.first<<" ver:"<<ret.second;
      return ret;
  }));

  EXPECT_CALL(*storage_, Store).WillRepeatedly(Invoke([&](const uint256_t& key, const uint256_t& value, bool is_local) {
      LOG(ERROR)<<"store key:"<<key<<" value:"<<value<<" is local:"<<is_local;
      if(start && is_local) {
        bool done = false;
        while(!done){
          for(auto it : load_time){
            if(it.second>1){
              done = true;
              break;
            }
          }
        }
      }

      if(is_local){
        int v = data_[key].second;
        data_[key] = std::make_pair(value, v+1);
      }
      else {
        int v = g_data_[key].second;
        g_data_[key] = std::make_pair(value, v+1);
      }
      return 1;
  }));

  EXPECT_CALL(*storage_, Reset).WillRepeatedly(Invoke([&](const uint256_t& key, const uint256_t& value, int64_t version, bool is_local) {
      LOG(ERROR)<<"reset key:"<<key<<" value:"<<value<<" version:"<<version<<" is local:"<<is_local;
      if(is_local){
        data_[key] = std::make_pair(value, version);
      }
      else {
        g_data_[key] = std::make_pair(value, version);
      }
  }));



  Address transfer_receiver = get_random_address();
  Address transfer_receiver2 = get_random_address();
  Address transfer_receiver3 = get_random_address();

  {
    Params func_params;
    func_params.set_func_name("transferif(address,address,address,uint256)");
    func_params.add_param(U256ToString(owner_address_));
    func_params.add_param(U256ToString(transfer_receiver));
    func_params.add_param(U256ToString(transfer_receiver2));
    func_params.add_param(U256ToString(300));

    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "", func_params, 0));
  }
  {
    Params func_params;
    func_params.set_func_name("transferif(address,address,address,uint256)");
    func_params.add_param(U256ToString(owner_address_));
    func_params.add_param(U256ToString(transfer_receiver));
    func_params.add_param(U256ToString(transfer_receiver2));
    func_params.add_param(U256ToString(300));

    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "", func_params, 0));
  }
  {
    Params func_params;
    func_params.set_func_name("set(address,uint256)");
    func_params.add_param(U256ToString(transfer_receiver));
    func_params.add_param(U256ToString(500));
    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "", func_params, 0));
  }

  std::set<int> done_list;
  committer_->SetExecuteCallBack([&](std::unique_ptr<ExecuteResp> resp){
      LOG(ERROR)<<"get resp:"<<resp->commit_id;
      done_list.insert(resp->commit_id);
      if(done_list.size()==3){
        done.set_value(true);
      }
  });

  start = true;
  {
    ExecContract(info);
  }
  done_future.get();

  uint256_t owner_key = AddressManager::AddressToSHAKey(owner_address_);
  uint256_t recer = AddressManager::AddressToSHAKey(transfer_receiver);
  uint256_t recer2 = AddressManager::AddressToSHAKey(transfer_receiver2);
  uint256_t recer3 = AddressManager::AddressToSHAKey(transfer_receiver3);
  EXPECT_EQ(data_[owner_key].first, 400);
  EXPECT_EQ(data_[recer].first, 800);
  EXPECT_EQ(data_[recer2].first, 300);

}

// rollback two columns
TEST_F(StreamingECommitterTest, ExecConflictCommitContract2) {
  // owner 1000
  std::promise<bool> done;
  std::future<bool> done_future = done.get_future();
  std::vector<ContractExecuteInfo> info;

  std::set<int64_t> pre_ids;
  controller_->SetPrecommitCallback([&](int64_t id){
    pre_ids.insert(id);
  });

  std::map<int64_t, int> ids;
  EXPECT_CALL(*controller_, Load).WillRepeatedly(Invoke([&](
      const int64_t commit_id, const uint256_t& address, int version) {
      if(commit_id==2 && version>0){
        while(pre_ids.size()<5){
          LOG(ERROR)<<"======== load :"<<commit_id<<" version:"<<version<<" data size:"<<pre_ids.size();
          sleep(1);
        }
      }
      ids[commit_id] = version;
      return controller_->LoadInternal(commit_id, address, version);
  }));

  Address transfer_receiver = get_random_address();
  Address transfer_receiver2 = get_random_address();
  Address transfer_receiver3 = get_random_address();
  Address transfer_receiver4 = get_random_address();

  {
    Params func_params;
    func_params.set_func_name("transferif(address,address,address,uint256)");
    func_params.add_param(U256ToString(owner_address_));
    func_params.add_param(U256ToString(transfer_receiver));
    func_params.add_param(U256ToString(transfer_receiver2));
    func_params.add_param(U256ToString(300));

    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "", func_params, 0));
  }
  {
    Params func_params;
    func_params.set_func_name("transferif(address,address,address,uint256)");
    func_params.add_param(U256ToString(owner_address_));
    func_params.add_param(U256ToString(transfer_receiver));
    func_params.add_param(U256ToString(transfer_receiver2));
    func_params.add_param(U256ToString(300));

    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "", func_params, 0));
  }
  {
    Params func_params;
    func_params.set_func_name("set(address,uint256)");
    func_params.add_param(U256ToString(transfer_receiver));
    func_params.add_param(U256ToString(500));
    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "", func_params, 0));
  }
  {
    Params func_params;
    func_params.set_func_name("transfer(address,address,uint256)");
    func_params.add_param(U256ToString(transfer_receiver));
    func_params.add_param(U256ToString(transfer_receiver3));
    func_params.add_param(U256ToString(300));

    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "", func_params, 0));
  }
  {
    Params func_params;
    func_params.set_func_name("set(address,uint256)");
    func_params.add_param(U256ToString(transfer_receiver3));
    func_params.add_param(U256ToString(500));
    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "", func_params, 0));
  }
  {
    Params func_params;
    func_params.set_func_name("set(address,uint256)");
    func_params.add_param(U256ToString(transfer_receiver));
    func_params.add_param(U256ToString(500));
    info.push_back(ContractExecuteInfo(owner_address_, contract_address_, "", func_params, 0));
  }

  std::set<int> done_list;
  committer_->SetExecuteCallBack([&](std::unique_ptr<ExecuteResp> resp){
      LOG(ERROR)<<"!!!!! get resp:"<<resp->commit_id;
      done_list.insert(resp->commit_id);
      if(done_list.size()==6){
        done.set_value(true);
      }
  });

  {
    ExecContract(info);
  }
  done_future.get();

  uint256_t owner_key = AddressManager::AddressToSHAKey(owner_address_);
  uint256_t recer = AddressManager::AddressToSHAKey(transfer_receiver);
  uint256_t recer2 = AddressManager::AddressToSHAKey(transfer_receiver2);
  uint256_t recer3 = AddressManager::AddressToSHAKey(transfer_receiver3);
  LOG(ERROR)<<"recr:"<<recer;
  EXPECT_EQ(data_[owner_key].first, 400);
  EXPECT_EQ(data_[recer].first, 1000);
  EXPECT_EQ(data_[recer2].first, 300);
  EXPECT_EQ(data_[recer3].first, 800);
}


}  // namespace
}
}  // namespace contract
}  // namespace resdb

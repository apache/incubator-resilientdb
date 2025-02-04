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
#include "service/contract/executor/x_manager/d_storage.h"

namespace resdb {
namespace contract {
namespace x_manager {

class PerformanceExecutor {
 public:
  PerformanceExecutor(int worker_num = 2) {
    auto tmp_storage = std::make_unique<D_Storage>();
    // auto tmp_storage = std::make_unique<DataStorage>();
    storage_ = tmp_storage.get();
    manager_ = std::make_unique<ContractManager>(
        std::move(tmp_storage), worker_num, ContractManager::Options::None);
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

  void AsyncExecuteMulti(std::vector<ContractExecuteInfo>& info) {
    manager_->AsyncExecContract(info);
  }

  void SetAsyncCallBack(
      std::function<void(std::unique_ptr<ExecuteResp>)> func) {
    manager_->SetExecuteCallBack(std::move(func));
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

class TimeTrack {
 public:
  TimeTrack(std::string name = "") {
    name_ = name;
    start_time_ = GetCurrentTime();
  }

  ~TimeTrack() {
    // uint64_t end_time = GetCurrentTime();
    // LOG(ERROR) << name_ <<" run:" << (end_time - start_time_) /
    // 1000000.0<<"s";
  }

  double GetRunTime() {
    uint64_t end_time = GetCurrentTime();
    return (end_time - start_time_) / 1000000.0;
  }

 private:
  std::string name_;
  uint64_t start_time_;
};

std::vector<Address> account_list, contract_list;
std::vector<Address> user_list;

void Init(int user_num, int contract_num, PerformanceExecutor* executor) {
  contract_list.clear();
  account_list.clear();
  user_list.clear();

  const std::string data_dir =
      std::string(getenv("PWD")) + "/" + "service/contract/performance/";
  std::string contract_path = data_dir + "data/kv.json";

  std::ifstream contract_fstream(contract_path, std::ifstream::in);
  if (!contract_fstream) {
    throw std::runtime_error(fmt::format(
        "Unable to open contract definition file {}", contract_path));
  }

  nlohmann::json definition = nlohmann::json::parse(contract_fstream);
  nlohmann::json contracts_json_ = definition["contracts"];
  std::string contract_name = "kv.sol:KV";
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

  {
    TimeTrack tract("create user");
    for (int i = 0; i < user_num; ++i) {
      Address account = executor->CreateAccount();
      user_list.push_back(account);
    }
  }
  {
    TimeTrack tract("deploy contract");
    for (int i = 0; i < contract_num; ++i) {
      Address account = executor->CreateAccount();
      account_list.push_back(account);

      Address contract_addr = executor->Deploy(account, deploy_info);
      // LOG(ERROR)<<"contract address:"<<contract_addr;
      contract_list.push_back(contract_addr);
    }
  }
}

struct ResultState {
  double run_time;
  int max_retry;
  double avg_retry;
  double latency;
  double execution_time;
};

std::vector<std::string> assigned;
int next_ = 0;
int group = 2;
std::string GetAddress(int c, int idx, int user_num) {
  if (c > 0) {
    int p = rand() % 100;
    if (assigned.size() > group && p < c) {
      int n_idx = rand() % (assigned.size() / group);
      return assigned[n_idx * group + idx];
    } else {
      std::string ret =
          AddressManager::AddressToHex(user_list[(next_++) % user_list.size()]);
      assigned.push_back(ret);
      return ret;
    }
  } else {
    std::string ret =
        AddressManager::AddressToHex(user_list[(next_++) % user_list.size()]);
    assigned.push_back(ret);
    return ret;
  }
}

Params GetFuncParams(std::string func_name, int c, int idx, int user_num) {
  Params func_params;
  func_params.set_func_name("add(address,address,uint256)");
  func_params.add_param(GetAddress(c, 0, user_num));
  func_params.add_param(GetAddress(c, 1, user_num));
  func_params.add_param(std::to_string(rand() % 1000));
  return func_params;
}

ResultState testMulti(int worker_num, int num, int contract_num, int user_num,
                      size_t batch_num, PerformanceExecutor* executor, int c,
                      bool is_async) {
  std::vector<ContractExecuteInfo> info, tmp_info;
  assigned.clear();
  // next_ = 0;

  PerformanceExecutor pexecutor(worker_num);
  Init(user_num, contract_num, &pexecutor);

  executor = &pexecutor;

  for (int i = 0; i < num; ++i) {
    Params func_params = GetFuncParams("transfer", c, i, user_num);
    tmp_info.push_back(ContractExecuteInfo(account_list[0], contract_list[0],
                                           "", func_params, 0));
  }

  TimeTrack tract("execute contract");
  std::vector<int> redo_time;
  std::vector<double> execution_time;
  std::map<int, uint64_t> submit_time, commit_time;
  int id = 0;
  for (auto& i : tmp_info) {
    submit_time[id] = GetCurrentTime();
    i.user_id = id++;
    info.push_back(i);
    if (info.size() == batch_num) {
      std::vector<std::unique_ptr<ExecuteResp>> ret =
          executor->ExecuteMulti(info);

      for (auto& resp : ret) {
        // if(resp->retry_time==0 || resp->retry_time>1)
        // printf("do time %d\n",resp->retry_time);
        redo_time.push_back(resp->retry_time);
        if (resp->runtime > 0) {
          execution_time.push_back(resp->runtime);
        }
        commit_time[resp->user_id] = GetCurrentTime();
      }
      info.clear();
    }
  }

  if (info.size() > 0) {
    std::vector<std::unique_ptr<ExecuteResp>> ret =
        executor->ExecuteMulti(info);
    for (auto& resp : ret) {
      // if(resp->retry_time==0 || resp->retry_time>1)
      // printf("do time %d\n",resp->retry_time);
      redo_time.push_back(resp->retry_time);
      commit_time[resp->user_id] = GetCurrentTime();
    }
  }

  double latency = 0;
  assert(submit_time.size() > 0);
  for (auto it : submit_time) {
    latency += commit_time[it.first] - it.second;
  }
  latency = latency / submit_time.size() / 1000000.0;

  double exe_time = 0;
  for (auto it : execution_time) {
    exe_time += it;
  }
  exe_time = exe_time / execution_time.size();
  // LOG(ERROR)<<"avg lat:"<<latency<<"s";

  int retry_sum = 0, max_retry = 0, retry_num = 0;
  for (int d : redo_time) {
    max_retry = std::max(max_retry, d);
    retry_sum += d;
    if (d > 0) {
      retry_num++;
    }
  }

  double run_time = tract.GetRunTime();
  ResultState result{run_time, max_retry,
                     retry_num ? (double)retry_sum / retry_num : 0, latency,
                     exe_time};
  return result;
}

int contract_num = 500;
int user_num = 100000;
int run_num = 100;
// int run_num = 1000;

struct RunTimeInfo {
  double exe_time, latency, redo_time, run_time;
  bool operator<(RunTimeInfo& a) { return this->run_time < a.run_time; }
};

double GetRuntime(std::vector<RunTimeInfo> list) {
  int s = 0, e = list.size();
  if (list.size() > 2) {
    e--;
  }
  if (list.size() > 1) {
    s++;
  }
  double sum = 0;
  for (int i = s; i < e; ++i) {
    // LOG(ERROR)<<"get idx:"<<i<<" run time:"<<list[i].run_time<<" exe
    // time:"<<list[i].exe_time;
    sum += list[i].run_time;
  }
  return sum / (e - s);
}

double GetTps(std::vector<RunTimeInfo> list) {
  int s = 0, e = list.size();
  if (list.size() > 2) {
    e--;
  }
  if (list.size() > 1) {
    s++;
  }
  double sum = 0;
  for (int i = s; i < e; ++i) {
    //  LOG(ERROR)<<"get idx:"<<i<<" run time:"<<list[i].run_time;
    sum += list[i].run_time;
  }
  return (e - s) / (sum / (e - s));
}

double GetExetime(std::vector<RunTimeInfo> list) {
  int s = 0, e = list.size();
  if (list.size() > 2) {
    e--;
  }
  if (list.size() > 1) {
    s++;
  }
  double sum = 0;
  for (int i = s; i < e; ++i) {
    sum += list[i].exe_time;
  }
  return sum / (e - s);
}

double GetLat(std::vector<RunTimeInfo> list) {
  int s = 0, e = list.size();
  if (list.size() > 2) {
    e--;
  }
  if (list.size() > 1) {
    s++;
  }
  double sum = 0;
  for (int i = s; i < e; ++i) {
    sum += list[i].latency;
  }
  return sum / (e - s);
}

double GetRedo(std::vector<RunTimeInfo> list) {
  int s = 0, e = list.size();
  if (list.size() > 2) {
    e--;
  }
  if (list.size() > 1) {
    s++;
  }
  double sum = 0;
  for (int i = s; i < e; ++i) {
    sum += list[i].redo_time;
  }
  return sum / (e - s);
}

ResultState GetMultiPerformanceData(int worker_num, int num, int batch_num,
                                    int c, bool is_async) {
  PerformanceExecutor executor(worker_num);
  Init(user_num, contract_num, &executor);

  // double run_time = 0;
  // double redo_time = 0;
  int max_retry = 0;
  // double latency = 0;
  // double exe_time = 0;

  std::vector<RunTimeInfo> info_list;
  for (int i = 0; i < run_num; ++i) {
    auto ret = testMulti(worker_num, num, contract_num, user_num, batch_num,
                         &executor, c, is_async);
    // run_time += ret.run_time;
    // redo_time += ret.avg_retry;
    max_retry = std::max(max_retry, ret.max_retry);
    // latency += ret.latency;
    // exe_time += ret.execution_time;
    info_list.push_back(
        {ret.execution_time, ret.latency, ret.avg_retry, ret.run_time});
  }

  sort(info_list.begin(), info_list.end());
  // LOG(ERROR)<<"info size:"<<info_list.size()<<" run
  // time:"<<GetRuntime(info_list);

  // return ResultState{run_time/run_num, max_retry, redo_time/run_num,
  // latency/run_num, exe_time/run_num};
  return ResultState{GetRuntime(info_list), max_retry, GetRedo(info_list),
                     GetLat(info_list), GetExetime(info_list)};
}

void GetPerformanceData(bool is_async) {
  int num = 100;

  // double single_run_time = GetSinglePerformanceData(num);
  // printf("single tps: %lf\n",num/single_run_time);

  //  std::vector<int> worker_num{1,2,4,8,16};
  // std::vector<int> conflict_num{0,20,50,100};
  // std::vector<int> batch_num{100,500};
  std::vector<int> worker_num{16};
  // std::vector<int> worker_num{1,2,4,8,16};
  std::vector<int> conflict_num{0, 10, 20, 30, 40, 50};
  // std::vector<int> conflict_num{0, 20, 50};
  std::vector<int> batch_num{500};

  for (int batch : batch_num) {
    for (int c : conflict_num) {
      for (int worker : worker_num) {
        num = batch;
        auto multi_runtime =
            GetMultiPerformanceData(worker, num, batch, c, is_async);
        printf(
            "multi worker: %d batch %d, confilt %d%% tps %lf avg try time %lf "
            "max retry_time %d latency %lf exe_time %lf\n",
            worker, batch, c, num / multi_runtime.run_time,
            multi_runtime.avg_retry, multi_runtime.max_retry,
            multi_runtime.latency, multi_runtime.execution_time);
      }
    }
  }
}

int main() {
  srand(time(0));

  GetPerformanceData(false);
}

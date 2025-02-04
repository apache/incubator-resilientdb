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

#include "service/contract/benchmark/occ_bm.h"

#include <fmt/core.h>

#include <fstream>
#include <future>

#include "common/utils/utils.h"
#include "glog/logging.h"
#include "service/contract/benchmark/generator/zipfian_generator.h"

namespace resdb {
namespace contract {

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
    // LOG(ERROR)<<"get idx:"<<i<<" run time:"<<list[i].run_time;
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
    LOG(ERROR) << "get idx:" << i << " run time:" << list[i].run_time;
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

PerformanceExecutor::PerformanceExecutor(int worker_num) {}

Address PerformanceExecutor::CreateAccount() {
  return AddressManager().CreateRandomAddress();
}

Address PerformanceExecutor::Deploy(const Address& address,
                                    DeployInfo deploy_info) {
  Address contract_address = manager_->DeployContract(address, deploy_info);
  assert(contract_address > 0);
  return contract_address;
}

absl::StatusOr<std::string> PerformanceExecutor::ExecuteOne(
    const Address& caller_address, const Address& contract_address,
    const Params& params) {
  return manager_->ExecContract(caller_address, contract_address, params);
}

std::vector<std::unique_ptr<ExecuteResp>> PerformanceExecutor::ExecuteMulti(
    std::vector<ContractExecuteInfo>& info) {
  return manager_->ExecContract(info);
}

void PerformanceExecutor::AsyncExecuteMulti(
    std::vector<ContractExecuteInfo>& info) {
  manager_->AsyncExecContract(info);
}

void PerformanceExecutor::SetAsyncCallBack(
    std::function<void(std::unique_ptr<ExecuteResp>)> func) {
  manager_->SetExecuteCallBack(std::move(func));
}

uint256_t PerformanceExecutor::Get(uint256_t key) {
  return storage_->Load(key).first;
}

OCCBM ::OCCBM(const std::string& name, bool sync, int user_num,
              int contract_num, int run_time)
    : name_(name),
      sync_(sync),
      user_num_(user_num),
      contract_num_(contract_num),
      run_time_(run_time) {}

void OCCBM::Init(PerformanceExecutor* executor, double alpha) {
  generator_ = std::make_unique<StringGenerator>(
      std::make_unique<ZipfianGenerator>(user_num_, alpha));
  contract_list_.clear();
  account_list_.clear();
  user_list_.clear();

  InitContract(executor);

  for (const uint256_t& addr : user_list_) {
    generator_->AddString(AddressManager::AddressToHex(addr));
  }
}

void OCCBM::InitContract(PerformanceExecutor* executor) {
  // const std::string data_dir = std::string(getenv("PWD")) + "/" +
  // "service/contract/performance/"; std::string contract_path = data_dir +
  // "data/kv.json";
  std::string contract_path = GetContractPath();

  std::ifstream contract_fstream(contract_path, std::ifstream::in);
  if (!contract_fstream) {
    throw std::runtime_error(fmt::format(
        "Unable to open contract definition file {}", contract_path));
  }

  nlohmann::json definition = nlohmann::json::parse(contract_fstream);
  // nlohmann::json contracts_json_ = definition["contracts"];
  // std::string contract_name = "kv.sol:KV";
  nlohmann::json contracts_json_ = definition[GetContractDef()];
  std::string contract_name = GetContractName();
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
    for (int i = 0; i < user_num_; ++i) {
      Address account = executor->CreateAccount();
      user_list_.push_back(account);
    }
  }
  {
    for (int i = 0; i < contract_num_; ++i) {
      Address account = executor->CreateAccount();
      account_list_.push_back(account);

      Address contract_addr = executor->Deploy(account, deploy_info);
      contract_list_.push_back(contract_addr);
    }
  }
}

std::unique_ptr<PerformanceExecutor> OCCBM::GetExecutor(int worker_num) {
  return nullptr;
}

Params OCCBM::GetFuncParams(StringGenerator* generator) {
  Params func_params;
  func_params.set_func_name("transfer(address,uint256)");
  func_params.add_param(generator->Next());
  func_params.add_param(std::to_string(rand() % 1000));
  return func_params;
}

ResultState OCCBM::Test(int worker_num, int batch_num, double alpha) {
  std::vector<ContractExecuteInfo> info;

  std::unique_ptr<PerformanceExecutor> executor = GetExecutor(worker_num);
  executor->InitManager();

  Init(executor.get(), alpha);

  for (int i = 0; i < batch_num; ++i) {
    Params func_params = GetFuncParams(generator_.get());
    info.push_back(ContractExecuteInfo(account_list_[0], contract_list_[0], "",
                                       func_params, 0));
  }

  std::vector<int> redo_time;
  std::vector<double> execution_time;
  std::map<int, uint64_t> submit_time, commit_time;
  int id = 0;
  for (auto& i : info) {
    submit_time[id] = GetCurrentTime();
    i.user_id = id++;
  }

  double run_time = 0;
  if (sync_) {
    TimeTrack tract("execute contract");
    std::vector<std::unique_ptr<ExecuteResp>> ret =
        executor->ExecuteMulti(info);
    for (auto& resp : ret) {
      redo_time.push_back(resp->retry_time);
      if (resp->runtime > 0) {
        execution_time.push_back(resp->runtime);
      }
      commit_time[resp->user_id] = GetCurrentTime();
    }
    run_time = tract.GetRunTime();
  } else {
    std::promise<bool> done;
    std::future<bool> done_future = done.get_future();
    TimeTrack tract("execute contract");
    executor->SetAsyncCallBack([&](std::unique_ptr<ExecuteResp> resp) {
      assert(resp != nullptr);
      commit_time[resp->user_id] = GetCurrentTime();
      if (resp->runtime > 0) {
        execution_time.push_back(resp->runtime);
      }
      // LOG(ERROR)<<"get response:"<<resp->commit_id<<"
      // user_id:"<<resp->user_id<<" size:"<<commit_time.size();
      if (commit_time.size() == info.size()) {
        done.set_value(true);
      }
      redo_time.push_back(resp->retry_time);
    });

    executor->AsyncExecuteMulti(info);
    done_future.get();
    run_time = tract.GetRunTime();
  }

  double latency = 0;
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
    retry_num++;
  }
  ResultState result{run_time, max_retry,
                     retry_num ? (double)retry_sum / retry_num : 0, latency,
                     exe_time};
  return result;
}

ResultState OCCBM::Process(int worker_num, int batch_num, double alpha) {
  srand(1234);
  int max_retry = 0;
  std::vector<RunTimeInfo> info_list;
  for (int i = 0; i < run_time_; ++i) {
    auto ret = Test(worker_num, batch_num, alpha);
    max_retry = std::max(max_retry, ret.max_retry);
    info_list.push_back(
        {ret.execution_time, ret.latency, ret.avg_retry, ret.run_time});
  }
  sort(info_list.begin(), info_list.end());
  return ResultState{GetRuntime(info_list), max_retry, GetRedo(info_list),
                     GetLat(info_list), GetExetime(info_list)};
}

void OCCBM::Process() {
  std::vector<int> worker_num{1, 4, 8, 12, 16};
  // std::vector<double>
  // alpha_num{0,0.2,0.4,0.6,0.65,0.7,0.75,0.8,0.85,0.9,0.95,1,1.05,1.1,1.15,1.2};
  std::vector<double> alpha_num{0.85};
  // std::vector<double>
  // alpha_num{0,0.2,0.4,0.6,0.65,0.7,0.75,0.8,0.85,0.9,0.95};
  // std::vector<double> alpha_num{0,0.2,0.4, 0.6, 0.65};
  // std::vector<double> alpha_num{0.7,0.75,0.8,0.85,0.9,0.95, 1};
  // std::vector<int> batch_num{300};
  std::vector<int> batch_num{500};

  for (int batch : batch_num) {
    for (double alpha : alpha_num) {
      for (int worker : worker_num) {
        auto multi_runtime = Process(worker, batch, alpha);
        printf(
            "name: %s worker: %d batch %d alpha %lf "
            "tps %lf avg try time %lf max retry_time %d latency %lf exe_time "
            "%lf\n",
            name_.c_str(), worker, batch, alpha, batch / multi_runtime.run_time,
            multi_runtime.avg_retry, multi_runtime.max_retry,
            multi_runtime.latency, multi_runtime.execution_time);
      }
    }
  }
}

}  // namespace contract
}  // namespace resdb

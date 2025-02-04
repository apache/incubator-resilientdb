#pragma once

#include "service/contract/benchmark/generator/string_generator.h"
#include "service/contract/executor/x_manager/address_manager.h"
#include "service/contract/executor/x_manager/contract_manager.h"

namespace resdb {
namespace contract {
namespace x_manager {

class PerformanceExecutor {
 public:
  PerformanceExecutor(int worker_num = 2);

  virtual void InitManager() = 0;

  Address CreateAccount();

  Address Deploy(const Address& address, DeployInfo deploy_info);

  absl::StatusOr<std::string> ExecuteOne(const Address& caller_address,
                                         const Address& contract_address,
                                         const Params& params);

  std::vector<std::unique_ptr<ExecuteResp>> ExecuteMulti(
      std::vector<ContractExecuteInfo>& info);

  void AsyncExecuteMulti(std::vector<ContractExecuteInfo>& info);

  void SetAsyncCallBack(std::function<void(std::unique_ptr<ExecuteResp>)> func);

  uint256_t Get(uint256_t key);

 protected:
  DataStorage* storage_;
  std::unique_ptr<ContractManager> manager_;
};

struct ResultState {
  double run_time;
  int max_retry;
  double avg_retry;
  double latency;
  double execution_time;
  double delay_time;
};

class OCCBM {
 public:
  OCCBM(const std::string& name, bool sync, int user_num = 10000,
        int contract_num = 1000, int run_time = 50);
  void Process();

  virtual std::unique_ptr<PerformanceExecutor> GetExecutor(int worker_num);

  virtual std::string GetContractPath() = 0;
  virtual std::string GetContractDef() = 0;
  virtual std::string GetContractName() = 0;
  virtual Params GetFuncParams(StringGenerator* generator);

 private:
  void Init(PerformanceExecutor* executor, double alpha);
  void InitContract(PerformanceExecutor* executor);

  ResultState Test(int worker_num, int batch_num, double alpha);
  ResultState Process(int worker_num, int batch_num, double alpha);

 protected:
  std::string name_;
  bool sync_;
  int user_num_, contract_num_, run_time_;
  std::vector<Address> account_list_, contract_list_;
  std::vector<Address> user_list_;

  std::unique_ptr<StringGenerator> generator_;
};

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb

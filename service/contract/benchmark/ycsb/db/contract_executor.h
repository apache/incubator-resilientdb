#pragma once

#include "service/contract/benchmark/executor/benchmark_executor.h"
#include "service/contract/executor/common/contract_execute_info.h"

namespace resdb {
namespace contract {
namespace benchmark {

class ContractExecutor {
 public:
  ContractExecutor(ExecutorType type, bool use_leveldb, int worker_num);
  virtual ~ContractExecutor() = default;

  void Init();

  std::vector<std::unique_ptr<ExecuteResp>> Execute(
      std::vector<ContractExecuteInfo>& info);

  Address GetAccountAddress();
  Address GetContractAddress();

 protected:
  virtual std::unique_ptr<BenchmarkExecutor> GetExecutor() = 0;

 private:
  void InitContract(BenchmarkExecutor* executor);
  std::string GetContractPath();
  std::string GetContractDef();
  std::string GetContractName();

 protected:
  ExecutorType type_;
  bool use_leveldb_;
  int worker_num_;
  bool sync_;
  std::unique_ptr<BenchmarkExecutor> executor_;
  std::vector<Address> account_list_, contract_list_;
};

}  // namespace benchmark
}  // namespace contract
}  // namespace resdb

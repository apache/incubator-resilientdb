#pragma once

#include "service/contract/benchmark/executor/benchmark_executor.h"
#include "service/contract/executor/manager/address_manager.h"
#include "service/contract/executor/manager/contract_manager.h"
#include "service/contract/executor/manager/d_storage.h"

namespace resdb {
namespace contract {
namespace benchmark {

class OCCExecutor : public BenchmarkExecutor {
 public:
  OCCExecutor(ExecutorType name, bool use_leveldb, int worker_num);
  virtual ~OCCExecutor() = default;

  Address CreateAccount() override;

  Address Deploy(const Address& address,
                 const DeployInfo& deploy_info) override;

  std::vector<std::unique_ptr<ExecuteResp>> ExecuteContract(
      std::vector<ContractExecuteInfo>& info) override;

 private:
  absl::StatusOr<std::string> ExecuteOne(const Address& caller_address,
                                         const Address& contract_address,
                                         const Params& params);

  void AsyncExecuteMulti(std::vector<ContractExecuteInfo>& info);

  void SetAsyncCallBack(std::function<void(std::unique_ptr<ExecuteResp>)> func);

  uint256_t Get(uint256_t key);

 private:
  void InitManager();
  std::unique_ptr<DataStorage> GetStorage();

 protected:
  ExecutorType name_;
  bool use_leveldb_;
  int worker_num_;
  DataStorage* storage_;
  std::unique_ptr<ContractManager> manager_;
};

}  // namespace benchmark
}  // namespace contract
}  // namespace resdb

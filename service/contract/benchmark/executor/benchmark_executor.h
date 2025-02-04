#pragma once

#include "service/contract/executor/common/contract_execute_info.h"
#include "service/contract/executor/common/utils.h"
#include "service/contract/proto/func_params.pb.h"

namespace resdb {
namespace contract {
namespace benchmark {

enum ExecutorType {
  OCC = 0,
  BAMBOO = 1,
  XP = 2,
  TPL = 3,
};

class BenchmarkExecutor {
 public:
  BenchmarkExecutor() = default;
  virtual ~BenchmarkExecutor() = default;

  virtual Address CreateAccount() = 0;

  virtual Address Deploy(const Address& owner_address,
                         const DeployInfo& deploy_info) = 0;

  virtual std::vector<std::unique_ptr<ExecuteResp>> ExecuteContract(
      std::vector<ContractExecuteInfo>& info) = 0;
};

}  // namespace benchmark
}  // namespace contract
}  // namespace resdb

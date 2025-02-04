#include "service/contract/benchmark/ycsb/occ_contract.h"

#include <glog/logging.h>

#include "common/utils/utils.h"
#include "service/contract/benchmark/executor/occ_executor.h"
#include "service/contract/benchmark/ycsb/db/contract_db.h"
#include "service/contract/benchmark/ycsb/db/contract_executor.h"

namespace resdb {
namespace contract {
namespace benchmark {

class ContractExecutorImpl : public ContractExecutor {
 public:
  ContractExecutorImpl(ExecutorType type, int worker_num)
      : ContractExecutor(type, true, worker_num) {}

  std::unique_ptr<BenchmarkExecutor> GetExecutor() {
    return std::make_unique<OCCExecutor>(type_, use_leveldb_, worker_num_);
  }
};

class ContractDBImpl : public ContractDB {
 public:
  ContractDBImpl(ExecutorType type, int worker_num, int batch_size)
      : ContractDB(batch_size) {
    contract_executor_ =
        std::make_unique<ContractExecutorImpl>(type, worker_num);
    contract_executor_->Init();
  }
};

std::unique_ptr<::ycsbc::DB> OCCContractDB::CreateDB(utils::Properties &props) {
  return std::make_unique<ContractDBImpl>(
      ExecutorType::OCC, stoull(props["worker_num"], NULL, 10),
      stoull(props["batch_size"], NULL, 10));
}

}  // namespace benchmark
}  // namespace contract
}  // namespace resdb

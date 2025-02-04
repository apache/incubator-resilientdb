#include "service/contract/benchmark/ycsb/bam_contract.h"

#include <glog/logging.h>

#include "common/utils/utils.h"
#include "service/contract/benchmark/executor/bam_executor.h"
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
    return std::make_unique<BamExecutor>(type_, use_leveldb_, worker_num_);
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

std::unique_ptr<::ycsbc::DB> BamContractDB::CreateDB(utils::Properties &props) {
  if (props["dbname"] == "xp") {
    return std::make_unique<ContractDBImpl>(
        ExecutorType::XP, stoull(props["worker_num"], NULL, 10),
        stoull(props["batch_size"], NULL, 10));
  }
  if (props["dbname"] == "2pl") {
    return std::make_unique<ContractDBImpl>(
        ExecutorType::TPL, stoull(props["worker_num"], NULL, 10),
        stoull(props["batch_size"], NULL, 10));
  } else {
    return std::make_unique<ContractDBImpl>(
        ExecutorType::BAMBOO, stoull(props["worker_num"], NULL, 10),
        stoull(props["batch_size"], NULL, 10));
  }
}

}  // namespace benchmark
}  // namespace contract
}  // namespace resdb

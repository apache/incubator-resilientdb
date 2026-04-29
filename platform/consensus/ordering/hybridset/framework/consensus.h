#pragma once

#include "executor/common/transaction_manager.h"
#include "platform/consensus/execution/transaction_executor.h"
#include "platform/consensus/ordering/common/framework/consensus.h"
#include "platform/consensus/ordering/hybridset/algorithm/hybridset.h"
#include "platform/consensus/ordering/hybridset/framework/performance_manager.h"
#include "platform/networkstrate/consensus_manager.h"
#include "enclave/sgx_cpp_u.h"

namespace resdb {
namespace hybridset {

class HybridSetConsensus : public common::Consensus {
 public:
  HybridSetConsensus(const ResDBConfig& config,
                     std::unique_ptr<TransactionManager> transaction_manager,
                     oe_enclave_t* enclave);

 protected:
  int ProcessCustomConsensus(std::unique_ptr<Request> request) override;
  int ProcessNewTransaction(std::unique_ptr<Request> request) override;
  int CommitMsg(const google::protobuf::Message& msg) override;
  int CommitMsgInternal(const Transaction& txn);

 private:
  std::unique_ptr<HybridSetPerformanceManager> GetPerformanceManager();

  std::unique_ptr<HybridSet> hybridset_;
};

}  // namespace hybridset
}  // namespace resdb

#pragma once

#include "executor/common/transaction_manager.h"
#include "platform/consensus/execution/transaction_executor.h"
#include "platform/consensus/ordering/common/framework/consensus.h"
#include "platform/consensus/ordering/achilles/algorithm/achilles.h"
#include "platform/networkstrate/consensus_manager.h"
#include "enclave/sgx_cpp_u.h"

namespace resdb {
namespace achilles {

class AchillesConsensus : public common::Consensus {
 public:
  AchillesConsensus(const ResDBConfig& config,
                    std::unique_ptr<TransactionManager> transaction_manager,
                    oe_enclave_t* enclave);

 protected:
  int ProcessCustomConsensus(std::unique_ptr<Request> request) override;
  int ProcessNewTransaction(std::unique_ptr<Request> request) override;
  int CommitMsg(const google::protobuf::Message& msg) override;
  int CommitMsgInternal(const Transaction& txn);

 private:
  std::unique_ptr<Achilles> achilles_;
};

}  // namespace achilles
}  // namespace resdb

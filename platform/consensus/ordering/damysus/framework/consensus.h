#pragma once

#include "executor/common/transaction_manager.h"
#include "platform/consensus/execution/transaction_executor.h"
#include "platform/consensus/ordering/common/framework/consensus.h"
#include "platform/consensus/ordering/damysus/algorithm/damysus.h"
#include "platform/networkstrate/consensus_manager.h"
#include "enclave/sgx_cpp_u.h"

namespace resdb {
namespace damysus {

class DamysusConsensus : public common::Consensus {
 public:
  DamysusConsensus(const ResDBConfig& config,
                   std::unique_ptr<TransactionManager> transaction_manager,
                   oe_enclave_t* enclave);

 protected:
  int ProcessCustomConsensus(std::unique_ptr<Request> request) override;
  int ProcessNewTransaction(std::unique_ptr<Request> request) override;
  int CommitMsg(const google::protobuf::Message& msg) override;
  int CommitMsgInternal(const Transaction& txn);

 private:
  std::unique_ptr<Damysus> damysus_;
};

}  // namespace damysus
}  // namespace resdb

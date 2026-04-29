#pragma once

#include "executor/common/transaction_manager.h"
#include "platform/consensus/execution/transaction_executor.h"
#include "platform/consensus/ordering/common/framework/consensus.h"
#include "platform/consensus/ordering/bullshark/algorithm/bullshark.h"
#include "platform/networkstrate/consensus_manager.h"

namespace resdb {
namespace bullshark {

class BullSharkConsensus : public common::Consensus {
 public:
  BullSharkConsensus(const ResDBConfig& config,
                     std::unique_ptr<TransactionManager> transaction_manager);

 protected:
  int ProcessCustomConsensus(std::unique_ptr<Request> request) override;
  int ProcessNewTransaction(std::unique_ptr<Request> request) override;
  int CommitMsg(const google::protobuf::Message& msg) override;
  int CommitMsgInternal(const Transaction& txn);

 private:
  std::unique_ptr<BullShark> bullshark_;
};

}  // namespace bullshark
}  // namespace resdb

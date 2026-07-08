#pragma once

#include <memory>
#include <mutex>

#include "executor/common/transaction_manager.h"
#include "platform/consensus/execution/transaction_executor.h"
#include "platform/consensus/ordering/common/framework/consensus.h"
#include "platform/consensus/ordering/hotstuff_1/algorithm/hotstuff.h"
#include "platform/networkstrate/consensus_manager.h"

namespace resdb {
namespace hotstuff_1 {

class Consensus : public common::Consensus {
 public:
  Consensus(const ResDBConfig& config,
            std::unique_ptr<TransactionManager> transaction_manager);
  ~Consensus() override = default;

 private:
  int ProcessCustomConsensus(std::unique_ptr<Request> request) override;
  int ProcessNewTransaction(std::unique_ptr<Request> request) override;
  int CommitMsg(const google::protobuf::Message& msg) override;
  int CommitMsgInternal(const Transaction& txn);

  std::unique_ptr<HotStuff> hotstuff_;
  Stats* global_stats_ = nullptr;
  int f_ = 0;
  int id_ = 0;
};

}  // namespace hotstuff_1
}  // namespace resdb

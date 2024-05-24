#pragma once

#include <deque>
#include <map>
#include <queue>
#include <thread>

#include "platform/statistic/stats.h"
#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/fairdag_rl/algorithm/tusk.h"
#include "platform/consensus/ordering/fairdag_rl/algorithm/graph.h"
#include "platform/consensus/ordering/fairdag_rl/algorithm/proposal_manager.h"
#include "platform/consensus/ordering/fairdag_rl/proto/proposal.pb.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"

namespace resdb {
namespace fairdag_rl {

class FairDAG : public common::ProtocolBase {
 public:
  FairDAG(int id, int f, int total_num, SignatureVerifier* verifier);
  ~FairDAG();

  //  recv txn -> send block with links -> rec block ack -> send block with certs
  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  bool ReceiveBlock(std::unique_ptr<Proposal> proposal);
  void ReceiveBlockACK(std::unique_ptr<Metadata> metadata);
  void ReceiveBlockCert(std::unique_ptr<Certificate> cert);

private:
  void CommitTxns(std::vector<std::unique_ptr<Transaction> >& txns);
  bool IsCommitted(const Transaction& txn);
  void AddTxnData(std::unique_ptr<Transaction> txn);

 private:
  std::mutex mutex_;
  std::unique_ptr<Tusk> tusk_;
  std::unique_ptr<Graph> graph_;
  int64_t local_time_ = 0;
  int execute_id_;
  int replica_num_;
  Stats* global_stats_;
  std::set<std::pair<int, int> > committed_;
  std::map<std::pair<int, int>, std::unique_ptr<Transaction> > data_;
};

}  // namespace tusk
}  // namespace resdb

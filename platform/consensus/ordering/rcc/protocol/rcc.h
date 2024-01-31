#pragma once

#include <deque>
#include <map>
#include <queue>
#include <thread>

#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/common/protocol/protocol_base.h"
#include "platform/consensus/ordering/rcc/proto/proposal.pb.h"
#include "platform/consensus/ordering/rcc/protocol/proposal_manager.h"
#include "platform/consensus/ordering/rcc/protocol/transaction_collector.h"

namespace resdb {
namespace rcc {
class RCC : protocol::ProtocolBase {
 public:
  RCC(int id, int f, int total_num,
      protocol::ProtocolBase::SingleCallFuncType single_call,
      protocol::ProtocolBase::BroadcastCallFuncType broadcast_call,
      protocol::ProtocolBase::CommitFuncType commit);
  ~RCC();

  void AsyncSend();
  void AsyncCommit();
  void CommitProposal(std::unique_ptr<Proposal> p);
  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  void SendTxn();
  bool ReceiveProposal(const Proposal& proposal);

 private:
  void UpgradeState(Proposal* proposal);

 private:
  std::atomic<int> local_txn_id_;
  LockFreeQueue<Proposal> commit_queue_, execute_queue_;
  LockFreeQueue<Transaction> txns_;

  std::thread send_thread_;
  std::thread commit_thread_;

  std::unique_ptr<ProposalManager> proposal_manager_;
  std::mutex mutex_[1024], txn_mutex_;

  std::map<int, std::map<int, std::unique_ptr<Proposal>>> seq_set_;
  std::map<int, std::map<int, std::set<int>>> received_num_[1024];
  int next_seq_;
  int totoal_proposer_num_;
  std::map<int, std::unique_ptr<Proposal>> received_data_[1024];
  std::set<int> is_commit_[1024];
  std::map<std::string, std::unique_ptr<TransactionCollector>> collector_[1024];
  int execute_id_ = 1;
  int batch_size_;
};

}  // namespace rcc
}  // namespace resdb

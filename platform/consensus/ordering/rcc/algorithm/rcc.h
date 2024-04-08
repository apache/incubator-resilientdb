#pragma once

#include <deque>
#include <map>
#include <queue>
#include <thread>

#include "platform/statistic/stats.h"
#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "platform/consensus/ordering/rcc/proto/proposal.pb.h"
#include "platform/consensus/ordering/rcc/algorithm/proposal_manager.h"
#include "platform/consensus/ordering/rcc/algorithm/transaction_collector.h"

namespace resdb {
namespace rcc {

class RCC : public common::ProtocolBase {
 public:
  RCC(int id, int f, int total_num, SignatureVerifier* verifier);
  ~RCC();

  void AsyncSend();
  void AsyncCommit();
  void CommitProposal(std::unique_ptr<Proposal> p);
  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  void SendTxn();
  bool ReceiveProposal(const Proposal& proposal);
  bool ReceiveProposalList(const Proposal& proposal);

 private:
  void UpgradeState(Proposal* proposal);

 private:
  std::atomic<int> local_txn_id_;
  LockFreeQueue<Proposal> commit_queue_, execute_queue_;
  LockFreeQueue<Transaction> txns_;
  SignatureVerifier* verifier_;

  std::thread send_thread_;
  std::thread commit_thread_;

  std::unique_ptr<ProposalManager> proposal_manager_;
  std::mutex mutex_[1024], txn_mutex_, seq_mutex_;
  std::condition_variable vote_cv_;

  std::map<int, std::map<int, std::unique_ptr<Proposal>>> seq_set_;
  std::map<int, std::map<int, std::set<int>>> received_num_[1024];
  int next_seq_;
  int totoal_proposer_num_;
  std::map<int, std::unique_ptr<Proposal>> received_data_[1024];
  std::set<int> is_commit_[1024];
  std::map<std::string, std::unique_ptr<TransactionCollector>> collector_[1024];
  int execute_id_ = 1;
  int batch_size_;
  int64_t commited_seq_ = 0;
  std::atomic<int> queue_size_;
  Stats* global_stats_;
  int64_t last_commit_time_ = 0;
  std::map<int,int64_t> commit_time_;
  std::map<int64_t, int> send_num_[10];
  std::map<int64_t, std::vector<std::unique_ptr<Proposal>> > pending_msg_[10];
};

}  // namespace rcc
}  // namespace resdb

#pragma once

#include <deque>
#include <map>
#include <queue>
#include <thread>

#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "platform/consensus/ordering/cassandra_cft/algorithm/proposal_graph.h"
#include "platform/consensus/ordering/cassandra_cft/algorithm/proposal_manager.h"
#include "platform/consensus/ordering/cassandra_cft/proto/proposal.pb.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace cassandra_cft {

class Cassandra: public common::ProtocolBase {
 public:
  Cassandra(int id, int f, int total_num, bool failure_mode);
  ~Cassandra();

  bool ReceiveProposal(std::unique_ptr<Proposal> proposal);

  void SetPrepareFunction(std::function<int(const Transaction&)> prepare);
  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);

void BroadcastTxn();
 private:
  bool IsStop();

  int SendTxn(int round);

void Notify(int round);
void CommitProposal(std::unique_ptr<Proposal> proposal);
void AsyncCommit();


void SetSBCSendReady(int round);
bool SBCSendReady(int round);
void RecvSBCSendReady(int round);
bool SBCRecvReady(int round);
void NotifyRecvReady();

void SBC();
void SetSendNext(int round);
bool CanSendNext(int round);


 private:
  std::unique_ptr<ProposalGraph> graph_;
  LockFreeQueue<Transaction> txns_;
  std::unique_ptr<ProposalManager> proposal_manager_;
  SignatureVerifier* verifier_;
  std::mutex mutex_, g_mutex_;
  std::map<int, std::set<int>> received_num_;
  // int state_;
  int id_, total_num_, f_, batch_size_;
  bool failure_mode_ = false;
  std::atomic<int> is_stop_;
  int timeout_ms_;
  int local_txn_id_, local_proposal_id_;
  LockFreeQueue<Proposal> commit_q_, execute_queue_, prepare_queue_;
  std::thread commit_thread_, consensus_thread_, block_thread_, prepare_thread_, sbc_thread_;
  std::condition_variable vote_cv_;
  std::map<int, bool> can_vote_;
  std::atomic<int> committed_num_;
  int voting_, start_ = false;
  std::map<int, std::vector<std::unique_ptr<Transaction>>> uncommitted_txn_;

  bool use_linear_ = false;
  int recv_num_ = 0;
  int execute_num_ = 0;
  int pending_num_ = 0;
  std::atomic<int> executed_;
  std::atomic<int> precommitted_num_;
  int last_vote_ = 0;
  int execute_id_;

  std::mutex block_mutex_;
  std::set<int> received_;
  std::map<int, std::set<int>> block_ack_;
  std::map<int, std::vector<std::unique_ptr<Proposal>>> future_proposal_;
  std::set<std::pair<int,int> > committed_;

  std::function<int(const Transaction&)> prepare_;

  int current_round_;

  std::mutex send_ready_mutex_, recv_ready_mutex_;
  std::condition_variable send_ready_cv_, recv_ready_cv_;
  int has_sent_ = 0;
  int sent_next_ = 1;

  Stats* global_stats_;

};

}  // namespace cassandra_cft
}  // namespace resdb

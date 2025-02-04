#pragma once

#include <deque>
#include <map>
#include <queue>
#include <thread>

#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/cassandra/algorithm/proposal_graph.h"
#include "platform/consensus/ordering/cassandra/algorithm/proposal_manager.h"
#include "platform/consensus/ordering/cassandra/proto/proposal.pb.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace cassandra {
namespace cassandra_recv {

class Cassandra : public common::ProtocolBase {
 public:
  Cassandra(int id, int f, int total_num, SignatureVerifier* verifier);
  ~Cassandra();

  void CheckBlock(const std::string& hash, int local_id);
  void ReceiveBlock(std::unique_ptr<Block> block);
  void ReceiveBlockACK(std::unique_ptr<BlockACK> block);
  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  bool ReceiveProposal(std::unique_ptr<Proposal> proposal);
  bool ReceiveVote(const VoteMessage& msg);
  bool ReceivePrepare(const VoteMessage& msg);
  int ReceiveRecovery(const CommittedProposals& proposals);

  bool ReceiveVoteACK(const VoteMessage& msg);
  bool ReceiveCommit(const VoteMessage& msg);

  void AskBlock(const Block& block);
  void SendBlock(const BlockQuery& block);

  void AskProposal(const Proposal& proposal);
  void SendProposal(const ProposalQuery& query);
  void ReceiveProposalQueryResp(const ProposalQueryResp& resp);
  void PrepareProposal(const Proposal& p);

  void SetPrepareFunction(std::function<int(const Transaction&)> prepare);

 private:
  bool IsStop();

  int SendTxn(int round);

  void Commit(const VoteMessage& msg);
  void CommitProposal(const Proposal& p);

  void AsyncConsensus();
  void AsyncCommit();
  void AsyncPrepare();
  bool WaitVote(int);
  void WaitCommit();

  bool CheckHistory(const Proposal& proposal);

  void Reset();
  bool CheckState(MessageType type, ProposalState state);

  void TrySendRecoveery(const Proposal& proposal);
  void BroadcastTxn();
  bool AddProposal(const Proposal& proposal);

  bool ProcessProposal(std::unique_ptr<Proposal> proposal);

 private:
  std::unique_ptr<ProposalGraph> graph_;
  LockFreeQueue<Transaction> txns_;
  std::unique_ptr<ProposalManager> proposal_manager_;
  SignatureVerifier* verifier_;
  std::mutex mutex_, g_mutex_;
  std::map<int, std::set<int>> received_num_;
  // int state_;
  int id_, total_num_, f_, batch_size_;
  std::atomic<int> is_stop_;
  int timeout_ms_;
  int local_txn_id_, local_proposal_id_;
  LockFreeQueue<Proposal> commit_queue_, execute_queue_, prepare_queue_;
  std::thread commit_thread_, consensus_thread_, block_thread_, prepare_thread_;
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

  std::function<int(const Transaction&)> prepare_;
  int current_round_;

  Stats* global_stats_;
};

}  // namespace cassandra_recv
}  // namespace cassandra
}  // namespace resdb

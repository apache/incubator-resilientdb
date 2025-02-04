#pragma once

#include <deque>
#include <map>
#include <queue>
#include <thread>

#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/cassandra/algorithm/basic/proposal_graph.h"
#include "platform/consensus/ordering/cassandra/algorithm/basic/proposal_manager.h"
#include "platform/consensus/ordering/cassandra/proto/proposal.pb.h"

namespace resdb {
namespace cassandra {
namespace basic {

class Cassandra {
 public:
  Cassandra(
      int id, int batch_size, int total_num, int f,
      std::function<int(int, google::protobuf::Message& msg, int)> single_call,
      std::function<int(int, google::protobuf::Message& msg)> broadcast_call,
      std::function<int(const Transaction& txn)> commit);
  ~Cassandra();

  void ReceiveBlock(std::unique_ptr<Block> block) {}

  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  bool ReceiveProposal(const Proposal& proposal);
  bool ReceiveVote(const VoteMessage& msg);
  bool ReceivePrepare(const VoteMessage& msg);
  int ReceiveRecovery(const CommittedProposals& proposals);

  bool ReceiveVoteACK(const VoteMessage& msg);
  bool ReceiveCommit(const VoteMessage& msg);

 private:
  int SendTxn();

  void Commit(const VoteMessage& msg);
  void CommitProposal(const Proposal& p);

  void AsyncConsensus();
  void AsyncCommit();
  bool WaitVote(int);
  void WaitCommit();

  void Reset();
  bool CheckState(MessageType type, ProposalState state);

  void TrySendRecoveery(const Proposal& proposal);

 private:
  std::unique_ptr<ProposalGraph> graph_;
  std::deque<std::unique_ptr<Transaction>> txns_;
  std::unique_ptr<ProposalManager> proposal_manager_;
  std::function<int(int, google::protobuf::Message& msg)> broadcast_call_;
  std::function<int(int, google::protobuf::Message& msg, int)> single_call_;
  std::function<int(const Transaction&)> commit_;
  std::mutex mutex_, g_mutex_;
  std::map<int, std::set<int>> received_num_;
  // int state_;
  int id_, total_num_, f_, batch_size_;
  int is_stop_;
  int timeout_ms_;
  int local_txn_id_, local_proposal_id_;
  LockFreeQueue<Proposal> commit_queue_, execute_queue_;
  std::thread commit_thread_, consensus_thread_;
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
};

}  // namespace basic
}  // namespace cassandra
}  // namespace resdb

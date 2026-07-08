#pragma once

#include <deque>
#include <condition_variable>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "platform/consensus/ordering/cassandra/algorithm/proposal_graph.h"
#include "platform/consensus/ordering/cassandra/algorithm/proposal_manager.h"
#include "platform/consensus/ordering/cassandra/proto/proposal.pb.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace cassandra {
namespace cassandra_recv {

class Cassandra: public common::ProtocolBase {
 public:
  Cassandra(int id, int f, int total_num, int block_size, SignatureVerifier* verifier);
  ~Cassandra();

  void CheckBlock(const std::string& hash, int local_id);
  void ReceiveBlock(std::unique_ptr<Block> block);
  void ReceiveBlockACK(std::unique_ptr<BlockACK> block);

  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  bool ReceiveProposal(std::unique_ptr<Proposal> proposal);
  bool ReceiveVote(const VoteMessage& msg);
  bool ReceivePrepare(const VoteMessage& msg);
  bool ReceiveProposalVote(std::unique_ptr<Proposal> proposal);
  int ReceiveRecovery(const CommittedProposals& proposals);

  bool ReceiveVoteACK(const VoteMessage& msg);
  bool ReceiveCommit(const VoteMessage& msg);

  void SetPrepareFunction(std::function<int(const Transaction&)> prepare);

  void ReceiveAskBlock(std::unique_ptr<BlockQuery> block);
  void AskBlock(int sender , int block_id );
  void ReceiveAskBlockAck(std::unique_ptr<Block> block);

  void SyncRound();
  void ReceiveRound(std::unique_ptr<BlockQuery> block);
  void ReceiveRoundAck(std::unique_ptr<BlockQuery> block);
  void ReceiveAskProposal(std::unique_ptr<ProposalQuery> query);
  void ReceiveAskProposalAck(std::unique_ptr<ProposalQueryResp> query);


 private:
  bool IsStop();

  int SendTxn(int round);

  void Commit(const VoteMessage& msg);
  void CommitProposal(const Proposal& p);

  void AsyncConsensus();
  void AsyncCommit();
  void AsyncPrepare();
  void MonitorProposalRound();
  bool SlowPath(int round);
  bool WaitVote(int);
  void WaitCommit();
  void StartRecovery(int target_round, const std::string& reason);
  void FinishRecoveryIfReady(const std::string& source);
  void RetryMissingParents();

  bool CheckHistory(const Proposal& proposal);

  void Reset();
  bool CheckState(MessageType type, ProposalState state);

  void TrySendRecoveery(const Proposal& proposal);
  void BroadcastTxn();
  bool AddProposal(const Proposal& proposal);

  bool ProcessProposal(std::unique_ptr<Proposal> proposal);

  bool Checklimit(int low, int hight, int proposer);

  int NextLeader(int round) ;
  bool IsLeader(int round) ;
  bool IsLeader(int round,int id) ;
  bool AddNewProposal(std::unique_ptr<Proposal> proposal);
void AskProposal(int sender, int proposal_id, const std::string& hash);


  void AskProposal(int round);


bool AddNewProposalInternal(std::unique_ptr<Proposal> proposal) ;


 private:
  std::unique_ptr<ProposalGraph> graph_;
  LockFreeQueue<Transaction> txns_;
  std::unique_ptr<ProposalManager> proposal_manager_;
  SignatureVerifier* verifier_;
  std::mutex mutex_, g_mutex_, round_mutex_, b_round_mutex_, recv_mutex_, gg_mutex_;
  // Dedicated mutex for notfound_ map (proposals waiting for a missing
  // parent). Without it, three different threads (cassandra::AddProposal,
  // ReceiveAskProposalAck, RetryMissingParents) raced on this map and
  // produced segfaults during the early pre-partition window of 32-node
  // clusters. Lock order: notfound_mutex_ is innermost; it is taken either
  // alone (recovery threads) or while already holding g_mutex_
  // (cassandra::AddProposal). It must NEVER be taken before g_mutex_.
  std::mutex notfound_mutex_;
  std::map<int, std::set<int>> received_num_, slow_received_num_;
  // Once we've forwarded the slow-path quorum vote for a given height to the
  // next leader, do NOT re-forward it on every subsequent proposal arrival
  // for that height. Without this guard ReceiveProposal()'s else-branch
  // re-runs GetStrongestProposal + SendMessage on every later proposal at
  // the same height, which (a) causes thousands of redundant network sends
  // per second at 32 nodes and (b) re-walks last_node_[height] (which can
  // grow up to total_num_ entries) under g_mutex_, contributing to the
  // log-flood / SIGKILL pattern observed.
  std::set<int> slow_sent_height_;

  // Rate-limit state for FinishRecoveryIfReady() recovery_progress diag.
  int last_recovery_log_target_ = -1;
  int last_recovery_log_qsize_ = -1;

  std::map<int, std::map<std::pair<int, std::string>, std::set<int>>> vote_;

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
  std::mutex proposal_timer_mutex_;
  std::condition_variable proposal_timer_cv_;
  std::thread proposal_timer_thread_;
  std::map<int, int> proposal_round_received_;
  int monitored_round_ = 1;

  Stats* global_stats_;
  int need_num_;
  std::map<int, std::queue<std::unique_ptr<Proposal>> > pending_, future_;
  std::map<int, std::set<int> > received_block_;
  std::map<int, int> txn_commit_;
  std::map<int, std::map<int, std::unique_ptr<BlockQuery> > > receive_round_;
  std::map<int, std::map<int, std::unique_ptr<ProposalQueryResp> > > receive_round_p_;
  std::map<int, int64_t> ask_p_;
  std::map<std::pair<int, int>, int64_t> ask_missing_p_;
  std::map<std::pair<int, int>, int64_t> ask_chain_p_;
  std::mutex ask_chain_mutex_;
  std::map<std::pair<int, int>, int64_t> recent_chain_ack_;
  std::mutex chain_ack_mutex_;
  std::set<std::pair<int,int> > send_block_;
  std::map<int, std::vector<std::unique_ptr<Proposal>> > pending_p_;
  int upgrade_ = 0;
  int recv_ = false;
  int last_add_proposal_ret_ = 0;
  std::map<std::pair<int,int>, std::vector<std::unique_ptr<Proposal>>> notfound_;
  // After a partition is detected (recv_>0) we collect distinct senders that
  // have acknowledged a max_round >= recv_. Recovery is only declared
  // complete when 2f+1 distinct senders agree we have caught up to that
  // height, ensuring that the local graph reflects a quorum view before we
  // resume making progress (avoids picking divergent strongest proposals).
  //
  // Stores per-sender highest reported max_round (NOT just a set of sender
  // ids). When the recovery target rises during catch-up, we keep historical
  // acks instead of clearing them, and recompute the effective quorum size
  // against the current target on demand. Without this, a fast-rising target
  // (which is common right after partition heal because every replica is
  // racing to catch up) would clear the quorum every few milliseconds and
  // recovery would never converge.
  std::map<int, int> recovery_quorum_senders_;
};

}  // namespace cassandra_recv
}  // namespace cassandra
}  // namespace resdb

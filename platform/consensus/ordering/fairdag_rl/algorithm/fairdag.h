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
  std::vector<std::pair<uint64_t, uint64_t>> FinalOrder(Graph* g);
  void ProcessCommitedLO(const std::vector<std::unique_ptr<Transaction> >& txns);
  void ConstructDependencyGraph(Graph * g, const std::vector<Transaction*>& txns);

  // void AddTxnData(std::unique_ptr<Transaction> txn);
  void AddLocalOrdering(Graph * g_ptr, const std::vector<Transaction*>& txns, std::set<std::pair<uint64_t, uint64_t>> &possible_edges);
  void AddEdges(std::set<std::pair<uint64_t, uint64_t>> &possible_edges);

  void CheckGraph();

  bool IsCommitted(const Transaction& txn);
  bool IsCommitted(int proxy_id, uint64_t user_seq);
  void SetCommitted(const Transaction& txn);
  void SetCommitted(int proxy_id, uint64_t user_seq);

  // bool IsSolid(const Transaction& txn);
  // bool IsSolid(int proxy_id, int user_seq);
  // void SetSolid(uint64_t round, int proxy_id, int user_seq);
  // void SetRoundSolid(uint64_t round, int proxy_id, int user_seq);
  bool IsSolid(uint64_t id);
  void SetSolid(uint64_t round, uint64_t id);
  void SetRoundSolid(uint64_t round, uint64_t id);

  void FindPossibleEdges(Graph* g, uint64_t id1, std::set<std::pair<uint64_t,uint64_t>>& possible_edges, std::vector<uint64_t>& ids);

  void Push_Committed_LO(std::unique_ptr<std::vector<std::unique_ptr<Transaction>>> local_orderings);

  void AsyncProcessCommittedLO();

  std::set<uint64_t> set_difference(const std::set<uint64_t>& A, const std::set<uint64_t>& B);
  void AddImplictWeight(Graph * g_ptr, std::set<std::pair<uint64_t,uint64_t>> &possible_edges);

 private:
  std::thread process_comitted_lo_thread_;
  std::mutex mutex_, lo_mutex_, cloq_mutex_;
  std::unique_ptr<Tusk> tusk_;
  std::unique_ptr<Graph> graph_;
  int64_t local_time_ = 0;
  int execute_id_;
  int replica_num_;
  Stats* global_stats_;
  std::set<std::pair<int, int> > committed_;
  // std::map<std::pair<int, int>, uint64_t> solid_; 
  std::map<uint64_t, uint64_t> solid_; 
  // std::map<uint64_t, std::set<std::pair<int, int>>> round_solid_;
  std::map<uint64_t, std::set<uint64_t>> round_solid_;
  // std::map<std::pair<int, int>, std::unique_ptr<Transaction> > data_; // USELESS
  // std::map<std::pair<int,int>, std::map<int, std::set<int>>> txn_key_proposers_;
  std::map<uint64_t, std::map<int, std::set<int>>> txn_id_proposers_;
  // std::map<std::pair<int,int64_t>,int> txn_key_2_idx_;
  std::map<std::pair<uint64_t,uint64_t>,uint64_t> txn_key_2_idx_;
  uint64_t idx_ = 0;
  std::map<int, std::set<uint64_t> > pending_txn_ids_;
  std::set<uint64_t> all_pending_txn_ids_;
  // std::map<std::pair<int,int>, std::string> txn_hash_;
  std::map<uint64_t, std::string> txn_hash_;
  std::map<uint64_t, std::pair<uint64_t,uint64_t> > proposals_idx_2_key_;
  std::queue<std::unique_ptr<Graph> > g_queue_;
  std::set<Graph*> g_set_;
  //[DK] [ISSUE] the values of id_g_ should be a vector of Graph* rather than one Graph*
  std::map<uint64_t, Graph*> id_2_first_g_;
  // std::map< std::pair<std::pair<int,int>, std::pair<int,int> > , int > weight_;
  std::map< std::pair<uint64_t, uint64_t>, std::set<uint64_t> > weight_;
  // std::map< std::pair<int,int>, std::set<std::pair<int,int>> > compared_node_keys_;
  std::map<uint64_t, std::set<uint64_t>> compared_txn_ids_;
  int max_round_ = 0;
  std::map<int, Graph*> local_g_;

  uint64_t shaded_threshold_, solid_threshold_;
  // std::map<std::pair<int, uint64_t>, uint64_t> commit_time_;
  std::map<uint64_t, uint64_t> commit_time_;

  std::vector<uint64_t> txn_to_add_ids_;
  LockFreeQueue<std::vector<std::unique_ptr<Transaction>>> committed_lo_queue_;

  std::unique_ptr<OrderManager> order_manager_;
};

}  // namespace tusk
}  // namespace resdb

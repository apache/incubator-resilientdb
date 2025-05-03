#pragma once

#include <deque>
#include <map>
#include <queue>
#include <thread>

#include "platform/statistic/stats.h"
#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/hs_rl/algorithm/hs.h"
#include "platform/consensus/ordering/hs_rl/algorithm/graph.h"
#include "platform/consensus/ordering/hs_rl/algorithm/proposal_manager.h"
#include "platform/consensus/ordering/hs_rl/proto/proposal.pb.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"

namespace resdb {
namespace hs_rl {

class Themis : public common::ProtocolBase {
 public:
  Themis(int id, int f, int total_num, SignatureVerifier* verifier);
  ~Themis();

  //  recv txn -> send block with links -> rec block ack -> send block with certs
  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  bool ReceiveLocalOrdering(std::unique_ptr<Proposal> proposal);
  bool ReceiveProposal(std::unique_ptr<Proposal> proposal);
  bool ReceiveCertificate(std::unique_ptr<Certificate> cert);

 private:
  std::vector<int> FinalOrder(Graph* g);
  void ProcessCausalHistory(const std::vector<Transaction* >& txns);
  void ConstructDependencyGraph(Graph * g, const std::vector<Transaction*>& txns);

  void AddTxnData(std::unique_ptr<Transaction> txn);
  void AddLocalOrdering(Graph * g_ptr, const std::vector<Transaction*>& txns, std::set<std::pair<int, int>> &possible_edges);
  void AddEdges(std::set<std::pair<int, int>> &possible_edges);

  void CheckGraph();

  bool IsCommitted(const Transaction& txn);
  bool IsCommitted(int proxy_id, int user_seq);
  void SetCommitted(const Transaction& txn);
  void SetCommitted(int proxy_id, int user_seq);

  bool IsSolid(const Transaction& txn);
  bool IsSolid(int proxy_id, int user_seq);
  void SetSolid(uint64_t round, int proxy_id, int user_seq);
  void SetRoundSolid(uint64_t round, int proxy_id, int user_seq);

  bool EdgeNeeded(int id1, int id2);
  std::vector<Graph*> CommonGraphs(std::pair<int,int> id_pair);

 private:
  std::mutex mutex_;
  std::unique_ptr<HotStuff> hs_;
  std::unique_ptr<Graph> graph_;
  int64_t local_time_ = 0;
  int execute_id_;
  int replica_num_;
  Stats* global_stats_;
  std::set<std::pair<int, int> > committed_;
  std::map<std::pair<int, int>, uint64_t> solid_; 
  std::map<uint64_t, std::set<std::pair<int, int>>> round_solid_;
  std::map<std::pair<int, int>, std::unique_ptr<Transaction> > data_;
  std::map<std::pair<int,int>, std::map<int, std::set<int>>> txn_key_proposers_;
  // std::map<std::pair<int,int>, std::set<int>> txn_key_proposers_;
  std::map<std::pair<int,int64_t>,int> txn_key_2_idx_;
  int idx_ = 0;
  std::map<std::pair<int,int>, std::set<int>> edge_counts_;
  std::map<int, std::set<int> > pending_proposals_;
  std::map<std::pair<int,int>, std::string> proposals_data_;
  std::map<int, std::pair<int,int> > proposals_idx_2_key_;
  std::queue<std::unique_ptr<Graph> > g_queue_;
  std::set<Graph*> g_set_;
  //[DK] [ISSUE] the values of id_g_ should be a vector of Graph* rather than one Graph*
  std::map<int, Graph*> id_2_first_g_;
  std::map< std::pair<std::pair<int,int>, std::pair<int,int> > , int > weight_;
  std::map< std::pair<std::pair<int,int>, std::pair<int,int> > , std::set<Graph*> > e2g_;
  std::map< std::pair<int,int>, std::set<std::pair<int,int>> > compared_node_keys_;
  int max_round_ = 0;
  std::map<int, Graph*> local_g_;

  size_t shaded_threshold_, solid_threshold_;

};

}  // namespace tusk
}  // namespace resdb

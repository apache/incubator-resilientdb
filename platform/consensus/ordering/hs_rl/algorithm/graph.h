#pragma once

#include <map>
#include <stack>
#include "glog/logging.h"
#include "platform/consensus/ordering/hs_rl/proto/proposal.pb.h"

namespace resdb {
namespace hs_rl {

class Graph {
public:
  Graph() { refered_ = 0; }
  void ResetEdge();
  void Inc(int num) { refered_+=num; }
  bool Ready(int count) { 
  //LOG(ERROR)<<" check ready refer:"<<refered_<<" g:"<<this; 
  return refered_ >= count; }
  void SetRound(int round) { round_ = round; }
  int Round(){ return round_; }
  void AddEdge(int a, const int b);
  void RemoveTxn(int a);

  std::vector<int> GetOrder(const std::vector<int>& commit_txns);
  std::vector<int> GetOrder();
  int Size(){ return g_.size() - del_.size(); }

  void AddNode(int  a);
  void ClearGraph();
  bool IsTournament();
  void Remove(int u);
  void RemoveNodesEdges(std::set<int> txn_ids);

private:
  int Node2ID(int a);
  int ID2Node(int id);
  void CheckGraph();
  void Clear();
  void Dfs(int u) ;
  void Order();
  int GetID(const std::string & hash);
  int GetTxnID(const Transaction& txn);

  private:
  std::map<int, std::set<int> > g_, preg_;
  std::map<int, std::vector<int> > og_;
  std::map<std::string, int> hash2idx_;
  int idx_ = 0;

  std::vector<int>low_, dfn_,visit_, belong_, d_, result_, v_;
  std::stack<int>stk_;
  int tot_ = 0;
  int scc_ = 0;
  std::vector<std::set<int> > ogs_;
  int ver_ = 1;
  int edge_num_ = 0;
  std::map<int,int> node_, id2node_;
  std::set<int> del_;
  int round_;
  int refered_ = 0;
};

}  // namespace hs_rl
}  // namespace resdb

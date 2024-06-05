#pragma once

#include <map>
#include <stack>
#include "platform/consensus/ordering/fairdag_rl/proto/proposal.pb.h"

namespace resdb {
namespace fairdag_rl {

class Graph {
public:
  void AddTxn(const Transaction& a, const Transaction& b);
  void AddTxn(int a, const int b);
  void RemoveTxn(const std::string& hash);
  void RemoveTxn(const Transaction& txn);
  void RemoveTxn(int a);

  std::vector<int> GetOrder(const std::vector<int>& commit_txns);
  int Size(){ return g_.size(); }

  private:
  void CheckGraph();
  void Clear();
  void Dfs(int u) ;
  void Order();
  int GetID(const std::string & hash);
  int GetTxnID(const Transaction& txn);

  private:
  std::map<int, std::set<int> > g_, preg_;
  std::map<int, std::vector<int> > og_;
  //std::map<std::string, std::set<std::string> > g_, preg_;
  std::map<std::string, int> hash2idx_;
  int idx_ = 1;

  std::vector<int>low_, dfn_,visit_, belong_, d_, result_;
  std::stack<int>stk_;
  int tot_ = 0;
  int scc_ = 0;
  std::vector<std::set<int> > ogs_;
  std::map<std::pair<int,int64_t>, int> key2idx_;
};

}  // namespace fairdag
}  // namespace resdb

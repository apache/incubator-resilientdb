#pragma once

#include <map>
#include <stack>

#include "platform/consensus/ordering/fairdag_rl/proto/proposal.pb.h"

namespace resdb {
namespace fairdag_rl {

class Graph {
 public:
  void AddTxn(int a, const int b);
  void RemoveTxn(int a);

  std::vector<int> GetOrder(const std::vector<int>& commit_txns);
  int Size() { return g_.size(); }

 private:
  void CheckGraph();
  void Clear();
  void Dfs(int u);
  void Order();
  int GetID(const std::string& hash);
  int GetTxnID(const Transaction& txn);

 private:
  std::map<int, std::set<int> > g_, preg_;
  std::map<int, std::vector<int> > og_;
  std::map<std::string, int> hash2idx_;
  int idx_ = 1;

  std::vector<int> low_, dfn_, visit_, belong_, d_, result_, v_;
  std::stack<int> stk_;
  int tot_ = 0;
  int scc_ = 0;
  std::vector<std::set<int> > ogs_;
  int ver_ = 1;
};

}  // namespace fairdag_rl
}  // namespace resdb

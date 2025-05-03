#pragma once

#include <map>
#include <stack>
#include "glog/logging.h"
#include "platform/consensus/ordering/pbft_rl/proto/proposal.pb.h"

namespace resdb {
namespace pbft_rl {

class Graph {
public:
  Graph() { refered_ = 0; }
  void Inc(int num) { refered_+=num; }
  bool Ready(int count) { 
  //LOG(ERROR)<<" check ready refer:"<<refered_<<" g:"<<this; 
  return refered_ >= count; }
  void SetRound(int round) { round_ = round; }
  uint64_t Round(){ return round_; }
  void AddEdge(uint64_t id1, uint64_t id2);

  std::vector<std::pair<uint64_t, uint64_t>> GetOrder();
  int Size(){ return g_.size(); }

  void AddNode(uint64_t  id);
  void ClearGraph();
  bool IsTournament();

  std::vector<uint64_t> GetAllNode();

private:
  void CheckGraph();
  void Clear();
  void Dfs(uint64_t u) ;
  void Order();

  private:
  std::map<uint64_t, std::set<uint64_t> > g_;
  std::map<uint64_t, std::vector<uint64_t> > og_;

  std::map<uint64_t, uint64_t> low_, dfn_, visit_, belong_, d_, v_;
  std::vector<std::pair<uint64_t, uint64_t>> result_;
  std::stack<uint64_t> stk_;
  uint64_t tot_ = 0;
  uint64_t scc_ = 0;
  std::map<uint64_t, std::set<uint64_t> > ogs_;
  uint64_t ver_ = 1;
  uint64_t edge_num_ = 0;
  uint64_t round_;
  uint64_t refered_ = 0;
};

}  // namespace pbft_rl
}  // namespace resdb

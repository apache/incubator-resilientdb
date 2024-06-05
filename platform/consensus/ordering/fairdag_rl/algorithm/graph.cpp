#include "platform/consensus/ordering/fairdag_rl/algorithm/graph.h"

#include <glog/logging.h>
#include <queue>
#include "common/utils/utils.h"


namespace resdb {
namespace fairdag_rl {

void Graph::Clear() {
  int num = idx_+1;
  low_ = std::vector<int>(num,0);
  dfn_ = std::vector<int>(num,0);
  visit_ = std::vector<int>(num,0);
  while(!stk_.empty())stk_.pop();
  belong_ = std::vector<int>(num,0);
  tot_ = 0;
  scc_ = 0;
  d_ = std::vector<int>(num,0);
  ogs_ = std::vector<std::set<int> > (num);
}

void Graph::Dfs(int u) {
    low_[u] = dfn_[u] = ++tot_;
    stk_.push(u);
    visit_[u] = true;
    for (int v :  g_[u]){
        if(g_.find(v) == g_.end()){
          continue;
        }
        if (!dfn_[v]) {
            Dfs(v);
            low_[u] = std::min(low_[u], low_[v]);
        }
        else if (visit_[v]) {
            low_[u] = std::min(low_[u], dfn_[v]);
        }
    }
    if (dfn_[u] == low_[u]) {
        ++scc_;
        int v;
        do {
          v = stk_.top();
          stk_.pop();
          belong_[v] = scc_;
          ogs_[scc_].insert(v);
        } while (v != u);
    }
}

void Graph::Order(){
  for (auto it :  g_){
    int u = it.first;
    for (int v :  g_[u]){
      if(belong_[v] != belong_[u]){
        og_[belong_[u]].push_back(belong_[v]);
        d_[belong_[v]]++;
      }
    }
  }

  std::queue<int> q;
  result_.clear();
  for(int i = 1; i <= scc_; ++i){
    if(d_[i] == 0){
      q.push(i);
    }
  }
  while(!q.empty()){
    int u = q.front();
    q.pop();
    for(int x : ogs_[u]){
      result_.push_back(x);
    }
    for(int v : og_[u]){
      d_[v]--;
      if(d_[v]==0){
        q.push(v);
      }
    }
  }
}

int Graph::GetID(const std::string & hash){
  assert(hash2idx_.find(hash) != hash2idx_.end());
    return hash2idx_[hash];
}

int Graph::GetTxnID(const Transaction& txn){
  std::string hash = txn.hash();
  auto it = hash2idx_.find(hash);
  if(it == hash2idx_.end()){
    hash2idx_[hash]=idx_;
    return idx_++;
  }
  return it->second;
}

void Graph::AddTxn(int  a, int b){
  int id1 = a;
  int id2 = b;
  if(g_[id1].find(id2) != g_[id1].end()){
    return;
  }

  g_[id1].insert(id2);
  idx_ = std::max(idx_, id1);
  idx_ = std::max(idx_, id2);
  return;
}

void Graph::RemoveTxn(int id){
  if(g_.find(id) == g_.end()) return;
  g_.erase(g_.find(id));
}

std::vector<int> Graph::GetOrder(const std::vector<int>& commit_txns) {
  std::vector<int> res;
  Clear();
  //LOG(ERROR)<<" commit_txns:"<<commit_txns.size();
  for(auto it : g_){
    int x = it.first;
    if(x<10){
      LOG(ERROR)<<" dfs x:"<<x;
    }
    if(visit_[x]){
      continue;
    }
    Dfs(x);
  }

  Order();

  std::set<int > id_m;
  for(int id : commit_txns){
    id_m.insert(id); 
  }

  for(int x :result_){
    if(id_m.find(x) != id_m.end()){
      res.push_back(x); 
    }
  }
  assert(res.size() != 0);
  
  return res;
}

}  // namespace fairdag_rl
}  // namespace resdb

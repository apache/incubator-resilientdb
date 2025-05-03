#include "platform/consensus/ordering/hs_rl/algorithm/graph.h"

#include <glog/logging.h>
#include <queue>
#include "common/utils/utils.h"


namespace resdb {
namespace hs_rl {

void Graph::ClearGraph(){
  g_.clear();
  preg_.clear();
  og_.clear();
  hash2idx_.clear();
  idx_ = 0;
  Clear();
}

void Graph::Clear() {
  // int num = g_.size()+1;
  int num = idx_+1;
  //LOG(ERROR)<<" clear num:"<<num;
  low_ = std::vector<int>(num,0);
  dfn_ = std::vector<int>(num,0);
  visit_ = std::vector<int>(num,0);
  while(!stk_.empty())stk_.pop();
  belong_ = std::vector<int>(num,0);
  tot_ = 0;
  scc_ = 0;
  d_ = std::vector<int>(num,0);
  ogs_ = std::vector<std::set<int> > (num);
  og_.clear();
  v_ = std::vector<int>(num,0);
  ver_ = 1;
}

bool Graph::IsTournament(){
  int num = g_.size() - del_.size();
  LOG(ERROR)<<"check is tournament num:"<< num << "expected edges:" << num*(num-1)/2 <<" edges:"<< edge_num_ <<" graph round:"<< round_;
  std::map<int, std::set<int>> to;

  if (edge_num_ >= num*(num-1)/2) {
    for(auto from:g_) {
      for (auto x:from.second) {
        assert(g_.find(x) != g_.end());
        to[x].insert(from.first);
      }
    }
    bool valid = true;
    if(round_ > 0) {
      for(auto from:g_) {
        int count = from.second.size() + to[from.first].size();
        // LOG(ERROR) << from.first << " " << count << " " << from.second.size() << " " << to[from.first].size();
        if(count + 1 != g_.size()) {
          valid = false;
        }
      }
    }

    if(!valid) {
      assert(false);
    }
  }
  
  return edge_num_ >= num*(num-1)/2;
}

void Graph::ResetEdge(){
  int a = edge_num_;
  edge_num_ = 0;
  for (auto& node : g_) {
    edge_num_ += node.second.size();
  }
  LOG(ERROR) << "CUT EDGES in round: " << round_ << " " << a - edge_num_;
}

void Graph::Remove(int u){
  int id = Node2ID(u);
  //LOG(ERROR)<<" remove :"<<u<<" id:"<<id<<" graph:"<<this;
  del_.insert(id);
}

void Graph::RemoveNodesEdges(std::set<int> txn_ids) {
  std::set<int> removable;
  for (auto txn_id: txn_ids) {
    if (node_.find(txn_id) != node_.end()) {
      int id = Node2ID(txn_id);
      removable.insert(id);
      g_[id].clear();
      g_.erase(id);
    }
  }
  for (auto it = g_.begin(); it != g_.end(); it++) {
    for (auto id: removable) {
      it->second.erase(id);
    }
  }
  for (auto id: removable) {
    node_.erase(ID2Node(id));
    id2node_.erase(id);
  }
  ResetEdge();
}

void Graph::Dfs(int u) {
  assert(del_.find(u) == del_.end());
    low_[u] = dfn_[u] = ++tot_;
    v_[u] = ver_;
    stk_.push(u);
    visit_[u] = true;
    //LOG(ERROR)<<" dfs u:"<<u;
    for (int v :  g_[u]){
      if(del_.find(v) != del_.end()){
        continue;
      }
        if(g_.find(v) == g_.end()){
          continue;
        }
        if (!dfn_[v]) {
            Dfs(v);
            low_[u] = std::min(low_[u], low_[v]);
        }
        else if (visit_[v]) {
          if(v_[v] == v_[u]){
            low_[u] = std::min(low_[u], dfn_[v]);
          }
        }
    }
    //LOG(ERROR)<<" dfn:"<<dfn_[u]<<" low:"<<low_[u];
    if (dfn_[u] == low_[u]) {
        ++scc_;
        int v;
        do {
          v = stk_.top();
          stk_.pop();
        //  LOG(ERROR)<<" get v:"<<v<<" scc:"<<scc_;
          belong_[v] = scc_;
          ogs_[scc_].insert(v);
          if(ogs_[scc_].size()>=1){
         //   LOG(ERROR)<<" find group"<<" v:"<< v<< " scc:"<<scc_;
          }
        } while (v != u);
    }
}

void Graph::Order(){
  LOG(ERROR)<<" order g size:"<<g_.size();
  for (auto it :  g_){
    int u = it.first;
    if(del_.find(u) != del_.end()){
      continue;
    }
   // LOG(ERROR)<<" u:"<<u<<" belong:"<<belong_[u];
    assert(belong_[u]>0);
    for (int v :  g_[u]){
      if(belong_[v] != belong_[u]){
        og_[belong_[u]].push_back(belong_[v]);
        d_[belong_[v]]++;
        //LOG(ERROR)<<" add desp:"<<belong_[u]<<" to:"<<belong_[v];
      }
    }
  }

  LOG(ERROR)<<" scc:"<<scc_;
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
    // LOG(ERROR)<<" get group:"<<u;
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

/*
int Graph::GetTxnID(const Transaction& txn){
  std::string hash = txn.hash();
  auto it = hash2idx_.find(hash);
  if(it == hash2idx_.end()){
    hash2idx_[hash]=idx_;
    return idx_++;
  }
  return it->second;
}
*/

int Graph::Node2ID(int a){
  if(node_.find(a) == node_.end()){
    id2node_[idx_]=a;
    node_[a] = idx_++;
  }
  return node_[a];
}

int Graph::ID2Node(int id){
    return id2node_[id];
}

void Graph::AddNode(int a){
  if (node_.find(a) == node_.end()) {
    int id = Node2ID(a);
    g_[id] = std::set<int>();
    // LOG(ERROR)<<"add node:"<<a<<" round:"<< round_;
  }
}

void Graph::AddEdge(int  a, int b){
  int id1 = Node2ID(a);
  int id2 = Node2ID(b);
  if(a==b) return;
  if(g_[id1].find(id2) != g_[id1].end()){
  //LOG(ERROR)<<" edge:"<<a<<" "<<b<<" exist";
    return;
  }
  if(g_[id2].find(id1) != g_[id2].end()){
  //LOG(ERROR)<<" edge:"<<b<<" "<<a<<" exist";
    return;
  }

  //LOG(ERROR)<<" add edge:"<<a<<" "<<b<<" graph:"<<this<<" id1:"<<id1<<" id2:"<<id2;
  g_[id1].insert(id2);
  idx_ = std::max(idx_, id1);
  idx_ = std::max(idx_, id2);
  edge_num_++;
  return;
}

/*
void Graph::RemoveTxn(int id){
  if(g_.find(id) == g_.end()) return;
  //LOG(ERROR)<<" remove node:"<<id;
  for(auto it : g_){
    int u = it.first;
    if(g_[u].find(id) != g_[u].end()){
      for(int  v : g_[id]){
        if(g_[u].find(v) == g_[u].end()) {
          //LOG(ERROR)<<" remove node:"<<id<<" from:"<<u<<" to:"<<v<<" fail";
          g_[u].insert(v);
        }
        //assert(g_[u].find(v) != g_[u].end());
      }
    }
  }
  g_.erase(g_.find(id));
}
*/

std::vector<int> Graph::GetOrder() {
  if(g_.size()==0){
    return std::vector<int>();
  }

  LOG(ERROR)<<"get order:"<< round_ <<" size:"<<g_.size();
  std::vector<int> res;
  Clear();
  //LOG(ERROR)<<" commit_txns:"<<commit_txns.size();
  for(auto it : g_){
    int x = it.first;
    if(del_.find(x) != del_.end()){
      continue;
    }
    //LOG(ERROR)<<" dfs x:"<<x;
    if(visit_[x]){
      continue;
    }
    ver_++;
    Dfs(x);
  }
  Order();
  for(size_t i = 0; i < result_.size(); ++i){
    result_[i] = ID2Node(result_[i]);
    // LOG(ERROR) << "result[" << i << "] = " << result_[i];
  }
  return result_;
}



}  // namespace hs_rl
}  // namespace resdb

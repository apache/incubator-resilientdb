#include "platform/consensus/ordering/pbft_rl/algorithm/graph.h"

#include <glog/logging.h>
#include <queue>
#include "common/utils/utils.h"


namespace resdb {
namespace pbft_rl {

std::vector<uint64_t> Graph::GetAllNode() {
  std::vector<uint64_t> v;
  for (auto& it: g_) {
    v.push_back(it.first);
  }
  return v;
}


void Graph::ClearGraph(){
  g_.clear();
  Clear();
}

void Graph::Clear() {
  int num = g_.size()+1;
  //LOG(ERROR)<<" clear num:"<<num;
  low_.clear();
  dfn_.clear();
  visit_.clear();
  while(!stk_.empty())stk_.pop();
  belong_.clear();
  tot_ = 0;
  scc_ = 0;
  d_.clear();
  ogs_.clear();
  og_.clear();
  v_.clear();
  ver_ = 1;
}

bool Graph::IsTournament(){
  uint64_t num = g_.size();
  LOG(ERROR)<<"check is tournament num: "<< num << " expected edges: " << num*(num-1)/2 <<" edges: "<< edge_num_ <<" graph round: "<< round_;
  std::map<uint64_t, std::set<uint64_t>> source_nodes;

  assert(edge_num_ <= num*(num-1)/2);

  if (edge_num_ == num*(num-1)/2) {
    for(auto node:g_) {
      for (auto node_pointed_to : node.second) {
        assert(g_.find(node_pointed_to) != g_.end());
        source_nodes[node_pointed_to].insert(node.first);
      }
    }
    if(round_ > 0) {
      for(auto node:g_) {
        // LOG(ERROR) << node.first << " " << num << " " << node.second.size() << " " << source_nodes[node.first].size(); 
        if(node.second.size() + source_nodes[node.first].size() + 1 != num) {
          assert(false);
        }
      }
    }
  }
  
  return edge_num_ == num*(num-1)/2;
}


void Graph::Dfs(uint64_t u) {
    low_[u] = dfn_[u] = ++tot_;
    v_[u] = ver_;
    stk_.push(u);
    visit_[u] = true;
    //LOG(ERROR)<<" dfs u:"<<u;
    for (uint64_t v :  g_[u]){
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
        uint64_t v;
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
    uint64_t u = it.first;
   // LOG(ERROR)<<" u:"<<u<<" belong:"<<belong_[u];
    assert(belong_[u]>0);
    for (auto v :  g_[u]){
      if(belong_[v] != belong_[u]){
        og_[belong_[u]].push_back(belong_[v]);
        d_[belong_[v]]++;
        //LOG(ERROR)<<" add desp:"<<belong_[u]<<" to:"<<belong_[v];
      }
    }
  }

  LOG(ERROR)<<" scc:"<<scc_;
  std::queue<uint64_t> q;
  result_.clear();
  for(uint64_t i = 1; i <= scc_; ++i){
    if(d_[i] == 0){
      q.push(i);
    }
  }
  uint64_t s = 1;
  while(!q.empty()){
    uint64_t u = q.front();
    q.pop();
    // LOG(ERROR)<<" get group:"<<u;
    for(auto x : ogs_[u]){
      result_.push_back(std::make_pair(x, s));
    }
    for(auto v : og_[u]){
      d_[v]--;
      if(d_[v]==0){
        q.push(v);
      }
    }
    s++;
  }
}


void Graph::AddNode(uint64_t id){
  if(g_.find(id) == g_.end()) {
    g_[id] = std::set<uint64_t>();
  }
  
}


void Graph::AddEdge(uint64_t id1, uint64_t id2){
  if(id1==id2) return;
  if(g_[id1].find(id2) != g_[id1].end()){
  //LOG(ERROR)<<" edge:"<<id1<<" "<<id2<<" exist";
    return;
  }
  if(g_[id2].find(id1) != g_[id2].end()){
  //LOG(ERROR)<<" edge:"<<id2<<" "<<id1<<" exist";
    return;
  }

  //LOG(ERROR) << " add edge:" << " graph:" << this << " id1:" << id1 << " id2:" << id2;
  g_[id1].insert(id2);
  edge_num_++;
  return;
}

std::vector<std::pair<uint64_t, uint64_t>> Graph::GetOrder() {
  if(g_.size()==0){
    return std::vector<std::pair<uint64_t, uint64_t>>();
  }

  LOG(ERROR)<<"get order:"<< round_ <<" size:"<<g_.size();
  std::vector<uint64_t> res;
  Clear();
  //LOG(ERROR)<<" commit_txns:"<<commit_txns.size();
  for(auto it : g_){
    uint64_t x = it.first;
    //LOG(ERROR)<<" dfs x:"<<x;
    if(visit_[x]){
      continue;
    }
    ver_++;
    Dfs(x);
  }
  Order();
  return result_;
}



}  // namespace pbft_rl
}  // namespace resdb

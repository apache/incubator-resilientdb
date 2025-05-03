#include "platform/consensus/ordering/hs_rl/algorithm/themis.h"

#include <glog/logging.h>
#include "common/utils/utils.h"

// #define IMPL

namespace resdb {
namespace hs_rl {

Themis::Themis(
      int id, int f, int total_num, SignatureVerifier* verifier)
      : ProtocolBase(id, f, total_num), replica_num_(total_num){
  
   LOG(ERROR)<<"id:"<<id<<" f:"<<f<<" total:"<<replica_num_;

  hs_ = std::make_unique<HotStuff>(id, f, total_num, verifier, 
    [&](int type, const google::protobuf::Message& msg, int node){
    return SendMessage(type, msg, node);
  },
    [&](int type, const google::protobuf::Message& msg){
      return Broadcast(type, msg);
    },
    [&](std::vector<Transaction*>& txns){
      return ProcessCausalHistory(txns);
  });
  graph_ = std::make_unique<Graph>();
  local_time_ = 0;
  execute_id_ = 1;
  max_round_ = 0;
  global_stats_ = Stats::GetGlobalStats();
  shaded_threshold_ = (replica_num_ - 2 * f_ + 1) / 2;
  solid_threshold_ = replica_num_ - 2 * f_ ;
  LOG(ERROR) << "Initiated Themis";
}

Themis::~Themis() {
}

bool Themis::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
    std::unique_lock<std::mutex> lk(mutex_);
  //txn->set_timestamp(local_time_++);
  //txn->set_proposer(id_);
  //LOG(ERROR)<<"recv txn from:"<<txn->proxy_id()<<" proxy user seq:"<<txn->user_seq();
  return hs_->ReceiveTransaction(std::move(txn));
}

bool Themis::ReceiveLocalOrdering(std::unique_ptr<Proposal> proposal) {
  return hs_->ReceiveLocalOrdering(std::move(proposal));
}

bool Themis::ReceiveProposal(std::unique_ptr<Proposal> proposal) {
  return hs_->ReceiveProposal(std::move(proposal));
}

bool Themis::ReceiveCertificate(std::unique_ptr<Certificate> cert) {
  return hs_->ReceiveCertificate(std::move(cert));
}

bool Themis::IsCommitted(const Transaction& txn){
  return IsCommitted(txn.proxy_id(), txn.user_seq());
}

bool Themis::IsCommitted(int proxy_id, int user_seq){
  return committed_.find(std::make_pair(proxy_id, user_seq)) != committed_.end();
}

void Themis::SetCommitted(const Transaction& txn){
  SetCommitted(txn.proxy_id(), txn.user_seq());
}

void Themis::SetCommitted(int proxy_id, int user_seq){
  committed_.insert(std::make_pair(proxy_id, user_seq));
}

bool Themis::IsSolid(const Transaction& txn){
  return IsSolid(txn.proxy_id(), txn.user_seq());
}

bool Themis::IsSolid(int proxy_id, int user_seq){
  return solid_.find(std::make_pair(proxy_id, user_seq)) != solid_.end();
}

void Themis::SetSolid(uint64_t round, int proxy_id, int user_seq){
  // LOG(ERROR) << "[X]set solid: " << txn_key_2_idx_[std::make_pair(proxy_id, user_seq)];
  auto key = std::make_pair(proxy_id, user_seq);
  if (solid_.find(key) == solid_.end()){
    solid_[key] = round;
  }
}

void Themis::SetRoundSolid(uint64_t round, int proxy_id, int user_seq){
  round_solid_[round].insert(std::make_pair(proxy_id, user_seq));
}


void Themis::AddTxnData(std::unique_ptr<Transaction> txn) {
  std::pair<int,int64_t> key = std::make_pair(txn->proxy_id(), txn->user_seq());
  if(data_.find(key) == data_.end()){
    data_[key] = std::move(txn);
  }
}

std::vector<int> Themis::FinalOrder(Graph* g){
  //LOG(ERROR)<<" commit:"<<g;
  std::vector<int> orders = g->GetOrder();
  int round = g->Round();

  int last = orders.size()-1;
  for(; last>=0;--last){
    int id = orders[last];
    auto key = proposals_idx_2_key_[id];
    if(txn_key_proposers_[key].find(round) == txn_key_proposers_[key].end()){
      assert(false);
    }
    if(IsSolid(key.first, key.second) && solid_[key] <= round){
      break;
    }
  }

  // LOG(ERROR) << "A1" << last;
  // LOG(ERROR)<<" orders size:"<<orders.size()<<" last:"<<last<<" size:"<<txn_key_proposers_.size();

  if(last<0){
    return std::vector<int>();
  }

  // LOG(ERROR)<<"new edges:"<<possible_edges.size()<<" txn size:"<<txns.size()<<" id size:"<<ids.size()<<" txns:"<<non_blank_id.size();
  std::set<int> shaded_txn_ids;
  for(int i = 0; i <= last; ++i){
    int id = orders[i];
    if(proposals_idx_2_key_.find(id) == proposals_idx_2_key_.end()) {
      LOG(ERROR) << id;
      assert(false);
    }
    auto key = proposals_idx_2_key_[id];
    assert(proposals_data_.find(key) != proposals_data_.end());
    std::string hash = proposals_data_[key];
    
    // The ordered shaded transactions that should be removed from other graphs.
    if (!IsSolid(key.first, key.second) || solid_[key] > round) {
      LOG(ERROR) << "[X]Remove id:" << id; 
      shaded_txn_ids.insert(id);
    }

    std::unique_ptr<Transaction> data = hs_->FetchData(hash);
    if(data == nullptr){
      LOG(ERROR)<<"no txn"; 
    }
    assert(data != nullptr);
    // LOG(ERROR)<<"[X] order proposal:"<<data->proxy_id()<<" seq:"<<data->user_seq()<<" id:"<<id<<" graph round:"<<g->Round();
    data->set_id(execute_id_++);
    SetCommitted(*data);
    Commit(*data);

    // Free pending_proposals_
    for (auto& round: txn_key_proposers_[key]) {
      for(auto proposer : round.second){
        if(pending_proposals_[proposer].find(id) != pending_proposals_[proposer].end()){
          pending_proposals_[proposer].erase(pending_proposals_[proposer].find(id));
        }
      }
    }

    // Free txn_key_proposers_
    txn_key_proposers_[key].clear();
    txn_key_proposers_.erase(txn_key_proposers_.find(key));
    // Free solid_
    solid_.erase(key);

    // Free weight_ and e2g_
    for (auto key2: compared_node_keys_[key]) {
      auto e = std::make_pair(key, key2);
      weight_.erase(e);
      e2g_.erase(e);
    }
    compared_node_keys_.erase(key);

    // Free mapping between id and key
    proposals_idx_2_key_.erase(id);
    txn_key_2_idx_.erase(key);
  }

  if (shaded_txn_ids.size() > 0) {
    for (auto& future_graph: g_set_) {
      if (future_graph == g) {
        continue;
      }
      future_graph->RemoveNodesEdges(shaded_txn_ids);
    }
    LOG(ERROR) << "Remove DONE";
  }

  g->ClearGraph();
  return orders;
}

void Themis::AddLocalOrdering(Graph * g_ptr, const std::vector<Transaction*>& txns, std::set<std::pair<int,int>> &possible_edges) {
  std::vector<int> non_blank_id;
  assert(txns.size()>0);
  int round = g_ptr->Round();
  int proposer =  txns[0]->proposer();

  // Assign ID and Classify
  for(uint64_t i = 0; i < txns.size(); ++i){
    if(IsCommitted(*txns[i])){
      continue;
    }

    // Assign ID
    LOG(ERROR) << "[X] proxy id: " << txns[i]->proxy_id() << " user_seq: " << txns[i]->user_seq() << " round: " << round;
    std::pair<int,int64_t> key = std::make_pair(txns[i]->proxy_id(), txns[i]->user_seq());
    if(txn_key_2_idx_.find(key) == txn_key_2_idx_.end()){
      proposals_idx_2_key_[idx_] = key;
      txn_key_2_idx_[key]=idx_++;
      proposals_data_[key] = txns[i]->hash();
    }

    // Classify while counting the proposers in previous rounds
    if(txn_key_proposers_.find(key) != txn_key_proposers_.end() && txn_key_proposers_[key].find(round) == txn_key_proposers_[key].end()) {
      txn_key_proposers_[key][round] = txn_key_proposers_[key].rbegin()->second;
    }
    txn_key_proposers_[key][round].insert(proposer);
    LOG(ERROR) << "add proposer " << proposer << " done with " << txn_key_proposers_[key][round].size();

    if(txn_key_proposers_[key][round].size() >= shaded_threshold_){
      non_blank_id.push_back(txn_key_2_idx_[key]);
      // LOG(ERROR)<< "[X]round: " << round << " non-blank proposer:"<<proposer << " proxy id:"<<key.first<<" seq:"<<key.second << " commit size:" << txn_key_proposers_[key][round].size() << " id: " << txn_key_2_idx_[key];
      if (txn_key_proposers_[key][round].size() >= solid_threshold_) {
        SetRoundSolid(round, key.first, key.second);
      }
    }
  }

  // Add Non-Blank Nodes into the Graph
  for(uint64_t i = 0; i < non_blank_id.size(); ++i){
    int id = non_blank_id[i];
    auto key = proposals_idx_2_key_[id];
    LOG(ERROR) << "non-blank proxy: " << key.first << " user_seq: " << key.second << " id: " << id;
    if(id_2_first_g_.find(id) == id_2_first_g_.end()){
      LOG(ERROR)<<"[FD] add new node: "<<id <<" g round:"<< round << "with: " << txn_key_proposers_[proposals_idx_2_key_[id]][round].size();
      id_2_first_g_[id] = g_ptr;
      g_ptr->AddNode(id);
    } 
    else if (id_2_first_g_[id] == g_ptr) {
      LOG(ERROR)<<"[FD] node exists: "<<id <<" g round:"<< round << "with: " << txn_key_proposers_[proposals_idx_2_key_[id]][round].size();
      continue;
    }
    else if (!IsSolid(key.first, key.second)) {
      LOG(ERROR)<<"[FD] add duplicate shaded node: "<<id <<" g round:"<< round;
      g_ptr->AddNode(id);
    } 
    else {
      // LOG(ERROR)<<"[FD] already added as a solid: "<<id <<" g round:"<< round;
    }
  }

  // LOG(ERROR) << "Weight Calculation";

  // Weight Calculation between txns in this local ordering
  for(uint64_t i = 0; i < txns.size(); ++i){
    if(IsCommitted(*txns[i])){
      continue;
    }
    std::pair<int,int64_t> key1 = std::make_pair(txns[i]->proxy_id(), txns[i]->user_seq());
    for(uint64_t j = i+1; j < txns.size(); ++j){
      if(IsCommitted(*txns[j])){
        continue;
      }
      std::pair<int,int64_t> key2 = std::make_pair(txns[j]->proxy_id(), txns[j]->user_seq());
      weight_[std::make_pair(key1, key2)]++;
      compared_node_keys_[key1].insert(key2);
      if (weight_[std::make_pair(key1, key2)] >= shaded_threshold_) {
        int id1 = txn_key_2_idx_[key1];
        int id2 = txn_key_2_idx_[key2];
        auto e = std::make_pair(id1, id2);
        LOG(ERROR) << "[1]add possible edge " << id1 << " " << id2 << " with " << weight_[std::make_pair(key1, key2)];
        possible_edges.insert(e);
      }
      
    }
  }

  // LOG(ERROR) << "Weight Calculation2";

  // Weight Calculation between txns in this local ordering and the uncommitted txns in previous local orderings
  for(uint64_t i = 0; i < txns.size(); ++i){
    if(IsCommitted(*txns[i])){
      continue;
    }
    std::pair<int,int64_t> key2 = std::make_pair(txns[i]->proxy_id(), txns[i]->user_seq());
    assert(txn_key_2_idx_.find(key2) != txn_key_2_idx_.end());
    int id2 = txn_key_2_idx_[key2];
    for(auto id1: pending_proposals_[proposer]){
      if (EdgeNeeded(id1, id2)) {
        auto key1 = proposals_idx_2_key_[id1];
        weight_[std::make_pair(key1, key2)]++;
        compared_node_keys_[key1].insert(key2);
        if (weight_[std::make_pair(key1, key2)] >= shaded_threshold_) {
          auto e = std::make_pair(id1, id2);
          // LOG(ERROR) << "e with id id1: " << id1 << " id2: " << id2 << " weight: " << weight_[std::make_pair(key1, key2)] << " pair :" << key1.first << " " << key1.second << " " << key2.first << " " << key2.second;
          LOG(ERROR) << "[2]add possible edge " << id1 << " " << id2;
          possible_edges.insert(e);
          // if (!IsSolid(key1.first, key1.second)) {
          //   g_ptr->AddNode(id1);
          // }
        }
      }
    }
  }

  // LOG(ERROR) << "Append Pending Txn";

  // Append Pending Transactions
  for(uint64_t i = 0; i < txns.size(); ++i){
    if(IsCommitted(*txns[i])){
      continue;
    }
    std::pair<int,int64_t> key = std::make_pair(txns[i]->proxy_id(), txns[i]->user_seq());
    int id = txn_key_2_idx_[key];
    pending_proposals_[proposer].insert(id);
  }

  // LOG(ERROR) << "DONE";
}

void Themis::AddEdges(std::set<std::pair<int,int>>& possible_edges) {
  int i = 0;
  for(auto e: possible_edges){
    auto key1 = proposals_idx_2_key_[e.first];
    auto key2 = proposals_idx_2_key_[e.second];
    uint64_t k1 = weight_[std::make_pair(key1, key2)];
    uint64_t k2 = weight_[std::make_pair(key2, key1)];
    // LOG(ERROR)<<" count key1:"<<key1.first<<" "<<key1.second<<" num:"<< k1<<"  id:"<<e.first
    // <<" key2:"<<key2.first<<" "<<key2.second<<" num:"<<k2<<" id:"<<e.second;
    assert(k1 >= shaded_threshold_ || k2 >= shaded_threshold_);
    bool direction = (k1 >= k2);

    int id1 = e.first;
    int id2 = e.second;
    auto graphs = CommonGraphs(e);
    // LOG(ERROR) << "[X]Common Graphs Found";
    for(auto g : graphs){
      assert(g != nullptr);
      if(direction){
        g->AddEdge(id1, id2);
        // LOG(ERROR)<<" add edge id1:"<<id1<<" id2:"<<id2<<" into g_round: "<<g->Round();
      }
      else {
        g->AddEdge(id2, id1);
        // LOG(ERROR)<<" add edge id2:"<<id2<<" id1:"<<id1<<" into g_round: "<<g->Round();
      }
    }
    // LOG(ERROR) << "[X] AddEdges Done for " << ++i << " in " <<  possible_edges.size();
  }
}

void Themis::ProcessCausalHistory(const std::vector<Transaction*> & txns) {
#ifndef IMPL
  int graph_round = 0;
  auto g = std::make_unique<Graph>();
  Graph * g_ptr = g.get();

  g_set_.insert(g_ptr);
  g_queue_.push(std::move(g)); 

  std::vector<Transaction*> txns_ptrs;
  for(auto& txn : txns){
    graph_round = std::max(graph_round, txn->round());
    txns_ptrs.push_back(txn); 
  }
  g_ptr->SetRound(graph_round);
  ConstructDependencyGraph(g_ptr, txns_ptrs);
  CheckGraph();
#endif
  
#ifdef IMPL
  std::map<int, std::vector<Transaction*>> txn_list;
  std::map<int, std::set<int>> refnum;
  for(auto &txn : txns){
    int round = txn->round();
    refnum[round].insert(txn->proposer());
    //LOG(ERROR)<<" get round:"<<round<<" max round:"<<max_round_;
    while(round >= max_round_){
      auto g = std::make_unique<Graph>();
      g->SetRound(max_round_);
      local_g_[max_round_] = g.get();
      //LOG(ERROR)<<" get new txn:"<<g.get()<<" round:"<<max_round_;
      g_queue_.push(std::move(g)); 
      max_round_++;
    }
    auto it = local_g_.find(round);
    if(it == local_g_.end()){
      round = max_round_-1;
      assert(local_g_.find(round) != local_g_.end());
    }
    txn_list[round].push_back(txn.get());
  }
  for(auto it : refnum){
    if(local_g_.find(it.first)!=local_g_.end()){
      //LOG(ERROR)<<" add ref num round:"<<it.first<<" num:"<<it.second.size();
      local_g_[it.first]->Inc(it.second.size());
    }
  }

  for(auto it : txn_list){
    //LOG(ERROR)<<" check round:"<<it.first<<" txn size:"<<it.second.size()<<" grapph:"<<local_g_[it.first];
    ConstructDependencyGraph(local_g_[it.first], it.second);
  }
  CheckGraph();
 #endif 
  //LOG(ERROR)<<" check graph";
  //CheckGraph();
}

void Themis::ConstructDependencyGraph(Graph * g, const std::vector<Transaction*>& txns) {
  assert(g!=nullptr);
  int proposer = -1;
  std::vector<Transaction*> proposal_list;
  std::set<std::pair<int, int>> possible_edges;
  for(uint64_t i = 0; i < txns.size(); ++i){
    if(IsCommitted(*txns[i])){
      continue;
    }
    if(proposer >=0 && txns[i]->proposer() != proposer){
      // [DK] Add the transactions that are from different replicas one by one
      LOG(ERROR) << "proposer: " << proposer;
      AddLocalOrdering(g, proposal_list, possible_edges);
      proposal_list.clear();
    }
    proposal_list.push_back(txns[i]);
    proposer = txns[i]->proposer();
  }
  if(proposal_list.size()>0){
    LOG(ERROR) << "proposal_list: ";
    AddLocalOrdering(g, proposal_list, possible_edges);
  }
  uint64_t round = g->Round();
  for (auto & key : round_solid_[round]) {
    SetSolid(round, key.first, key.second);
  }
  round_solid_[round].clear();
  round_solid_.erase(round);
  LOG(ERROR) << "Start Adding Edges";
  AddEdges(possible_edges);
  LOG(ERROR) << "End Adding Edges";
}

void Themis::CheckGraph(){
  LOG(ERROR)<<"start";
  while(!g_queue_.empty()){
    Graph * g = g_queue_.front().get();
    #ifdef IMPL
    if(!g->Ready(2*f_+1)) {
      //LOG(ERROR)<<" graph:"<<g<<" not ready";
      break;
    }
    #endif 
    // if(g->Size()==0){
    //   //LOG(ERROR)<<" graph is done:"<<g<<" round:"<<g->Round();
    //   #ifdef IMPL
    //   assert(local_g_.find(g->Round()) != local_g_.end());
    //   local_g_.erase(local_g_.find(g->Round()));
    //   #endif
    //   g_queue_.pop();
    // }
    if(g->IsTournament()){
      std::vector<int> orders = FinalOrder(g);
      g_set_.erase(g);
      LOG(ERROR)<<"[X] graph is done:"<<g<<" order size:"<<orders.size()<<" graph size:"<<g->Size() << " round: " << g->Round();
      // if(orders.size()==0){
      //   break;
      // }
      g_queue_.pop();
    }
    else {
      break;
    }
  }
  LOG(ERROR)<<"done";
}

bool Themis::EdgeNeeded(int id1, int id2) {
  int lower_round_1, lower_round_2;
  int upper_round_1, upper_round_2;
  if(id_2_first_g_.find(id1) == id_2_first_g_.end() || id_2_first_g_.find(id2) == id_2_first_g_.end()) {
    return true;
  }
  lower_round_1 = id_2_first_g_[id1]->Round();
  lower_round_2 = id_2_first_g_[id2]->Round();
  upper_round_1 = upper_round_2 = 0x7FFFFFF;
  auto key1 = proposals_idx_2_key_[id1];
  auto key2 = proposals_idx_2_key_[id2];
  if (IsSolid(key1.first, key1.second)) {
    upper_round_1 = solid_[key1];
  } 
  if (IsSolid(key2.first, key2.second)) {
    upper_round_2 = solid_[key2];
  }
  return (upper_round_1 >= lower_round_2) || (upper_round_2 >= lower_round_1);
}

std::vector<Graph*> Themis::CommonGraphs(std::pair<int,int> id_pair) {
  std::vector<Graph*> v;
  int id1 = id_pair.first;
  int id2 = id_pair.second;
  auto key1 = proposals_idx_2_key_[id1];
  auto key2 = proposals_idx_2_key_[id2];
  auto e = std::make_pair(key1, key2);

  if (id_2_first_g_.find(id1) == id_2_first_g_.end() || id_2_first_g_.find(id2) == id_2_first_g_.end()) {
    if (id_2_first_g_.find(id1) == id_2_first_g_.end()) {
      LOG(ERROR) << "id1: " << id1;
    }
    if (id_2_first_g_.find(id2) == id_2_first_g_.end()) {
      LOG(ERROR) << "id2: " << id2;
    }
    assert(false);
  }
  int lower_round_1 = id_2_first_g_[id1]->Round();
  int lower_round_2 = id_2_first_g_[id2]->Round();
  int upper_round_1, upper_round_2;
  upper_round_1 = upper_round_2 = 0x7FFFFFF;

  if (IsSolid(key1.first, key1.second)) {
    upper_round_1 = solid_[key1];
  } 
  if (IsSolid(key2.first, key2.second)) {
    upper_round_2 = solid_[key2];
  }
  int lower_round = std::max(lower_round_1, lower_round_2);
  int upper_round = std::min(upper_round_1, upper_round_2);

  // LOG(ERROR) << "[X] Adding Edges between " << id1 << " and " << id2 << " from " << lower_round << " to " << upper_round << " " << IsSolid(key1.first, key1.second);

  if (upper_round >= lower_round) {
    for(auto g: g_set_) {
      int round = g->Round();
      if (round < lower_round || round > upper_round) {
        continue;
      } else if (e2g_[e].find(g) != e2g_[e].end()) {
        continue;
      } else {
        v.push_back(g);
        e2g_[e].insert(g);
      }
    }
  }
  return v;
}


// done:0x723c90000b70

}  // namespace tusk
}  // namespace resdb

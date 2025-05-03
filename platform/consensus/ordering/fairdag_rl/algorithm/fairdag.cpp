#include "platform/consensus/ordering/fairdag_rl/algorithm/fairdag.h"

#include <glog/logging.h>
#include "common/utils/utils.h"

// #define IMPL

namespace resdb {
namespace fairdag_rl {

FairDAG::FairDAG(
      int id, int f, int total_num, SignatureVerifier* verifier)
      : ProtocolBase(id, f, total_num), replica_num_(total_num){
  
   LOG(ERROR)<<"id:"<<id<<" f:"<<f<<" total:"<<replica_num_;

  tusk_ = std::make_unique<Tusk>(id, f, total_num, verifier, 
    [&](int type, const google::protobuf::Message& msg, int node){
    return SendMessage(type, msg, node);
    },
    [&](int type, const google::protobuf::Message& msg){
      return Broadcast(type, msg);
    },
    [&](std::unique_ptr<std::vector<std::unique_ptr<Transaction>>> local_orderings){
      return Push_Committed_LO(std::move(local_orderings));
    });
    graph_ = std::make_unique<Graph>();
  local_time_ = 0;
  execute_id_ = 1;
  max_round_ = 0;
  global_stats_ = Stats::GetGlobalStats();
  shaded_threshold_ = f_ + 1;
  solid_threshold_ = 2 * f_ +1;
  order_manager_ = std::make_unique<OrderManager>();
  process_comitted_lo_thread_ = std::thread(&FairDAG::AsyncProcessCommittedLO, this);
}

FairDAG::~FairDAG() {
}

bool FairDAG::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  std::unique_lock<std::mutex> lk(mutex_);
  //txn->set_timestamp(local_time_++);
  //txn->set_proposer(id_);
  //LOG(ERROR)<<"recv txn from:"<<txn->proxy_id()<<" proxy user seq:"<<txn->user_seq();
  return tusk_->ReceiveTransaction(std::move(txn));
}

bool FairDAG::ReceiveBlock(std::unique_ptr<Proposal> proposal) {
  return tusk_->ReceiveBlock(std::move(proposal));
}

void FairDAG::ReceiveBlockACK(std::unique_ptr<Metadata> metadata) {
  tusk_->ReceiveBlockACK(std::move(metadata));
}

void FairDAG::ReceiveBlockCert(std::unique_ptr<Certificate> cert) {
  tusk_->ReceiveBlockCert(std::move(cert));
}

bool FairDAG::IsCommitted(const Transaction& txn){
  return IsCommitted(txn.proxy_id(), txn.user_seq());
}

bool FairDAG::IsCommitted(int proxy_id, uint64_t user_seq){
  return committed_.find(std::make_pair(proxy_id, user_seq)) != committed_.end();
}

void FairDAG::SetCommitted(const Transaction& txn){
  SetCommitted(txn.proxy_id(), txn.user_seq());
}

void FairDAG::SetCommitted(int proxy_id, uint64_t user_seq){
  committed_.insert(std::make_pair(proxy_id, user_seq));
}

bool FairDAG::IsSolid(const uint64_t id){
  return solid_.find(id) != solid_.end();
}

void FairDAG::SetSolid(uint64_t round, uint64_t id){
  if (solid_.find(id) == solid_.end()){
    solid_[id] = round;
  }
}

void FairDAG::SetRoundSolid(uint64_t round, uint64_t id){
  round_solid_[round].insert(id);
}


// void FairDAG::AddTxnData(std::unique_ptr<Transaction> txn) {
//   std::pair<int,int64_t> key = std::make_pair(txn->proxy_id(), txn->user_seq());
//   if(data_.find(key) == data_.end()){
//     data_[key] = std::move(txn);
//   }
// }

std::vector<std::pair<uint64_t, uint64_t>> FairDAG::FinalOrder(Graph* g){
  //LOG(ERROR)<<" commit:"<<g;
  auto orders = g->GetOrder();
  uint64_t round = g->Round();

  int last = orders.size()-1;
  for(; last>=0;--last){
    auto id = orders[last].first;
    if(IsSolid(id) && solid_[id] <= round){
      break;
    } else {
      txn_to_add_ids_.push_back(id);
    }
  }

  // LOG(ERROR) << "A1" << last;
  // LOG(ERROR)<<" orders size:"<<orders.size()<<" last:"<<last<<" size:"<<txn_id_proposers_.size();

  if(last<0){
    return std::vector<std::pair<uint64_t, uint64_t>>();
  }

  // LOG(ERROR)<<"new edges:"<<possible_edges.size()<<" txn size:"<<txns.size()<<" id size:"<<ids.size()<<" txns:"<<non_blank_id.size();
  // std::set<int> shaded_txn_ids;
  for(int i = 0; i <= last; ++i){
    auto id = orders[i].first;
    auto scc = orders[i].second;
    if(proposals_idx_2_key_.find(id) == proposals_idx_2_key_.end()) {
      assert(false);
    }
    auto key = proposals_idx_2_key_[id];
    assert(txn_hash_.find(id) != txn_hash_.end());
    std::string hash = txn_hash_[id];
    
    std::unique_ptr<Transaction> data = tusk_->FetchTxn(hash);
    if(data == nullptr){
      LOG(ERROR)<<"no txn"; 
    }

    if(commit_time_.find(id) != commit_time_.end()){
	    uint64_t c_time = GetCurrentTime() - commit_time_[id];
	    // LOG(ERROR)<<"commit proposal: seq:"<< id <<" commit:"<<c_time;
    }

    assert(data != nullptr);
    // LOG(ERROR)<<"[X] order proposal:"<<data->proxy_id()<<" seq:"<<data->user_seq()<<" id:"<<id<<" graph round:"<<g->Round();
    data->set_id(execute_id_++);
    order_manager_->AddFinalOrderingRecord(data->proxy_id(), data->user_seq(), round * 100 + scc);
    SetCommitted(*data);
    Commit(*data);

    // Free pending_txn_ids_
    all_pending_txn_ids_.erase(id);
    LOG(ERROR) << "[ER] " << id;
    for (auto& round: txn_id_proposers_[id]) {
      for(auto proposer : round.second){
        if(pending_txn_ids_[proposer].find(id) != pending_txn_ids_[proposer].end()){
          pending_txn_ids_[proposer].erase(pending_txn_ids_[proposer].find(id));
        }
      }
    }

    // Free txn_id_proposers_
    txn_id_proposers_[id].clear();
    txn_id_proposers_.erase(txn_id_proposers_.find(id));
    // Free solid_
    solid_.erase(id);

    // Free weight_
    for (auto id2: compared_txn_ids_[id]) {
      auto e1 = std::make_pair(id, id2);
      auto e2 = std::make_pair(id2, id);
      weight_.erase(e1);
      weight_.erase(e2);
    }
    compared_txn_ids_.erase(id);

    // Free mapping between id and key
    proposals_idx_2_key_.erase(id);
    id_2_first_g_.erase(id);
    txn_key_2_idx_.erase(key);
  }

  // if (shaded_txn_ids.size() > 0) {
  //   for (auto& future_graph: g_set_) {
  //     if (future_graph == g) {
  //       continue;
  //     }
  //     future_graph->RemoveNodesEdges(shaded_txn_ids);
  //   }
  //   LOG(ERROR) << "Remove DONE";
  // }

  g->ClearGraph();
  return orders;
}

std::set<uint64_t> FairDAG::set_difference(const std::set<uint64_t>& A, const std::set<uint64_t>& B) {
    std::set<uint64_t> result;
    for (const auto& elem : A) {
        if (B.find(elem) == B.end()) {
            result.insert(elem);
        }
    }
    return result;
}

void FairDAG::AddImplictWeight(Graph * g_ptr, std::set<std::pair<uint64_t,uint64_t>> &possible_edges) {
  std::unique_lock<std::mutex> lk(lo_mutex_);
  for (uint64_t proposer = 1; proposer <= replica_num_; proposer++) {
    auto unseen_txn_ids = set_difference(all_pending_txn_ids_, pending_txn_ids_[proposer]);
    for (auto & id1: pending_txn_ids_[proposer]) {
      for (auto & id2: unseen_txn_ids) {
        auto e = std::make_pair(id1, id2);
        weight_[e].insert(proposer);
        // LOG(ERROR) << "[AW]: "<< id1 << " " << id2 << " size: " << weight_[e].size();
        compared_txn_ids_[id1].insert(id2);
        if (weight_[e].size() >= shaded_threshold_) {
          // LOG(ERROR) << "[AE]: "<< id1 << " " << id2;
          // fflush(stdout);
          possible_edges.insert(e);
        }
      }
    }
  }
}

void FairDAG::AddLocalOrdering(Graph * g_ptr, const std::vector<Transaction*>& txns, std::set<std::pair<uint64_t,uint64_t>> &possible_edges) {
  std::unique_lock<std::mutex> lk(lo_mutex_);
  std::vector<uint64_t> non_blank_id;
  std::vector<uint64_t> txn_ids;
  assert(txns.size()>0);
  int round = g_ptr->Round();
  int proposer =  txns[0]->proposer();

  // Assign ID and Classify
  for(uint64_t i = 0; i < txns.size(); ++i){
    if(IsCommitted(*txns[i])){
      continue;
    }

    uint64_t id;
    // Assign ID
    std::pair<int,int64_t> key = std::make_pair(txns[i]->proxy_id(), txns[i]->user_seq());
    if(txn_key_2_idx_.find(key) == txn_key_2_idx_.end()){
      proposals_idx_2_key_[idx_] = key;
      txn_hash_[idx_] = txns[i]->hash();
      txn_key_2_idx_[key]=idx_++;
    }
    id = txn_key_2_idx_[key];
    txn_ids.push_back(id);
    
    // Classify while counting the proposers in previous rounds
    if(txn_id_proposers_.find(id) != txn_id_proposers_.end() && txn_id_proposers_[id].find(round) == txn_id_proposers_[id].end()) {
      txn_id_proposers_[id][round] = txn_id_proposers_[id].rbegin()->second;
    }
    txn_id_proposers_[id][round].insert(proposer);


    if(commit_time_.find(id) == commit_time_.end()){
	    commit_time_[id] = GetCurrentTime();
    }

    // LOG(ERROR)<< "i: " << i << " id: "<< id <<  " round: " << round << " size: " << txn_id_proposers_[id][round].size() << " proposer: " << proposer << " " << key.first << " " <<  key.second;

    if(txn_id_proposers_[id][round].size() >= shaded_threshold_){
      
      if(commit_time_.find(id) != commit_time_.end()){
	      uint64_t c_time = GetCurrentTime() - commit_time_[id];
	      // LOG(ERROR)<<" commit proposal: create time:"<<commit_time_[id]<<" commit:"<<c_time;
      }

      non_blank_id.push_back(id);
      // LOG(ERROR)<< "[X]round: " << round << " non-blank proposer:"<<proposer << " proxy id:"<<key.first<<" seq:"<<key.second << " commit size:" << txn_id_proposers_[id][round].size() << " id: " << id;
      if (txn_id_proposers_[id][round].size() >= solid_threshold_) {
        SetRoundSolid(round, id);
      }
    }
  }

  // Add Non-Blank Nodes into the Graph
  for(uint64_t i = 0; i < non_blank_id.size(); ++i){
    uint64_t id = non_blank_id[i];
    if(id_2_first_g_.find(id) == id_2_first_g_.end()){
      auto key = proposals_idx_2_key_[id];
      // LOG(ERROR)<<"[F1] add new node: "<< id << " " << key.first << " " << key.second <<" g round: "<< round << " with: " << txn_id_proposers_[id][round].size();
      id_2_first_g_[id] = g_ptr;
      g_ptr->AddNode(id);
    }
    // else if (id_2_first_g_[id] == g_ptr) {
    //   // LOG(ERROR)<<"[FD] node exists: "<<id <<" g round:"<< round << "with: " << txn_id_proposers_[id][round].size();
    //   continue;
    // }
    // else if (!IsSolid(key.first, key.second)) {
    //   // LOG(ERROR)<<"[FD] add duplicate shaded node: "<<id <<" g round:"<< round;
    //   g_ptr->AddNode(id);
    // } 
    // else {
    //   LOG(ERROR)<<"[F2] already added: "<<id <<" g round:"<< round;
    // }
  }

  // LOG(ERROR) << "Weight Calculation";

  // Weight Calculation between txns in this local ordering
  for(uint64_t i = 0; i < txn_ids.size(); ++i){
    uint64_t id1 = txn_ids[i];
    for(uint64_t j = i+1; j < txn_ids.size(); ++j){
      uint64_t id2 = txn_ids[j];
      auto e = std::make_pair(id1, id2);
      weight_[e].insert(proposer);
      compared_txn_ids_[id1].insert(id2);
      if (weight_[e].size() >= shaded_threshold_) {
        // LOG(ERROR) << "[AE2]: "<< id1 << " " << id2;
        possible_edges.insert(e);
      }
    }
  }

  // LOG(ERROR) << "Weight Calculation2";

  // Weight Calculation between txns in this local ordering and the uncommitted txns in previous local orderings
  for(auto& id2: txn_ids){
    for(auto& id1: pending_txn_ids_[proposer]){
        auto e = std::make_pair(id1, id2);
        weight_[e].insert(proposer);
        compared_txn_ids_[id1].insert(id2);
        if (weight_[e].size() >= shaded_threshold_) {
          // LOG(ERROR) << "e with id id1: " << id1 << " id2: " << id2 << " weight: " << weight_[e].size();
          possible_edges.insert(e);
        }
    }
  }

  // LOG(ERROR) << "Append Pending Txn";

  // Append Pending Transactions
  for(auto id: txn_ids){
    pending_txn_ids_[proposer].insert(id);
    all_pending_txn_ids_.insert(id);
  }

  // LOG(ERROR) << "DONE";
}

void FairDAG::AddEdges(std::set<std::pair<uint64_t,uint64_t>>& possible_edges) {
  for(auto e: possible_edges){
    // LOG(ERROR)  << "edge " << e.first << " " <<  e.second << " " << id_2_first_g_[e.first]->Round() << " " <<  id_2_first_g_[e.second]->Round();

    if (id_2_first_g_.find(e.first) == id_2_first_g_.end() 
        || id_2_first_g_.find(e.second) == id_2_first_g_.end() 
        || id_2_first_g_[e.first] != id_2_first_g_[e.second]) 
    {
      continue;
    }

    uint64_t id1 = e.first;
    uint64_t id2 = e.second;
    auto re = std::make_pair(id2, id1);
    auto g = id_2_first_g_[e.first];
    if (g == nullptr) {
      LOG(ERROR) << "id1: " << id1 << " id2: " << id2;
      fflush(stdout);
      assert(g != nullptr);
    }
    

    uint64_t w1 = weight_.find(e) != weight_.end() ? weight_[e].size() : 0;
    uint64_t w2 = weight_.find(re) != weight_.end() ? weight_[re].size() : 0;

    assert(w1 >= shaded_threshold_ || w2 >= shaded_threshold_);
    bool direction = (w1 >= w2);

    if(direction){
      g->AddEdge(id1, id2);
      // LOG(ERROR)<<" add edge id1:"<<id1<<" id2:"<<id2<<" into g_round: "<<g->Round();
    }
    else {
      g->AddEdge(id2, id1);
      // LOG(ERROR)<<" add edge id2:"<<id2<<" id1:"<<id1<<" into g_round: "<<g->Round();
    }
    // LOG(ERROR) << "[X] AddEdges Done for " << ++i << " in " <<  possible_edges.size();
  }
}

void FairDAG::Push_Committed_LO (std::unique_ptr<std::vector<std::unique_ptr<Transaction>>> local_orderings) {
  committed_lo_queue_.Push(std::move(local_orderings));
}

void FairDAG::AsyncProcessCommittedLO () {
  while(!IsStop()){
   auto local_orderings = committed_lo_queue_.Pop();
   if (local_orderings != nullptr) {
    ProcessCommitedLO(*local_orderings);
   }
  }
}


void FairDAG::ProcessCommitedLO(const std::vector<std::unique_ptr<Transaction>> & local_orderings) {
#ifndef IMPL
  int graph_round = 0;
  auto g = std::make_unique<Graph>();
  Graph * g_ptr = g.get();

  g_set_.insert(g_ptr);
  g_queue_.push(std::move(g)); 

  std::vector<Transaction*> lo_txn_list;
  for(auto& txn : local_orderings){
    graph_round = std::max(graph_round, static_cast<int>(txn->round()));
    lo_txn_list.push_back(txn.get()); 
  }
  g_ptr->SetRound(graph_round);
  ConstructDependencyGraph(g_ptr, lo_txn_list);
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

void FairDAG::ConstructDependencyGraph(Graph * g, const std::vector<Transaction*>& lo_txn_list) {
  assert(g!=nullptr);
  int proposal_id = -1;
  std::vector<Transaction*> local_ordering;
  std::set<std::pair<uint64_t, uint64_t>> possible_edges;
  for(uint64_t i = 0; i < lo_txn_list.size(); ++i){
    if(IsCommitted(*lo_txn_list[i])){
      continue;
    }
    // int proposer = lo_txn_list[i]->proposer();
    if(proposal_id >=0 && lo_txn_list[i]->proposal_id() != proposal_id){
      // [DK] Add the transactions that are from different replicas one by one
      AddLocalOrdering(g, local_ordering, possible_edges);
      local_ordering.clear();
    }
    local_ordering.push_back(lo_txn_list[i]);
    proposal_id = lo_txn_list[i]->proposal_id();
  }
  if(local_ordering.size()>0){
    AddLocalOrdering(g, local_ordering, possible_edges);
  }

  AddImplictWeight(g, possible_edges);

  uint64_t round = g->Round();
  for (auto & id : round_solid_[round]) {
    SetSolid(round, id);
  }
  round_solid_[round].clear();
  round_solid_.erase(round);
  LOG(ERROR) << "Start Adding Edges";
  AddEdges(possible_edges);
  LOG(ERROR) << "End Adding Edges";
}

void FairDAG::FindPossibleEdges(Graph* g, uint64_t id1, std::set<std::pair<uint64_t,uint64_t>>& possible_edges, std::vector<uint64_t>& ids){
  if(ids.size() == 0) {
    auto allnode = g->GetAllNode();
    for (auto v: allnode) {
      ids.push_back(v);
    }
  }
  for (auto id2: ids) {
    if (id1 != id2) {
      auto e = std::make_pair(id1, id2);
      auto re = std::make_pair(id2, id1);
      if (weight_.find(e) != weight_.end() && weight_[e].size() >= shaded_threshold_) {
        possible_edges.insert(e);
      } 
      else if (weight_.find(re) != weight_.end() && weight_[re].size() >= shaded_threshold_) 
      {
        possible_edges.insert(re);
      }
    }
  }
  ids.push_back(id1);
}

void FairDAG::CheckGraph(){
  LOG(ERROR)<<"start";
  while(!g_queue_.empty()){
    Graph * g = g_queue_.front().get();
    if (txn_to_add_ids_.size() > 0) {
      std::vector<uint64_t> nodes;
      for (auto &id: txn_to_add_ids_) {
        auto key = proposals_idx_2_key_[id];
        LOG(ERROR) << "ReAdd " << id << " " << key.first << " " << key.second  << " to round " << g->Round();
        g->AddNode(id);
        id_2_first_g_[id] = g;
        std::set<std::pair<uint64_t,uint64_t>> possible_edges;
        FindPossibleEdges(g, id, possible_edges, nodes);
        AddEdges(possible_edges);
      }
      txn_to_add_ids_.clear();
      nodes.clear();
    }

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
      auto orders = FinalOrder(g);
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


}  // namespace tusk
}  // namespace resdb

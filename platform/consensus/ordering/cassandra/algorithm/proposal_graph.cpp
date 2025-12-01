#include "platform/consensus/ordering/cassandra/algorithm/proposal_graph.h"

#include <glog/logging.h>

#include <queue>
#include <stack>

#include "common/utils/utils.h"

namespace resdb {
namespace cassandra {
namespace cassandra_recv {

/*
std::vector<ProposalState> GetStates() {
  return std::vector<ProposalState>{ProposalState::New, ProposalState::Prepared,
                                    ProposalState::PreCommit};
}
*/

ProposalGraph::ProposalGraph(int fault_num, int id, int total_num) : f_(fault_num),id_(id), total_num_(total_num) {
  ranking_ = std::make_unique<Ranking>();
  current_height_ = 0;
  global_stats_ = Stats::GetGlobalStats();
}

int ProposalGraph::GetBlockNum(const std::string& hash, int local_id, int proposer_id) {
if(num_callback_) {
  return num_callback_(hash, local_id, proposer_id);
}

  return 0;
}

void ProposalGraph::IncreaseHeight() {
  //LOG(ERROR) << "increase height:" << current_height_;
  current_height_++;
}

void ProposalGraph::TryUpgradeHeight(int height) {
  if (last_node_[height].size() > 0) {
    // LOG(ERROR) << "upgrade height:" << height;
    current_height_ = height;
  } else {
    // assert(1 == 0);
    LOG(ERROR) << "need to recovery";
  }
}

std::string Encode(const std::string& hash) {
  std::string ret;
  for (int i = 0; i < hash.size(); ++i) {
    int x = hash[i];
    ret += std::to_string(x);
  }
  return ret;
}

void ProposalGraph::AddProposalOnly(const Proposal& proposal) {
  auto it = node_info_.find(proposal.header().hash());
  if (it == node_info_.end()) {
    auto np = std::make_unique<NodeInfo>(proposal);
    node_info_[proposal.header().hash()] = std::move(np);
    //LOG(ERROR) << "add proposal proposer:" << proposal.header().proposer_id()
    //           << " id:" << proposal.header().proposal_id()
    //           << " hash:" << Encode(proposal.header().hash());
  }
}

int ProposalGraph::AddProposal(const Proposal& proposal) {
   LOG(ERROR) << "add proposal height:" << proposal.header().height()
             << " current height:" << current_height_<<" from:"<<proposal.header().proposer_id()<<" proposal id:"<<proposal.header().proposal_id();
  assert(current_height_ >= latest_commit_.header().height());


  /*
  if (proposal.header().height() < current_height_) {
    LOG(ERROR) << "height not match:" << current_height_
               << " proposal height:" << proposal.header().height();
    return false;
  }
  */

  if (proposal.header().height() > current_height_) {
    pending_header_[proposal.header().height()].insert(
        proposal.header().proposer_id());
  } else {
    while (!pending_header_.empty()) {
    LOG(ERROR)<<" pending heade:"<<pending_header_.begin()->first<<" current height:"<<current_height_;
      if (pending_header_.begin()->first <= current_height_) {
        pending_header_.erase(pending_header_.begin());
      } else {
        break;
      }
    }
  }

  if (proposal.header().height() > current_height_ + 1) {
    LOG(ERROR) << "height not match:" << current_height_
               << " proposal height:" << proposal.header().height();
    if (pending_header_[proposal.header().height()].size() >= f_ + 1) {
      TryUpgradeHeight(proposal.header().height());
    }
    return 1;
  }

  if (!VerifyParent(proposal)) {
    LOG(ERROR) << "verify parent fail:" << proposal.header().proposer_id()
               << " id:" << proposal.header().proposal_id();

  #ifdef GOPOA
    g_[proposal.header().prehash()].push_back(proposal.header().hash());
    auto np = std::make_unique<NodeInfo>(proposal);
    //new_proposals_[proposal.header().hash()] = &np->proposal;
    // LOG(ERROR)<<"add proposal proposer:"<<proposal.header().proposer_id()<<"
    // id:"<<proposal.header().proposal_id()<<"
    // hash:"<<Encode(proposal.header().hash());
    node_info_[proposal.header().hash()] = std::move(np);
    last_node_[proposal.header().height()].insert(proposal.header().hash());
  #endif

    // assert(1==0);
    return 2;
  }

  //LOG(ERROR)<<"history size:"<<proposal.history_size();
   /*
  for (const auto& history : proposal.history()) {
    std::string hash = history.hash();
    auto node_it = node_info_.find(hash);
    assert(node_it != node_info_.end());
  }
  */

  //LOG(ERROR)<<" proposal history:"<<proposal.history_size();
  if(proposal.history_size()>0){
    const auto& history = proposal.history(0);
    std::string hash = history.hash();
    auto node_it = node_info_.find(hash);
    node_it->second->votes[ProposalState::New].insert(proposal.header().proposer_id());
    CheckState(node_it->second.get(),
        static_cast<resdb::cassandra::ProposalState>(history.state()));

    if (node_it->second->state == ProposalState::PoR 
      && node_it->second->proposal.header().proposer_id() == id_){
      //LOG(ERROR)<<" remove por blocks:"<<node_it->second->proposal.sub_block_size();
      std::set<std::string> exist;
      for(auto block : node_it->second->proposal.sub_block()) {
        if(new_blocks_.find(block.hash()) != new_blocks_.end()){
          new_blocks_.erase(new_blocks_.find(block.hash()));
          exist.insert(block.hash());
        }
      }

      //RemoveBlocks(node_it->second->proposal);
    }

    int num = 0;
    int cur_h = proposal.header().height();
    for(int i = 0; i <3 && proposal.history_size()>=3; ++i){
      const auto& sub_history = proposal.history(i);
      std::string sub_hash = sub_history.hash();
      auto sub_node_it = node_info_.find(sub_hash);
      if(sub_node_it == node_info_.end()){
        continue;
      }
      //LOG(ERROR)<<" state:"<<sub_node_it->second->state<<" node:"<<sub_node_it->second->proposal.header().proposer_id()
      //<<" height:"<<sub_node_it->second->proposal.header().height();
      assert(cur_h == sub_node_it->second->proposal.header().height()+1);
      cur_h--;
    // LOG(ERROR)<<"proposal:("<<node_it->second->proposal.header().proposer_id()
      if (sub_node_it->second->state != ProposalState::PoR) {
        break;
      }
      num++;
    }
  
    //LOG(ERROR)<<"get num:"<<num; 
    if(num == 3) {
      const auto& sub_history = proposal.history(2);
      std::string sub_hash = sub_history.hash();

      auto sub_node_it = node_info_.find(sub_hash);
      if(sub_node_it != node_info_.end()){
        //LOG(ERROR)<<" state:"<<node_it->second->state;
        if (sub_node_it->second->state != ProposalState::Committed) {
          Commit(sub_hash);
        }
      }
    }
  }

    //LOG(ERROR) << "height:" << current_height_
     //          << " proposal height:" << proposal.header().height();
  if (proposal.header().height() < current_height_) {
    LOG(ERROR) << "height not match:" << current_height_
               << " proposal height:" << proposal.header().height();

    // LOG(ERROR)<<"add proposal proposer:"<<proposal.header().proposer_id()<<"
    // id:"<<proposal.header().proposal_id();
    // g_[proposal.header().prehash()].push_back(proposal.header().hash());
    auto np = std::make_unique<NodeInfo>(proposal);
    //new_proposals_[proposal.header().hash()] = &np->proposal;
    node_info_[proposal.header().hash()] = std::move(np);
    last_node_[proposal.header().height()].insert(proposal.header().hash());

    return 1;
  } else {
    g_[proposal.header().prehash()].push_back(proposal.header().hash());
    auto np = std::make_unique<NodeInfo>(proposal);
    //new_proposals_[proposal.header().hash()] = &np->proposal;
    //LOG(ERROR)<<"add proposal proposer:"<<proposal.header().proposer_id()<<" id:"<<proposal.header().proposal_id()<<" hash:"<<Encode(proposal.header().hash());
    node_info_[proposal.header().hash()] = std::move(np);
    last_node_[proposal.header().height()].insert(proposal.header().hash());
  }

  //LOG(ERROR)<<"add graph done";
  return 0;
}

void ProposalGraph::UpgradeState(ProposalState& state) {
return;
}

int ProposalGraph::CheckState(NodeInfo* node_info, ProposalState state) {
   //LOG(ERROR) << "node: (" << node_info->proposal.header().proposer_id() <<
   //","
    //         << node_info->proposal.header().proposal_id()
     //        << ") state:" << node_info->state
      //       << " vote num:" << node_info->votes[ProposalState::New].size();

  if(node_info->votes[ProposalState::New].size() >= 2 * f_ + 1) {
      state = PoR;
  }
  else if(node_info->votes[ProposalState::New].size() >= f_ + 1) {
      state = PoA;
  }
  else {
      state = New;
  }
  node_info->state = state;
  LOG(ERROR) << "node: (" << node_info->proposal.header().proposer_id() <<
   ","
             << node_info->proposal.header().proposal_id()
             << ") get state:" << node_info->state
             << " vote num:" << node_info->votes[ProposalState::New].size();


  return true;
}

void ProposalGraph::Commit(const std::string& hash) {
  auto it = node_info_.find(hash);
  if (it == node_info_.end()) {
    LOG(ERROR) << "node not found, hash:" << hash;
    assert(1 == 0);
    return;
  }

 // LOG(ERROR) << "commit, hash:";
  std::set<std::string> is_main_hash;
  is_main_hash.insert(hash);
  
  int from_proposer = it->second->proposal.header().proposer_id();
  int from_proposal_id = it->second->proposal.header().proposal_id();
  LOG(ERROR)<<"commit :"<<it->second->proposal.header().proposer_id()<<" id:"<<it->second->proposal.header().proposal_id();

  std::vector<std::vector<Proposal*>> commit_p;
  auto bfs = [&]() {
    std::queue<std::string> q;
    q.push(hash);
    while (!q.empty()) {
      std::string c_hash = q.front();
      q.pop();

      auto it = node_info_.find(c_hash);
      if (it == node_info_.end()) {
        LOG(ERROR) << "node not found, hash:";
  #ifdef GOPOA
        commit_p.clear();
        break;
  #endif
        assert(1 == 0);
      }

      Proposal* p = &it->second->proposal;
      if (it->second->state == ProposalState::Committed) {
        continue;
      }

      //LOG(ERROR)<<" bfs sub block size :"<<p->sub_block_size();
      /*
      for(auto block : p->sub_block()){
        LOG(ERROR)<<" get sub block proposer:"<<p->header().proposer_id()<<" local id:"<<block.local_id();
      }
    */
      it->second->state = ProposalState::Committed;
      if (is_main_hash.find(c_hash) != is_main_hash.end()) {
        commit_num_[p->header().proposer_id()]++;
        // LOG(ERROR)<<"commit main node:"<<p->header().proposer_id();
        is_main_hash.insert(p->header().prehash());
        commit_p.push_back(std::vector<Proposal*>());
      }

      commit_p.back().push_back(p);
      if (!p->header().prehash().empty()) {
        q.push(p->header().prehash());
      }
    }
  };

  bfs();
  if (commit_p.size() > 1) {
    LOG(ERROR) << "commit more hash";
  }
  if(commit_p.size()==0) {
    LOG(ERROR) << "commit not ready";
    return;
  }
  assert(commit_p.size()>0);
  int block_num = 0;
  int p_num = 0;
  for (int i = commit_p.size() - 1; i >= 0; i--) {
    for (int j = 0; j < commit_p[i].size(); ++j) {
      /*
      if (j == 0) {
        LOG(ERROR) << "commmit proposal lead from:"
                   << commit_p[i][j]->header().proposer_id()
                   << " height:" << commit_p[i][j]->header().height()
                   << " size:" << commit_p[i].size();
      }
      */
      //LOG(ERROR) << "commmit proposal:"
       //          << commit_p[i][j]->header().proposer_id()
        //         << " height:" << commit_p[i][j]->header().height()
         //        << " idx:" << j 
          //       << " delay:" << (GetCurrentTime() - commit_p[i][j]->create_time()) 
           //      << " commit from:"<< from_proposer<<" id:"<<from_proposal_id;
      block_num += commit_p[i][j]->sub_block_size();

      LOG(ERROR) << "commmit proposal from:" << commit_p[i][j]->header().proposer_id()
                   << " height:" << commit_p[i][j]->header().height()
                   << " block size:" << commit_p[i][j]->sub_block_size();



      for(auto block : commit_p[i][j]->sub_block()){
        //LOG(ERROR) << "commmit proposal from:" << commit_p[i][j]->header().proposer_id()
                   //<< " block id:" << block.local_id();
        if(check_.find(std::make_pair(block.local_id(), commit_p[i][j]->header().proposer_id())) != check_.end()){
          //LOG(ERROR) << "commmit proposal from:" << commit_p[i][j]->header().proposer_id()
          //         << " block id:" << block.local_id() << "has committed";
        }
        else {
          check_.insert(std::make_pair(block.local_id(), commit_p[i][j]->header().proposer_id()));
          //LOG(ERROR)<<" trans size:"<<block.data().transaction_size();
          p_num+=GetBlockNum(block.hash(), block.local_id(), commit_p[i][j]->header().proposer_id());
        }
      }

      if (commit_callback_) {
        commit_callback_(*commit_p[i][j]);
      }
    }
  }
  //global_stats_->AddCommitBlock(block_num);
  global_stats_->AddCommitTxn(p_num);
  //LOG(ERROR)<<" commit proposal num:"<<p_num;
  LOG(ERROR)<<"commit proposal from :"<<it->second->proposal.header().proposer_id()<<" id:"<<it->second->proposal.header().proposal_id()<<" height:"<<it->second->proposal.header().height()<<" num:"<<p_num;
  // TODO clean
  last_node_[it->second->proposal.header().height()].clear();
  latest_commit_ = it->second->proposal;
  it->second->state = ProposalState::Committed;
  // Clear(latest_commit_.header().hash());
}

std::vector<std::unique_ptr<Proposal>> ProposalGraph::GetNotFound(
    int height, const std::string& hash) {
  auto it = not_found_proposal_.find(height);
  if (it == not_found_proposal_.end()) {
    return std::vector<std::unique_ptr<Proposal>>();
  }
  auto pre_it = it->second.find(hash);
  std::vector<std::unique_ptr<Proposal>> ret;
  if (pre_it != it->second.end()) {
    ret = std::move(pre_it->second);
    it->second.erase(pre_it);
    //LOG(ERROR) << "found future height:" << height;
  }
  return ret;
}

bool ProposalGraph::VerifyParent(const Proposal& proposal) {
  // LOG(ERROR) << "last commit:" << latest_commit_.header().proposal_id()
  //           << " current :" << proposal.header().proposal_id()
  //           << " height:" << proposal.header().height();

  if (proposal.header().prehash() == latest_commit_.header().hash()) {
    return true;
  }

  std::string prehash = proposal.header().prehash();
  // LOG(ERROR)<<"prehash:"<<prehash;

  auto it = node_info_.find(prehash);
  if (it == node_info_.end()) {
  //  LOG(ERROR) << "prehash not here";
    not_found_proposal_[proposal.header().height()][proposal.header().prehash()]
        .push_back(std::make_unique<Proposal>(proposal));
    return false;
  } else {
    if (proposal.header().height() !=
        it->second->proposal.header().height() + 1) {
      LOG(ERROR) << "link to invalid proposal, height:"
                 << proposal.header().height()
                 << " pre height:" << it->second->proposal.header().height();
      return false;
    }
  }
  return true;
}

void ProposalGraph::UpdateHistory(Proposal* proposal) {
  proposal->mutable_history()->Clear();
  std::string hash = proposal->header().hash();

  for (int i = 0; i < 3 && !hash.empty(); ++i) {
    auto node_it = node_info_.find(hash);
    if(node_it == node_info_.end()){
      break;
    }
    auto his = proposal->add_history();
    his->set_hash(hash);
    his->set_state(node_it->second->state);
    his->set_sender(node_it->second->proposal.header().proposer_id());
    his->set_id(node_it->second->proposal.header().proposal_id());
    hash = node_it->second->proposal.header().prehash();
    //LOG(ERROR)<<" get his proposer:"<<node_it->second->proposal.header().proposer_id()<<" height:"<<node_it->second->proposal.header().height();
  }
}

Proposal* ProposalGraph::GetStrongestProposal() {
  LOG(ERROR) << "get strong proposal from height:" << current_height_;
  if (last_node_.find(current_height_) == last_node_.end()) {
    LOG(ERROR) << "no data:" << current_height_;
    return nullptr;
  }

  //LOG(ERROR)<<" node size:"<<last_node_[current_height_].size();
  NodeInfo* sp = nullptr;
  for (const auto& last_hash : last_node_[current_height_]) {
    if(node_info_.find(last_hash) == node_info_.end()){
      continue;
    }
    NodeInfo* node_info = node_info_[last_hash].get();
    assert(node_info->proposal.header().height() == current_height_);
    if (sp == nullptr || Compare(*sp, *node_info)) {
      sp = node_info;
    }
  }
  assert(sp != nullptr);

  LOG(ERROR)<<" last node size:"<<last_node_[current_height_].size()<<" height:"<<current_height_<<" get strong from:"<<sp->proposal.header().proposer_id();

  for (const auto& last_hash : last_node_[current_height_]) {
    if(node_info_.find(last_hash) == node_info_.end()){
      continue;
    }
    NodeInfo* node_info = node_info_[last_hash].get();
  //  LOG(ERROR)<<" node info:"<<node_info->proposal.header().proposer_id()<<" sub blocks:"<<node_info->proposal.sub_block_size();

    if(node_info->proposal.header().proposer_id() != id_){
      continue;
    }

    //LOG(ERROR)<<" node info:"<<node_info->proposal.header().proposer_id()<<" sub blocks:"<<node_info->proposal.sub_block_size()<<" node state:"<<node_info->state;
    if(node_info->state == ProposalState::PoR) {
      assert(node_info->proposal.header().proposer_id() == sp->proposal.header().proposer_id());
      for(auto sub_block : node_info->proposal.sub_block()){ 
        if(new_blocks_.find(sub_block.hash()) != new_blocks_.end()){
          new_blocks_.erase(new_blocks_.find(sub_block.hash()));
        }
      }
      continue;
    }

    //LOG(ERROR)<<"get sub block size:"<<node_info->proposal.sub_block_size();
    for(auto sub_block : node_info->proposal.sub_block()){ 
      new_blocks_[sub_block.hash()] = sub_block;
    }
    //LOG(ERROR)<<" new blocks:"<<new_blocks_.size();


    std::string pre_prehash = node_info->proposal.header().prehash();
    if(node_info_.find(pre_prehash) != node_info_.end()){
      NodeInfo* pre_node_info = node_info_[pre_prehash].get();
      //LOG(ERROR)<<" pre node info:"<<pre_node_info->proposal.header().proposer_id()
      //<<" pre node state:"<<pre_node_info->state;

      if(pre_node_info->state == ProposalState::PoR 
          && pre_node_info->proposal.header().proposer_id() == id_) {
        for(auto sub_block : pre_node_info->proposal.sub_block()){ 
          if(new_blocks_.find(sub_block.hash()) != new_blocks_.end()){
            //LOG(ERROR)<<" remove new blocks:"<<sub_block.local_id(); 
            new_blocks_.erase(new_blocks_.find(sub_block.hash()));
          }
        }
      }
    }

    /*
    if(node_info->proposal.header().proposer_id() == sp->proposal.header().proposer_id()) {
      continue;
    }

    if(sp == node_info) {
      continue;
    }
    */

        /*
    if(node_info_.find(last_hash) != node_info_.end()){
      node_info_.erase(node_info_.find(last_hash));
    }
    */
  }



  //LOG(ERROR)<<" update his";
  UpdateHistory(&sp->proposal);
   //LOG(ERROR) << "get strong proposal from height:" << current_height_ << " ->("
   //          << sp->proposal.header().proposer_id() << ","
   //          << sp->proposal.header().proposal_id() << ")";
  return &sp->proposal;
}

bool ProposalGraph::Cmp(int id1, int id2) {
   //LOG(ERROR) << "commit commit num:" << id1 << " " << id2
   //          << " commit  time:" << commit_num_[id1] << " " <<
   //          commit_num_[id2];
  if (commit_num_[id1]  < commit_num_[id2]) {
    return false;
  }

  if (commit_num_[id1] > commit_num_[id2] ) {
    return true;
  }
  return id1 < id2;
}

int ProposalGraph::StateScore(const ProposalState& state) {
  // return state == ProposalState::Prepared? 1:0;
  return state;
}

int ProposalGraph::CompareState(const ProposalState& state1,
                                const ProposalState& state2) {
  // LOG(ERROR) << "check state:" << state1 << " " << state2;
  return StateScore(state1) - StateScore(state2);
}

// p1 < p2
bool ProposalGraph::Compare(const NodeInfo& p1, const NodeInfo& p2) {
  //LOG(ERROR) << "proposer:" << p1.proposal.header().proposer_id() << " "
  //           << p2.proposal.header().proposer_id()
  //          << "height:" << p1.proposal.header().height() << " "
  //          << p2.proposal.header().height()
  //          <<" state:"<< p1.state<<" "<<p2.state
  //         <<" hash cmp:"<< (p1.proposal.header().hash() < p2.proposal.header().hash())
  //         <<" cmp num:" << Cmp(p1.proposal.header().proposer_id(), p2.proposal.header().proposer_id())
  //         <<" sub block:" << p1.proposal.sub_block_size() <<" "<< p2.proposal.sub_block_size();
  if (p1.proposal.header().height() != p2.proposal.header().height()) {
    return p1.proposal.header().height() < p2.proposal.header().height();
  }
  // LOG(ERROR)<<"proposer:"<<p1.proposal.header().proposer_id()<<"
  // "<<p2.proposal.header().proposer_id();
  if (CompareState(p1.state, p2.state) != 0) {
    return CompareState(p1.state, p2.state) < 0;
  }

  #ifdef GOPOA
  if(850<=p1.proposal.header().height()&&p1.proposal.header().height()<=1350) {
    int h = (p1.proposal.header().height())%2;
    if ( h == 0) h = 2;
    return abs(p1.proposal.header().proposer_id() - h ) > abs(p2.proposal.header().proposer_id() - h);
  }
  #endif

  int h = (p1.proposal.header().height())%total_num_;
  if ( h == 0) h = total_num_;
  //LOG(ERROR)<<" check height :"<<h<<" cmp:"<<abs(p1.proposal.header().proposer_id() - h )<<" "<<abs(p2.proposal.header().proposer_id() - h);
  //if (p1.proposal.header().height() <= 120 && 220 <= proposal.header().height()) {
    return abs(p1.proposal.header().proposer_id() - h ) > abs(p2.proposal.header().proposer_id() - h);
  //}

  if (abs(p1.proposal.sub_block_size() - p2.proposal.sub_block_size()) > 5) {
    //return p1.proposal.sub_block_size() < p2.proposal.sub_block_size();
  }
  return p1.proposal.header().hash() < p2.proposal.header().hash();

  if (p1.proposal.header().proposer_id() ==
      p2.proposal.header().proposer_id()) {
    return p1.proposal.header().proposal_id() <
           p2.proposal.header().proposal_id();
  }

  return Cmp(p1.proposal.header().proposer_id(),
             p2.proposal.header().proposer_id());
}

Proposal* ProposalGraph::GetLatestStrongestProposal() {
  Proposal* sp = GetStrongestProposal();
  if (sp == nullptr) {
    if (current_height_ > 0) {
      assert(1 == 0);
    }
    return &latest_commit_;
  }
  // LOG(ERROR) << "====== get strong proposal from:" <<
  // sp->header().proposer_id()
  //           << " id:" << sp->header().proposal_id();
  return sp;
}

ProposalState ProposalGraph::GetProposalState(const std::string& hash) const {
  auto node_it = node_info_.find(hash);
  if (node_it == node_info_.end()) {
    return ProposalState::New;
  }
  return node_it->second->state;
}

const Proposal* ProposalGraph::GetProposalInfo(const std::string& hash) const {
  auto it = node_info_.find(hash);
  if (it == node_info_.end()) {
    LOG(ERROR) << "hash not found:" << Encode(hash);
    return nullptr;
  }
  return &it->second->proposal;
}

int ProposalGraph::GetCurrentHeight() { return current_height_; }

std::vector<Proposal*> ProposalGraph::GetNewProposals(int height) {
  std::vector<Proposal*> ps;
  for (auto it : new_proposals_) {
  /*
    if (it.second->header().height() >= height) {
      continue;
    }
    */
    ps.push_back(it.second);
  }
  for (Proposal* p : ps) {
    new_proposals_.erase(new_proposals_.find(p->header().hash()));
  }
  return ps;
}

std::vector<Block> ProposalGraph::GetNewBlocks() {
  std::vector<Block> ps;
  for (auto it : new_blocks_) {
    ps.push_back(it.second);
  }
  //new_blocks_.clear();
  return ps;
}


}  // namespace cassandra_recv
}  // namespace cassandra
}  // namespace resdb

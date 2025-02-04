#include "platform/consensus/ordering/cassandra/algorithm/basic/proposal_graph.h"

#include <glog/logging.h>

#include <stack>

namespace resdb {
namespace cassandra {
namespace basic {

std::vector<ProposalState> GetStates() {
  return std::vector<ProposalState>{ProposalState::New, ProposalState::Prepared,
                                    ProposalState::PreCommit};
}

ProposalGraph::ProposalGraph(int fault_num) : f_(fault_num) {
  ranking_ = std::make_unique<Ranking>();
  current_height_ = 0;
}

void ProposalGraph::IncreaseHeight() {
  LOG(ERROR) << "increase height:" << current_height_;
  current_height_++;
}

void ProposalGraph::TryUpgradeHeight(int height) {
  if (last_node_[height].size() > 0) {
    LOG(ERROR) << "upgrade height:" << height;
    current_height_ = height;
  } else {
    assert(1 == 0);
    LOG(ERROR) << "need to recovery";
  }
}

bool ProposalGraph::AddProposal(const Proposal& proposal) {
  // LOG(ERROR) << "add proposal height:" << proposal.header().height()
  //           << " current height:" << current_height_;
  assert(current_height_ >= latest_commit_.header().height());
  if (proposal.header().height() < current_height_) {
    LOG(ERROR) << "height not match:" << current_height_
               << " proposal height:" << proposal.header().height();
    return false;
  }

  if (proposal.header().height() > current_height_) {
    pending_header_[proposal.header().height()].insert(
        proposal.header().proposer_id());
  } else {
    while (!pending_header_.empty()) {
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
    return false;
  }

  if (!VerifyParent(proposal)) {
    LOG(ERROR) << "verify parent fail:" << proposal.header().proposer_id()
               << " id:" << proposal.header().proposal_id();
    // assert(1==0);
    return false;
  }

  // LOG(ERROR)<<"history size:"<<proposal.history_size();
  for (const auto& history : proposal.history()) {
    std::string hash = history.hash();
    auto node_it = node_info_.find(hash);
    if (node_it == node_info_.end()) {
      LOG(ERROR) << " history node not found";
      return false;
    }
  }

  for (const auto& history : proposal.history()) {
    std::string hash = history.hash();
    auto node_it = node_info_.find(hash);
    for (auto s : GetStates()) {
      if (s >= node_it->second->state && s <= history.state()) {
        if (s != history.state()) {
          LOG(ERROR) << "update from higher state";
        }
        node_it->second->votes[s].insert(proposal.header().proposer_id());
      }
    }
    // LOG(ERROR)<<"proposal:("<<node_it->second->proposal.header().proposer_id()<<","<<node_it->second->proposal.header().proposal_id()<<")"<<"
    // history state:"<<history.state()<<"
    // num:"<<node_it->second->votes[history.state()].size();
    CheckState(node_it->second.get(),
               static_cast<resdb::cassandra::ProposalState>(history.state()));
    if (node_it->second->state == ProposalState::PreCommit) {
      Commit(hash);
    }
    // LOG(ERROR)<<"history
    // proposal:("<<node_it->second->proposal.header().proposer_id()
    //<<","<<node_it->second->proposal.header().proposal_id()
    //<<" state:"<<node_it->second->state;
  }

  g_[proposal.header().prehash()].push_back(proposal.header().hash());
  node_info_[proposal.header().hash()] = std::make_unique<NodeInfo>(proposal);
  last_node_[proposal.header().height()].insert(proposal.header().hash());
  return true;
}

void ProposalGraph::UpgradeState(ProposalState& state) {
  switch (state) {
    case None:
    case New:
      state = Prepared;
      break;
    case Prepared:
      state = PreCommit;
      break;
    default:
      break;
  }
}

int ProposalGraph::CheckState(NodeInfo* node_info, ProposalState state) {
  // LOG(ERROR) << "node: (" << node_info->proposal.header().proposer_id() <<
  // ","
  //           << node_info->proposal.header().proposal_id()
  //           << ") state:" << node_info->state
  //           << " vote num:" << node_info->votes[node_info->state].size();

  while (node_info->votes[node_info->state].size() >= 2 * f_ + 1) {
    UpgradeState(node_info->state);
    // LOG(ERROR) << "==========  proposal:("
    //            << node_info->proposal.header().proposer_id() << ","
    //            << node_info->proposal.header().proposal_id() << ")"
    //            << " upgrate state:" << node_info->state;
  }
  return true;
}

/*
bool ProposalGraph::RecoveryCommit(const Proposal& proposal) {
  if(proposal.header().prehash() != latest_commit_.header().hash()){
    LOG(ERROR)<<"not the next commit from:"<<proposal.header().proposer_id()
    <<" id:"<<proposal.header().proposal_id();
    return false;
  }

  LOG(ERROR)<<"recovery the next commit from:"<<proposal.header().proposer_id()
  <<" id:"<<proposal.header().proposal_id();
  node_info_[proposal.header().hash()] = std::make_unique<NodeInfo>(proposal);
  commit_num_[proposal.header().proposer_id()]++;
  node_info_[proposal.header().hash()]->state = ProposalState::Committed;
  latest_commit_ = proposal;
  last_node_.clear();
  return true;
}
    */

void ProposalGraph::Commit(const std::string& hash) {
  auto it = node_info_.find(hash);
  if (it == node_info_.end()) {
    LOG(ERROR) << "node not found, hash:" << hash;
    assert(1 == 0);
    return;
  }

  // LOG(ERROR)<<"commit :"<<it->second->proposal.proposer_id()<<"
  // num:"<<commit_num_[it->second->proposal.proposer_id()];

  if (it->second->state != ProposalState::PreCommit) {
    LOG(ERROR) << "hash not committed:" << hash;
    assert(1 == 0);
    return;
  }

  commit_num_[it->second->proposal.header().proposer_id()]++;

  // LOG(ERROR)<<"commit :"<<it->second->proposal.header().proposer_id()
  //<<" id:"<<it->second->proposal.header().proposal_id();

  std::stack<Proposal*> commit_p;
  std::string c_hash = hash;
  while (true) {
    auto it = node_info_.find(c_hash);
    if (it == node_info_.end()) {
      LOG(ERROR) << "node not found, hash:" << hash;
      break;
      // assert(1 == 0);
      // return;
    }

    if (it->second->state == ProposalState::Committed) {
      break;
    }

    commit_p.push(&it->second->proposal);
    it->second->state = ProposalState::Committed;
    commit_num_[it->second->proposal.header().proposer_id()]++;
    c_hash = it->second->proposal.header().prehash();
    if (c_hash.empty()) {
      break;
    }
  }

  if (commit_p.size() > 1) {
    LOG(ERROR) << "commit more hash";
  }
  while (!commit_p.empty()) {
    if (commit_callback_) {
      commit_callback_(*commit_p.top());
    }
    commit_p.pop();
  }
  /*
  if (commit_callback_) {
    // LOG(ERROR)<<"!!!!!!!! callback:";
    commit_callback_(it->second->proposal);
  }
  */
  // TODO clean
  last_node_[it->second->proposal.header().height()].clear();
  latest_commit_ = it->second->proposal;
  it->second->state = ProposalState::Committed;
  Clear(latest_commit_.header().hash());
}

void ProposalGraph::Clear(const std::string& h) {
  std::string hash = h;
  for (int i = 0; i < 4; ++i) {
    auto it = node_info_.find(hash);
    if (it == node_info_.end()) {
      LOG(ERROR) << "node not found, hash:" << hash;
      return;
    }

    std::string pre = it->second->proposal.header().prehash();
    if (pre.empty()) {
      return;
    }
    hash = pre;
  }
  {
    auto pre_it = node_info_.find(hash);
    if (pre_it == node_info_.end()) {
      LOG(ERROR) << "node not found, hash:" << hash;
      return;
    }
    node_info_.erase(pre_it);
  }
}

/*
Proposal * ProposalGraph::GetLatestCommitProposal() {
  Proposal * sp = GetStrongestProposal();
  if(sp == nullptr){
    return &latest_commit_;
  }
  return sp;

}

bool ProposalGraph::ChangeState(const std::string& hash, ProposalState state) {
  auto it = node_info_.find(hash);
  if(it == node_info_.end()){
    LOG(ERROR)<<"node not found, hash:"<<hash;
    return false;
  }
  //LOG(ERROR)<<"change state hash:"<<hash<<" state:"<<state;
  if(state == ProposalState::Committed && it->second->state !=
ProposalState::Committed){
  //TODO clean
    LOG(ERROR)<<"set last commit";
    latest_commit_ = it->second->proposal;
  }
  it->second->state = state;
  return true;
}

std::string ProposalGraph::GetLatestCommit()const{
  return latest_commit_.header().hash();
}
*/

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
    LOG(ERROR) << "found future height:" << height;
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
    LOG(ERROR) << "prehash not here";
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
    auto his = proposal->add_history();
    his->set_hash(hash);
    his->set_state(node_it->second->state);
    hash = node_it->second->proposal.header().prehash();
  }
}

Proposal* ProposalGraph::GetStrongestProposal() {
  // LOG(ERROR) << "get strong proposal from height:" << current_height_;
  if (last_node_.find(current_height_) == last_node_.end()) {
    LOG(ERROR) << "no data";
    return nullptr;
  }

  NodeInfo* sp = nullptr;
  for (const auto& last_hash : last_node_[current_height_]) {
    NodeInfo* node_info = node_info_[last_hash].get();
    if (sp == nullptr || Compare(*sp, *node_info)) {
      sp = node_info;
    }
  }

  UpdateHistory(&sp->proposal);
  // LOG(ERROR) << "get strong proposal from height:" << current_height_ << "
  // ->("
  //           << sp->proposal.header().proposer_id() << ","
  //           << sp->proposal.header().proposal_id() << ")";
  return &sp->proposal;
}

bool ProposalGraph::Cmp(int id1, int id2) {
  // LOG(ERROR) << "commit commit num:" << id1 << " " << id2
  //           << " commit  time:" << commit_num_[id1] << " " <<
  //           commit_num_[id2];
  if (commit_num_[id1] + 1 < commit_num_[id2]) {
    return false;
  }

  if (commit_num_[id1] > commit_num_[id2] + 1) {
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
  // LOG(ERROR) << "proposer:" << p1.proposal.header().proposer_id() << " "
  //           << p2.proposal.header().proposer_id()
  //          << "height:" << p1.proposal.header().height() << " "
  //          << p2.proposal.header().height();
  if (p1.proposal.header().height() != p2.proposal.header().height()) {
    return p1.proposal.header().height() < p2.proposal.header().height();
  }
  // LOG(ERROR)<<"proposer:"<<p1.proposal.header().proposer_id()<<"
  // "<<p2.proposal.header().proposer_id();
  if (CompareState(p1.state, p2.state) != 0) {
    return CompareState(p1.state, p2.state) < 0;
  }

  if (p1.proposal.header().proposer_id() ==
      p2.proposal.header().proposer_id()) {
    return p1.proposal.header().proposal_id() <
           p2.proposal.header().proposal_id();
  }

  return Cmp(p1.proposal.header().proposer_id(),
             p2.proposal.header().proposer_id());
}

/*


std::vector<Proposal> ProposalGraph::GetCommittedProposalFrom(const Proposal&
proposal) const { std::vector<Proposal> ret; auto it =
node_info_.find(proposal.header().prehash()); if(it == node_info_.end()){
    LOG(ERROR)<<"prehash not found from node info";
    return ret;
  }
  if(it->second->state != ProposalState::Committed){
    LOG(ERROR)<<"prehash is not committed";
    return ret;
  }

  std::stack<Proposal> stack;
  std::string hash = latest_commit_.header().hash();
  while(hash != proposal.header().hash()){
    assert(!hash.empty());
    auto it = node_info_.find(hash);
    assert(it != node_info_.end());
    LOG(ERROR)<<"get proposal from
proposer:"<<it->second->proposal.header().proposer_id()<<"
id:"<<it->second->proposal.header().proposal_id();
    stack.push(it->second->proposal);
    hash = it->second->proposal.header().prehash();
    if(hash == proposal.header().prehash()){
      LOG(ERROR)<<"find node done";
      break;
    }
  }
  while(!stack.empty()){
    ret.push_back(stack.top());
    stack.pop();
  }
  return ret;
}
*/

Proposal* ProposalGraph::GetLatestStrongestProposal() {
  Proposal* sp = GetStrongestProposal();
  if (sp == nullptr) {
    if (current_height_ > 0) {
      assert(1 == 0);
    }
    return &latest_commit_;
  }
  LOG(ERROR) << "====== get strong proposal from:" << sp->header().proposer_id()
             << " id:" << sp->header().proposal_id();
  return sp;
}

ProposalState ProposalGraph::GetProposalState(const std::string& hash) const {
  auto node_it = node_info_.find(hash);
  if (node_it == node_info_.end()) {
    return ProposalState::None;
  }
  return node_it->second->state;
}

const Proposal* ProposalGraph::GetProposalInfo(const std::string& hash) const {
  auto it = node_info_.find(hash);
  if (it == node_info_.end()) {
    return nullptr;
  }
  return &it->second->proposal;
}

int ProposalGraph::GetCurrentHeight() { return current_height_; }

}  // namespace basic
}  // namespace cassandra
}  // namespace resdb

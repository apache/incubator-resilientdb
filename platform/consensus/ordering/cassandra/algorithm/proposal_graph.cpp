#include "platform/consensus/ordering/cassandra/algorithm/proposal_graph.h"

#include <glog/logging.h>

#include <queue>
#include <stack>

#include "common/utils/utils.h"

namespace resdb {
namespace cassandra {
namespace cassandra_recv {

std::vector<ProposalState> GetStates() {
  return std::vector<ProposalState>{ProposalState::New, ProposalState::Prepared,
                                    ProposalState::PreCommit};
}

ProposalGraph::ProposalGraph(int fault_num) : f_(fault_num) {
  ranking_ = std::make_unique<Ranking>();
  current_height_ = 0;
  global_stats_ = Stats::GetGlobalStats();
}

void ProposalGraph::IncreaseHeight() {
  // LOG(ERROR) << "increase height:" << current_height_;
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
    // LOG(ERROR) << "add proposal proposer:" << proposal.header().proposer_id()
    //           << " id:" << proposal.header().proposal_id()
    //           << " hash:" << Encode(proposal.header().hash());
  }
}

int ProposalGraph::AddProposal(const Proposal& proposal) {
  // LOG(ERROR) << "add proposal height:" << proposal.header().height()
  //          << " current height:" << current_height_<<"
  //          from:"<<proposal.header().proposer_id()<<" proposal
  //          id:"<<proposal.header().proposal_id();
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
    // assert(1==0);
    return 2;
  }

  // LOG(ERROR)<<"history size:"<<proposal.history_size();
  for (const auto& history : proposal.history()) {
    std::string hash = history.hash();
    auto node_it = node_info_.find(hash);
    assert(node_it != node_info_.end());
    /*
    if (node_it == node_info_.end()) {
      LOG(ERROR) << " history node not found";
      return false;
    }
    else {
      LOG(ERROR)<<"find history";
    }
    */
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
    // LOG(ERROR)<<"proposal:("<<node_it->second->proposal.header().proposer_id()
    //<<","<<node_it->second->proposal.header().proposal_id()<<")"
    //<<" history state:"<<history.state()<<"
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

  if (proposal.header().height() < current_height_) {
    LOG(ERROR) << "height not match:" << current_height_
               << " proposal height:" << proposal.header().height();

    // LOG(ERROR)<<"add proposal proposer:"<<proposal.header().proposer_id()<<"
    // id:"<<proposal.header().proposal_id();
    // g_[proposal.header().prehash()].push_back(proposal.header().hash());
    auto np = std::make_unique<NodeInfo>(proposal);
    new_proposals_[proposal.header().hash()] = &np->proposal;
    node_info_[proposal.header().hash()] = std::move(np);
    last_node_[proposal.header().height()].insert(proposal.header().hash());

    return 1;
  } else {
    g_[proposal.header().prehash()].push_back(proposal.header().hash());
    auto np = std::make_unique<NodeInfo>(proposal);
    new_proposals_[proposal.header().hash()] = &np->proposal;
    // LOG(ERROR)<<"add proposal proposer:"<<proposal.header().proposer_id()<<"
    // id:"<<proposal.header().proposal_id()<<"
    // hash:"<<Encode(proposal.header().hash());
    node_info_[proposal.header().hash()] = std::move(np);
    last_node_[proposal.header().height()].insert(proposal.header().hash());
  }
  return 0;
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
    //           << node_info->proposal.header().proposer_id() << ","
    //           << node_info->proposal.header().proposal_id() << ")"
    //           << " upgrate state:" << node_info->state << (GetCurrentTime() -
    //           node_info->proposal.create_time());
  }
  return true;
}

void ProposalGraph::Commit(const std::string& hash) {
  auto it = node_info_.find(hash);
  if (it == node_info_.end()) {
    LOG(ERROR) << "node not found, hash:" << hash;
    assert(1 == 0);
    return;
  }

  if (it->second->state != ProposalState::PreCommit) {
    LOG(ERROR) << "hash not committed:" << hash;
    assert(1 == 0);
    return;
  }

  std::set<std::string> is_main_hash;
  is_main_hash.insert(hash);

  int from_proposer = it->second->proposal.header().proposer_id();
  int from_proposal_id = it->second->proposal.header().proposal_id();
  // LOG(ERROR)<<"commit :"<<it->second->proposal.header().proposer_id()<<"
  // id:"<<it->second->proposal.header().proposal_id();

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
        assert(1 == 0);
      }

      Proposal* p = &it->second->proposal;
      if (it->second->state == ProposalState::Committed) {
        // LOG(ERROR)<<" try commit proposal,
        // sender:"<<p->header().proposer_id()
        //<<" proposal id:"<<p->header().proposal_id()<<" has been committed";
        continue;
      }

      it->second->state = ProposalState::Committed;
      if (is_main_hash.find(c_hash) != is_main_hash.end()) {
        commit_num_[p->header().proposer_id()]++;
        // LOG(ERROR)<<"commit main node:"<<p->header().proposer_id();
        is_main_hash.insert(p->header().prehash());
        commit_p.push_back(std::vector<Proposal*>());
      }

      commit_p.back().push_back(p);
      // LOG(ERROR)<<"commit node:"<<p->header().proposer_id()<<"
      // id:"<<p->header().proposal_id()
      //<<" weak proposal size:"<<p->weak_proposals().hash_size();
      // LOG(ERROR)<<"push p:"<<p->header().proposer_id();
      for (const std::string& w_hash : p->weak_proposals().hash()) {
        auto it = node_info_.find(w_hash);
        if (it == node_info_.end()) {
          LOG(ERROR) << "node not found, hash:";
          assert(1 == 0);
        }

        // LOG(ERROR)<<"add weak
        // proposal:"<<it->second->proposal.header().proposer_id()<<"
        // id:"<<it->second->proposal.header().proposal_id();
        q.push(w_hash);
      }
      if (!p->header().prehash().empty()) {
        q.push(p->header().prehash());
      }
    }
  };

  bfs();
  if (commit_p.size() > 1) {
    LOG(ERROR) << "commit more hash";
  }
  int block_num = 0;
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
      // LOG(ERROR) << "commmit proposal:"
      //           << commit_p[i][j]->header().proposer_id()
      //           << " height:" << commit_p[i][j]->header().height()
      //           << " idx:" << j
      //           << " delay:" << (GetCurrentTime() -
      //           commit_p[i][j]->create_time())
      //           << " commit from:"<< from_proposer<<" id:"<<from_proposal_id;
      block_num += commit_p[i][j]->block_size();
      if (commit_callback_) {
        commit_callback_(*commit_p[i][j]);
      }
    }
  }
  global_stats_->AddCommitBlock(block_num);

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
    his->set_sender(node_it->second->proposal.header().proposer_id());
    his->set_id(node_it->second->proposal.header().proposal_id());
    hash = node_it->second->proposal.header().prehash();
  }
}

Proposal* ProposalGraph::GetStrongestProposal() {
  // LOG(ERROR) << "get strong proposal from height:" << current_height_;
  if (last_node_.find(current_height_) == last_node_.end()) {
    LOG(ERROR) << "no data:" << current_height_;
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
  //          << sp->proposal.header().proposer_id() << ","
  //          << sp->proposal.header().proposal_id() << ")";
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
    return ProposalState::None;
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
    if (it.second->header().height() >= height) {
      continue;
    }
    ps.push_back(it.second);
  }
  for (Proposal* p : ps) {
    new_proposals_.erase(new_proposals_.find(p->header().hash()));
  }
  return ps;
}

}  // namespace cassandra_recv
}  // namespace cassandra
}  // namespace resdb

#include "platform/consensus/ordering/cassandra_cft/algorithm/proposal_manager.h"

#include <glog/logging.h>

#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"

namespace resdb {
namespace cassandra_cft {

ProposalManager::ProposalManager(int32_t id, int f, int total_num, ProposalGraph* graph)
    : id_(id), f_(f), total_num_(total_num), graph_(graph) {
  local_proposal_id_ = 1;
  round_ = 1;
  global_stats_ = Stats::GetGlobalStats();
}

int ProposalManager::CurrentRound() {
  return round_;
}

std::unique_ptr<Proposal> ProposalManager::MakeBlock(
    std::vector<std::unique_ptr<Transaction>>& txns) {
  auto proposal = std::make_unique<Proposal>();
  for (const auto& txn : txns) {
    *proposal->add_transactions() = *txn;
  }

  proposal->mutable_header()->set_height(round_++);
  proposal->mutable_header()->set_proposer_id(id_);
  proposal->set_sender(id_);
  //LOG(ERROR)<<" make proposal round:"<<proposal->header().height();
  //proposal->set_local_id(local_block_id_++);
  GeneateGraph(proposal.get());
  return proposal;
}

void ProposalManager::GeneateGraph(Proposal * proposal) {
  int round = proposal->header().height();
  std::unique_lock<std::mutex> lk(mutex_);
  for(auto it : proposals_) {
    int proposer = it.first;
    if(it.second.round() < round) {
      //LOG(ERROR)<<" add graph proposal round:"<<it.second.round()<<" proposer:"<<proposer;
      *proposal->add_node_info() = it.second;
    }
    else {
      if(round>1) {
        //LOG(ERROR)<<" add graph pre proposal round:"<<it.second.round()<<" proposer:"<<proposer;
        assert(proposals_rounds_[round-1][proposer]->proposal !=nullptr);
        *proposal->add_node_info() = GetNodeInfo(*proposals_rounds_[round-1][proposer]->proposal);
      }
    }
  }
  if(round > 1) {
    GenerateVoteInfo(proposal);
  }
  //LOG(ERROR)<<" ====== "<<round;
}

void ProposalManager::GenerateVoteInfo(Proposal * proposal) {
  NodeInfo * vote_info = proposal->mutable_vote();
  int proposer = 1;
  int vote_round = -1;
  int round = proposal->header().height()-1;
  for(const auto& p : proposal->node_info()) {
    //LOG(ERROR)<<" node info proposer:"<<p.proposer()<<" round:"<<p.round()<<" vote record:"<<vote_record_[p.proposer()];
    if(p.round() != round) {
      continue;
    }
    if(vote_record_[p.proposer()] > vote_round) {
      vote_round = vote_record_[p.proposer()];
      proposer = p.proposer();
    }
  }
  //LOG(ERROR)<<" generate vote:"<<proposer<<" vote round:"<<vote_round<<" round:"<<round;
  assert(vote_round>=0);
  vote_info->set_proposer(proposer);
  vote_info->set_round(round);
}

void ProposalManager::AddProposal(std::unique_ptr<Proposal> proposal) {
  int round = proposal->header().height();
  int proposer = proposal->header().proposer_id();
  //LOG(ERROR)<<" add proposal round:"<<round<<" from proposer:"<<proposer;
  NodeInfo node_info = GetNodeInfo(*proposal);
  {
    std::unique_lock<std::mutex> lk(mutex_);
    /*
    if(round > 1) {
      AddVote(*proposal);
    }
    */
    proposals_[proposer] = node_info;
    LOG(ERROR)<<" add proposal round:"<<round<<" from proposer:"<<proposer;
    proposals_rounds_[round][proposer] = std::make_unique<ProposalInfo>(std::move(proposal));
    //LOG(ERROR)<<" add proposal round:"<<round<<" from proposer:"<<proposer;
    assert(proposals_rounds_[round][proposer]->proposal != nullptr);
  }
}

std::unique_ptr<Proposal> ProposalManager::FetchProposal(const NodeInfo& info) {
  int round = info.round();
  int proposer = info.proposer();
  std::unique_lock<std::mutex> lk(mutex_);
  LOG(ERROR)<<" fetch proposer:"<<proposer<<" round:"<<round;
  assert(proposals_rounds_[round][proposer]->proposal !=nullptr);
  return std::move(proposals_rounds_[round][proposer]->proposal);
}

void ProposalManager::AddVote(const Proposal& proposal) {
  NodeInfo vote_info = proposal.vote();
  int round = vote_info.round();
  int proposer = vote_info.proposer();
  //LOG(ERROR)<<" add vote:"<<round<<" proposer:"<<proposer;
  ProposalInfo *pi = proposals_rounds_[round][proposer].get();
  assert(pi != nullptr);
  pi->vote.insert(proposal.header().proposer_id());
  if(pi->vote.size() == f_+1) {
    //LOG(ERROR)<<" vote done:"<<round<<" proposer:"<<proposer;
    assert(round > vote_record_[proposer]);
    vote_record_[proposer] = round;
    vote_proposer_[round] = proposer;
  }
}


void ProposalManager::CheckVote(int round) {

  if(round < 1) {
    return;
  }

  int leader = -1;
  std::map<int,int> num;
  {
    std::unique_lock<std::mutex> lk(mutex_);
    for(auto& it : proposals_rounds_[round+1]) {
      ProposalInfo *rpi = it.second.get();
      Proposal * p = rpi->proposal.get();
      {
        NodeInfo vote_info = p->vote();
        int round = vote_info.round();
        int proposer = vote_info.proposer();
        num[proposer]++;
        if(num[proposer] == f_+1) {
          leader = proposer;
        }
      }
    }
  }

/*
      //LOG(ERROR)<<" add vote:"<<round<<" proposer:"<<proposer<<" from round:"<<round+1<<" proposer:"<<p->header().proposer_id();
      ProposalInfo *pi = proposals_rounds_[round][proposer].get();

      assert(pi != nullptr);
      pi->vote.insert(p->header().proposer_id());
      if(pi->vote.size() == f_+1) {
        //LOG(ERROR)<<" vote done:"<<round<<" proposer:"<<proposer;
        assert(round > vote_record_[proposer]);
        vote_record_[proposer] = round;
        leader = proposer;
        break;
      }
    }
  }
  */

  //LOG(ERROR)<<" check vote:"<<round;
  int proposer = leader;
  //LOG(ERROR)<<" check vote:"<<round<<" proposer:"<<proposer;

  std::unique_ptr<Proposal> commit_p = nullptr;
  while(commit_p == nullptr){
    std::unique_lock<std::mutex> lk(mutex_);
    ProposalInfo *pi = proposals_rounds_[round][proposer].get();
    //LOG(ERROR)<<" get pi:"<<(pi == nullptr);
    if(pi == nullptr) {
      usleep(100);
      continue;
    }
    commit_p = std::move(pi->proposal);
    break;
  }
  //assert(pi != nullptr);
  //assert(pi->vote.size() >= f_+1);
  //LOG(ERROR)<<" commit vote:"<<round<<" proposer:"<<proposer;
  CommitProposal(std::move(commit_p));
  //CommitProposal(std::move(pi->proposal));
  //vote_proposer_.erase(vote_proposer_.find(round));
  //LOG(ERROR)<<" check vote done";
}

NodeInfo ProposalManager::GetNodeInfo(const Proposal& proposal) {
  int round = proposal.header().height();
  int proposer = proposal.header().proposer_id();
  NodeInfo node_info;
  node_info.set_round(round);
  node_info.set_proposer(proposer);
  return node_info;
}

void ProposalManager::CommitProposal(std::unique_ptr<Proposal> proposal) {
  commit_func_(std::move(proposal));
}

bool ProposalManager::Ready() {
  if(round_ == 1){
    return true;
  }
  std::unique_lock<std::mutex> lk(mutex_);
  //LOG(ERROR)<<" round size:"<<proposals_rounds_[round_-1].size()<<" total:"<<total_num_<<" round:"<<round_;
  return proposals_rounds_[round_-1].size() == total_num_;
}

bool ProposalManager::Ready(int round) {
  if(round < 1){
    return true;
  }
  std::unique_lock<std::mutex> lk(mutex_);
  //LOG(ERROR)<<" round size:"<<proposals_rounds_[round].size()<<" total:"<<total_num_<<" round:"<<round;
  return proposals_rounds_[round].size() == total_num_;
}

bool ProposalManager::MayReady(int round){
  if(round < 1){
    return true;
  }
  std::unique_lock<std::mutex> lk(mutex_);
  //LOG(ERROR)<<" round size:"<<proposals_rounds_[round].size()<<" total:"<<total_num_<<" round:"<<round;
  return proposals_rounds_[round].size() >= f_+1;

}


}  // namespace cassandra_cft
}  // namespace resdb

#include "platform/consensus/ordering/cassandra/algorithm/proposal_manager.h"

#include <glog/logging.h>

#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"

namespace resdb {
namespace cassandra {
namespace cassandra_recv {

namespace {
std::string Encode(const std::string& hash) {
  std::string ret;
  for (int i = 0; i < hash.size(); ++i) {
    int x = hash[i];
    ret += std::to_string(x);
  }
  return ret;
}

}

ProposalManager::ProposalManager(int32_t id, ProposalGraph* graph)
    : id_(id), graph_(graph) {
  local_proposal_id_ = 1;
  global_stats_ = Stats::GetGlobalStats();
}

int ProposalManager::CurrentRound() { return graph_->GetCurrentHeight(); }

std::unique_ptr<Block> ProposalManager::MakeBlock(
    std::vector<std::unique_ptr<Transaction>>& txns) {
  auto block = std::make_unique<Block>();
  Block::BlockData* data = block->mutable_data();
  for (const auto& txn : txns) {
    *data->add_transaction() = *txn;
  }

  std::string data_str;
  data->SerializeToString(&data_str);
  std::string hash = SignatureVerifier::CalculateHash(data_str);
  block->set_hash(hash);
  block->set_sender_id(id_);
  block->set_create_time(GetCurrentTime());
  block->set_local_id(local_block_id_++);
  // LOG(ERROR)<<"make block time:"<<block->create_time();
  return block;
}

void ProposalManager::AddLocalBlock(std::unique_ptr<Block> block) {
  std::unique_lock<std::mutex> lk(mutex_);
  //LOG(ERROR)<<"add local block :"<<block->local_id();
  blocks_candidates_[block->local_id()] = std::move(block);
}

void ProposalManager::BlockReady(const std::string& hash, int local_id) {
  std::unique_lock<std::mutex> lk(mutex_);
  //LOG(ERROR)<<"ready block:"<<local_id;
  auto it = blocks_candidates_.find(local_id);
  assert(it != blocks_candidates_.end());
  assert(it->second->hash() == hash);
  blocks_.push_back(std::move(it->second));
  blocks_candidates_.erase(it);
  //  Notify();
  notify_.notify_all();
}

void ProposalManager::AddBlock(std::unique_ptr<Block> block) {
  std::unique_lock<std::mutex> lk(p_mutex_);
  int sender = block->sender_id();
  int block_id = block->local_id();
  //LOG(ERROR)<<"add block from sender:"<<sender<<" id:"<<block_id<<" blocksize:"<<pending_blocks_[sender].size()<<" hash:"<<Encode(block->hash());
  pending_blocks_[sender][block->hash()] = std::move(block);
}

bool ProposalManager::ContainBlock(const std::string& hash, int sender) {
  std::unique_lock<std::mutex> lk(p_mutex_);
  auto bit = pending_blocks_[sender].find(hash);
  return bit != pending_blocks_[sender].end();
}

bool ProposalManager::ContainBlock(const Block& block) {
  std::unique_lock<std::mutex> lk(p_mutex_);
  int sender = block.sender_id();
  const std::string& hash = block.hash();
  auto bit = pending_blocks_[sender].find(hash);
  return bit != pending_blocks_[sender].end();
}

const Block* ProposalManager::QueryBlock(const std::string& hash) {
  std::unique_lock<std::mutex> lk(p_mutex_);
  auto bit = pending_blocks_[id_].find(hash);
  assert(bit != pending_blocks_[id_].end());
  return bit->second.get();
}

Block* ProposalManager::GetBlockSnap(const std::string& hash, int sender) {
  std::unique_lock<std::mutex> lk(p_mutex_);

  //LOG(ERROR)<<" sender:"<<sender<<" block size:"<<pending_blocks_[sender].size();
  auto it = pending_blocks_[sender].find(hash);
  if (it == pending_blocks_[sender].end()) {
    LOG(ERROR) << "block from sender:" << sender << " not found"<<" pending size:"<<pending_blocks_[sender].size();
    return nullptr;
  }
  assert(it != pending_blocks_[sender].end());
  // LOG(ERROR)<<"get block: sender:"<<sender<<" id:"<<block->local_id();
  return it->second.get();
}

int ProposalManager::GetBlockNum(const std::string& hash, int sender) {
  std::unique_lock<std::mutex> lk(p_mutex_);
  auto it = pending_blocks_[sender].find(hash);
  if (it == pending_blocks_[sender].end()) {
    assert(1==0);
  }
  assert(it != pending_blocks_[sender].end());
  const auto& block = it->second;
  //LOG(ERROR)<<"get block: sender:"<<sender<<" id:"<<block->local_id()<<" removed";
  int num = block->data().transaction_size();
  LOG(ERROR)<<"get block: sender:"<<sender<<" id:"<<block->local_id()<<" trans num:"<<num;
  return num;
}

std::unique_ptr<Block> ProposalManager::GetBlock(const std::string& hash,
                                                 int sender) {
  bool need_wait = false;
  while (true) {
    if (need_wait) {
      usleep(1000);
    }
    std::unique_lock<std::mutex> lk(p_mutex_);
    auto it = pending_blocks_[sender].find(hash);
    if (it == pending_blocks_[sender].end()) {
      //LOG(ERROR) << "block from sender:" << sender << " not found";
      return nullptr;
      need_wait = true;
      assert(1==0);
      continue;
    }
    assert(it != pending_blocks_[sender].end());
    auto block = std::move(it->second);
    //LOG(ERROR)<<"get block: sender:"<<sender<<" id:"<<block->local_id()<<" removed";
    pending_blocks_[sender].erase(it);
    return block;
  }
}

void ProposalManager::ClearProposal(const Proposal& p) {}

/*
void ProposalManager::Notify(){
  //std::unique_lock<std::mutex> lk(notify_mutex_);
  notify_.notify_all();
}

void ProposalManager::Wait(){
  //std::unique_lock<std::mutex> lk(notify_mutex_);
  notify_.wait_for(lk, std::chrono::microseconds(100000),
      [&] { return !blocks_.empty(); });
}
*/

bool ProposalManager::WaitBlock() {
  std::unique_lock<std::mutex> lk(mutex_);
  if (blocks_.empty()) {
    notify_.wait_for(lk, std::chrono::microseconds(1000000),
                     [&] { return !blocks_.empty(); });
    //LOG(ERROR) << "wait proposal block size:" << blocks_.size();
  }
  return !blocks_.empty();
}

std::unique_ptr<Proposal> ProposalManager::GenerateProposal(int round,
                                                            bool need_empty) {
  auto proposal = std::make_unique<Proposal>();
  std::string data;
  {
    //LOG(ERROR) << "generate proposal pending block size:" << blocks_.size();
    std::unique_lock<std::mutex> lk(mutex_);
    if (blocks_.empty() && !need_empty) {
      // return nullptr;
      //    Wait();
      // notify_.wait_for(lk, std::chrono::microseconds(100000),
      //[&] { return !blocks_.empty(); });
      // if(blocks_.empty()){
      // return nullptr;
      //}
      return nullptr;
      // LOG(ERROR) << "generate wait proposal block size:" << blocks_.size();
    }
    int max_block = 1;
    int num = 0;
    int64_t current_time = GetCurrentTime();
    proposal->set_create_time(current_time);
    //LOG(ERROR)<<"block size:"<<blocks_.size();
    for (auto& block : blocks_) {
      data += block->hash();
      Block* ab = proposal->add_block();
      *ab = *block;

      Block * sb = proposal->add_sub_block();
      sb->set_hash(block->hash());
      sb->set_sender_id(block->sender_id());
      sb->set_local_id(block->local_id());
      sb->set_create_time(block->create_time());

      /*
      ab->set_hash(block->hash());
      ab->set_sender_id(block->sender_id());
      ab->set_local_id(block->local_id());
      ab->set_create_time(block->create_time());
      */
      //LOG(ERROR) << " add block:" << block->local_id()
      //           << " block delay:" << (current_time - block->create_time())
      //           << " block size:" << blocks_.size()
      //           << " txn:" << block->data().transaction_size();
      max_block--;
      num++;
      // break;
      if (max_block <= 0) break;
      // blocks_.pop_front();
      // break;
    }
  global_stats_->AddBlockSize(num);
    //LOG(ERROR) << "block num:" << num;
    while (num > 0) {
      blocks_.pop_front();
      num--;
    }
    /*
    if(!blocks_.empty()){
      blocks_.pop_front();
    }
    */
    // blocks_.clear();
  }

  Proposal* last = graph_->GetLatestStrongestProposal();
  proposal->mutable_header()->set_height(round);
  //LOG(ERROR) << "get last proposal, proposer:" << last->header().proposer_id()
  //           << " id:" << last->header().proposal_id();

  graph_->IncreaseHeight();
  if (last != nullptr) {
    assert(last->header().height() + 1 == round);

    proposal->mutable_header()->set_prehash(last->header().hash());
    *proposal->mutable_history() = last->history();
  }

  
  {
    std::vector<Block> ps = graph_->GetNewBlocks();
    for(auto block : ps) {
      auto sub_block = proposal->add_sub_block();
      *sub_block = block;
    }
    //LOG(ERROR)<<" proposal sub block size:"<<proposal->sub_block_size();
  }

  proposal->mutable_header()->set_proposer_id(id_);
  proposal->mutable_header()->set_proposal_id(local_proposal_id_++);
  data += std::to_string(proposal->header().proposal_id()) +
          std::to_string(proposal->header().proposer_id()) +
          std::to_string(proposal->header().height());

  std::string hash = SignatureVerifier::CalculateHash(data);
  proposal->mutable_header()->set_hash(hash);
  // proposal->set_create_time(GetCurrentTime());
  return proposal;
}

void ProposalManager::ObtainHistoryProposal(const Proposal* p,
                                            std::set<std::pair<int, int>>& v,
                                            std::vector<const Proposal*>& resp,
                                            int current_height) {
  // LOG(ERROR)<<"obtain proposal history:"<<p->header().proposer_id()<<"
  // id:"<<p->header().proposal_id();
  const std::string& pre_hash = p->header().prehash();
  if (!pre_hash.empty()) {
    const Proposal* next_p = graph_->GetProposalInfo(pre_hash);
    assert(next_p != nullptr);
    int height = next_p->header().height();
    // LOG(ERROR)<<"Obtain next height:"<<height<<"
    // proposer:"<<next_p->header().proposer_id();
    //  LOG(ERROR)<<"ask height:"<<current_height<<" current height:"<<height;
    if (current_height - height > 105) {
      return;
    }
    if (v.find(std::make_pair(next_p->header().proposer_id(),
                              next_p->header().proposal_id())) != v.end()) {
      return;
    }
    v.insert(std::make_pair(next_p->header().proposer_id(),
                            next_p->header().proposal_id()));
    // LOG(ERROR)<<"Obtain height:"<<height<<"
    // proposer:"<<next_p->header().proposer_id()<<"
    // id:"<<next_p->header().proposal_id();
    ObtainHistoryProposal(next_p, v, resp, current_height);
    resp.push_back(next_p);
  }
}

int ProposalManager::VerifyProposalHistory(const Proposal* p) {
  // LOG(ERROR)<<"verify proposal, proposer:"<<p->header().proposer_id()<<"
  // id:"<<p->header().proposal_id();
  int ret = 0;
  const std::string& pre_hash = p->header().prehash();
  if (!pre_hash.empty()) {
    const Proposal* next_p = graph_->GetProposalInfo(pre_hash);
    if (next_p == nullptr) {
      LOG(ERROR) << "no prehash:";
      if (tmp_proposal_.find(pre_hash) == tmp_proposal_.end()) {
        LOG(ERROR) << " prehash not found";
        return 2;
      }
      ret = 1;
      auto& it = tmp_proposal_[pre_hash];
      assert(it != nullptr);
      LOG(ERROR) << "Find pre hash in tmp:" << it->header().proposer_id()
                 << " id:" << it->header().proposal_id();
    }
  }

  return ret;
}

std::unique_ptr<ProposalQueryResp> ProposalManager::QueryProposal(
    const std::string& hash) {
  const Proposal* p = graph_->GetProposalInfo(hash);
  if (p == nullptr) {
    p = GetLocalProposal(hash);
    LOG(ERROR) << "get from local:" << (p == nullptr);
  }
  assert(p != nullptr);
  LOG(ERROR) << "query proposal id:" << p->header().proposal_id();
  int current_height = p->header().height();

  std::set<std::pair<int, int>> v;
  std::vector<const Proposal*> list;

  {
    std::unique_lock<std::mutex> lk(q_mutex_);
    ObtainHistoryProposal(p, v, list, current_height);
  }

  std::unique_ptr<ProposalQueryResp> resp =
      std::make_unique<ProposalQueryResp>();
  for (const Proposal* p : list) {
    *resp->add_proposal() = *p;
  }
  return resp;
}

int ProposalManager::VerifyProposal(const Proposal& proposal) {
  std::unique_lock<std::mutex> lk(q_mutex_);
  int ret = VerifyProposalHistory(&proposal);
  if (ret != 0) {
    LOG(ERROR) << "add to temp proposer:" << proposal.header().proposer_id()
               << " id:" << proposal.header().proposal_id();
    tmp_proposal_[proposal.header().hash()] =
        std::make_unique<Proposal>(proposal);
  }
  return ret;
}

void ProposalManager::ReleaseTmpProposal(const Proposal& proposal) {
  std::unique_lock<std::mutex> lk(q_mutex_);
  auto it = tmp_proposal_.find(proposal.header().hash());
  if (it != tmp_proposal_.end()) {
    //LOG(ERROR) << "release proposal:" << proposal.header().proposer_id()
    //           << " id:" << proposal.header().proposal_id();
    tmp_proposal_.erase(it);
  }
  graph_->AddProposalOnly(proposal);
}

void ProposalManager::AddLocalProposal(const Proposal& proposal) {
  std::unique_lock<std::mutex> lk(t_mutex_);
  local_proposal_[proposal.header().hash()] =
      std::make_unique<Proposal>(proposal);
  //LOG(ERROR) << "add local id:" << proposal.header().proposal_id();
}

Proposal* ProposalManager::GetLocalProposal(const std::string& hash) {
  std::unique_lock<std::mutex> lk(t_mutex_);
  auto it = local_proposal_.find(hash);
  if (it == local_proposal_.end()) return nullptr;
  return it->second.get();
}

void ProposalManager::RemoveLocalProposal(const std::string& hash) {
  std::unique_lock<std::mutex> lk(t_mutex_);
  auto it = local_proposal_.find(hash);
  if (it == local_proposal_.end()) return;
  //LOG(ERROR) << "remove local id:" << it->second->header().proposal_id();
  local_proposal_.erase(it);
}

int ProposalManager::VerifyProposal(const ProposalQueryResp& resp) {
  std::map<std::pair<int, int>, std::unique_ptr<Proposal>> list;
  LOG(ERROR) << "verify resp proposal size:" << resp.proposal_size();
  for (auto& it : tmp_proposal_) {
    std::unique_ptr<Proposal> p = std::move(it.second);
    list[std::make_pair(p->header().height(), p->header().proposer_id())] =
        std::move(p);
  }

  tmp_proposal_.clear();

  for (const Proposal& p : resp.proposal()) {
    LOG(ERROR)<<"verify resp proposal proposer:"<<p.header().proposer_id()<<" id:"<<p.header().proposal_id();
    if (list.find(std::make_pair(p.header().height(),
                                 p.header().proposer_id())) != list.end()) {
      continue;
    }
    list[std::make_pair(p.header().height(), p.header().proposer_id())] =
        std::make_unique<Proposal>(p);
  }

  std::vector<std::unique_ptr<Proposal>> fail_list;
  while (!list.empty()) {
    auto it = list.begin();
    int ret = VerifyProposalHistory(it->second.get());
     LOG(ERROR)<<"verify propser:"<<it->second->header().proposer_id()<<" height:"<<it->second->header().height()<<" ret:"<<ret;
    if (ret == 0) {
      ReleaseTmpProposal(*it->second);
        LOG(ERROR)<<"proposal from:"<<it->second->header().proposer_id()
       <<" id:"<<it->second->header().proposal_id()<<" activate";
    } else {
      fail_list.push_back(std::move(it->second));
    }
    list.erase(it);
    // assert(ret==0);
  }
  // tmp_proposal_.clear();
  for (auto& p : fail_list) {
    LOG(ERROR) << "proposal from:" << p->header().proposer_id()
               << " id:" << p->header().proposal_id() << " fail";
    assert(p != nullptr);
    tmp_proposal_[p->header().hash()] = std::move(p);
  }
  return 0;
}

}  // namespace cassandra_recv
}  // namespace cassandra
}  // namespace resdb

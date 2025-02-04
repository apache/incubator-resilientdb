#include "platform/consensus/ordering/cassandra/algorithm/fast_path/proposal_manager.h"

#include <glog/logging.h>

#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"

namespace resdb {
namespace cassandra {
namespace cassandra_fp {

ProposalManager::ProposalManager(int32_t id, int total_num,
                                 ProposalGraph* graph)
    : id_(id), graph_(graph) {
  total_num_ = total_num;
  local_proposal_id_ = 1;
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
  LOG(ERROR) << "make block time:" << block->create_time();
  return block;
}

void ProposalManager::AddLocalBlock(std::unique_ptr<Block> block) {
  std::unique_lock<std::mutex> lk(mutex_);
  LOG(ERROR) << "notify block :" << block->create_time();
  blocks_.push_back(std::move(block));
  //  Notify();
  notify_.notify_all();
}

void ProposalManager::AddBlock(std::unique_ptr<Block> block) {
  std::unique_lock<std::mutex> lk(p_mutex_);
  int sender = block->sender_id();
  int block_id = block->local_id();
  pending_blocks_[sender][block->hash()] = std::move(block);
  // LOG(ERROR)<<"add block: sender:"<<sender<<" id:"<<block_id;
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
      LOG(ERROR) << "block from sender:" << sender << " not found";
      need_wait = true;
      continue;
    }
    assert(it != pending_blocks_[sender].end());
    auto block = std::move(it->second);
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
    LOG(ERROR) << "wait proposal block size:" << blocks_.size();
  }
  return !blocks_.empty();
}

std::unique_ptr<Proposal> ProposalManager::GenerateProposal(int round,
                                                            bool need_empty) {
  auto proposal = std::make_unique<Proposal>();
  std::string data;
  {
    LOG(ERROR) << "generate proposal block size:" << blocks_.size();
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
    int max_block = 3;
    int num = 0;
    int64_t current_time = GetCurrentTime();
    proposal->set_create_time(current_time);
    for (auto& block : blocks_) {
      data += block->hash();
      Block* ab = proposal->add_block();
      ab->set_hash(block->hash());
      ab->set_create_time(block->create_time());
      // LOG(ERROR)<<" add block:"<<block->local_id()<<" block
      // delay:"<<(current_time - block->create_time())<<" block
      // size:"<<blocks_.size();
      max_block--;
      num++;
      // break;
      if (max_block <= 0) break;
      // blocks_.pop_front();
      // break;
    }
    // LOG(ERROR)<<"block num:"<<num;
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

  graph_->IncreaseHeight();
  if (last != nullptr) {
    assert(last->header().height() + 1 == round);

    proposal->mutable_header()->set_prehash(last->header().hash());
    *proposal->mutable_history() = last->history();
  }

  {
    std::vector<Proposal*> ps = graph_->GetNewProposals(round);
    // LOG(ERROR)<<"get weak p from round:"<<round<<" size:"<<ps.size();
    for (Proposal* p : ps) {
      if (p->header().height() >= round) {
        LOG(ERROR) << "round invalid:" << round
                   << " header:" << p->header().height();
      }
      assert(p->header().height() < round);

      if (p->header().hash() == last->header().hash()) {
        continue;
      }
      // LOG(ERROR)<<"add weak p:"<<p->header().height()<<"
      // proposer:"<<p->header().proposer_id();
      *proposal->mutable_weak_proposals()->add_hash() = p->header().hash();
      data += p->header().hash();
    }
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

int ProposalManager::GetNextLeader(int height) {
  return height % total_num_ + 1;
}

void ProposalManager::Confirm(int height, int leader) {
  graph_->Confirm(height, leader);
}

}  // namespace cassandra_fp
}  // namespace cassandra
}  // namespace resdb

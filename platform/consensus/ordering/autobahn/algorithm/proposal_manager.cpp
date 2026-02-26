#include "platform/consensus/ordering/autobahn/algorithm/proposal_manager.h"

#include <glog/logging.h>

#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"

namespace resdb {
namespace autobahn {

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

ProposalManager::ProposalManager(int32_t id, int total_num, int f, SignatureVerifier* verifier)
    : id_(id), total_num_(total_num), f_(f), verifier_(verifier) {
  current_height_ = 0;
  local_block_id_ = 1;
  current_slot_ = 1;
}

void ProposalManager::MakeBlock(
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
  AddLocalBlock(std::move(block));
  // LOG(ERROR)<<"make block time:"<<block->create_time();
}

void ProposalManager::AddBlock(std::unique_ptr<Block> block) {
  std::unique_lock<std::mutex> lk(mutex_);
  int sender = block->sender_id();
  int block_id = block->local_id();

  //LOG(ERROR)<<"add block from sender:"<<sender<<" id:"<<block_id; 
  
  if(block_id>1) {
    assert(block->last_sign_info_size() >= f_+1);
    assert(VerifyBlock(*block));
    assert(pending_blocks_[sender].find(block_id-1) != pending_blocks_[sender].end());
    *pending_blocks_[sender][block_id-1]->mutable_sign_info() = block->last_sign_info();
  }
  pending_blocks_[sender][block_id] = std::move(block);
}
      
Block* ProposalManager::GetBlock(int sender, int64_t block_id) {
  std::unique_lock<std::mutex> lk(mutex_);
  //LOG(ERROR)<<" get block from sender:"<<sender<<" block id:"<<block_id;
  auto it = pending_blocks_[sender].find(block_id);
  assert(it != pending_blocks_[sender].end());
  return it->second.get();
}

void ProposalManager::AddLocalBlock(std::unique_ptr<Block> block) {
  std::unique_lock<std::mutex> lk(mutex_);
  //LOG(ERROR)<<"add local block :"<<block->local_id();
  blocks_candidates_[block->local_id()] = std::move(block);
}

const Block* ProposalManager::GetLocalBlock(int64_t block_id) {
  std::unique_lock<std::mutex> lk(mutex_);
  if(blocks_candidates_.find(block_id) == blocks_candidates_.end()) {
    return nullptr;
  }
  //LOG(ERROR)<<"get local block :"<<block_id;
  while(!blocks_candidates_.empty() && blocks_candidates_.begin()->first < block_id) {
    blocks_candidates_.erase(blocks_candidates_.begin());
  }
  Block * block = blocks_candidates_.begin()->second.get();
  UpdateLastSign(block);
  return block;
}

void ProposalManager::BlockReady(const std::map<int, SignInfo>& sign_info, int64_t local_id) {
  std::unique_lock<std::mutex> lk(mutex_);
  //LOG(ERROR)<<"ready block:"<<local_id;
  auto it = blocks_candidates_.find(local_id);
  if(it == blocks_candidates_.end()){
    return;
  }
  assert(it != blocks_candidates_.end());
  Block * block = it->second.get();
  for(auto sit : sign_info) {
    assert(sit.second.hash() == block->hash());
    *block->add_sign_info() = sit.second;
    LOG(ERROR)<<" add last sign:"<<sit.second.sender_id();
  }

  //LOG(ERROR)<<" update block sender:"<<id_<<" local id:"<<local_id;
  assert(it->second != nullptr);
  pending_blocks_[id_][local_id] = std::move(it->second);
  blocks_candidates_.erase(it);
  current_height_ = std::max(current_height_, local_id);
}

int64_t ProposalManager::GetCurrentBlockId() {
  std::unique_lock<std::mutex> lk(mutex_);
  return current_height_;
}

void ProposalManager:: UpdateLastSign(Block * block) {
  int block_id = block->local_id();
  //LOG(ERROR)<<" update block sign:"<<block_id;
  if(block_id>1) {
    auto it = pending_blocks_[id_].find(block_id-1);
    assert(it != pending_blocks_[id_].end());
    *block->mutable_last_sign_info() = it->second->sign_info();
  }
}

bool ProposalManager::VerifyBlock(const Block& block) {

  if(block.last_sign_info_size() < f_+1) {
    LOG(ERROR)<<" sign info size fail";
    return false;
  }

  std::set<int> senders;
  for(const auto& sign_info : block.last_sign_info()){
    if(sign_info.hash() != block.last_sign_info(0).hash()){
      LOG(ERROR)<<" sign info hash fail";
      return false;
    }
    if(sign_info.local_id() != block.last_sign_info(0).local_id()){
      LOG(ERROR)<<" sign info local id fail";
      return false;
    }
    //LOG(ERROR)<<" check sign :"<<sign_info.sender_id();
    senders.insert(sign_info.sender_id());

    bool valid = verifier_->VerifyMessage(sign_info.hash(),
        sign_info.sign());
    if (!valid) {
      LOG(ERROR)<<" sign info sign fail";
      return false;
    }
  }
  //LOG(ERROR)<<" sign info sender size"<< senders.size();
  return senders.size() >= f_+1;
}

SignInfo ProposalManager::SignBlock(const Block& block) {
  SignInfo sign_info;
  sign_info.set_hash(block.hash());
  sign_info.set_sender_id(id_);
  sign_info.set_local_id(block.local_id());

  auto hash_signature_or = verifier_->SignMessage(block.hash());
  if (!hash_signature_or.ok()) {
    LOG(ERROR) << "Sign message fail";
    return SignInfo();
  }
  *sign_info.mutable_sign()=*hash_signature_or;
  return sign_info;
}

void ProposalManager::UpdateView(int sender, int64_t block_id) {
  std::unique_lock<std::mutex> lk(slot_mutex_);
  //LOG(ERROR)<<"update slot sender:"<<sender<<" slot:"<<current_slot_;
  if(slot_state_[sender].first != current_slot_) {
    new_blocks_[current_slot_]++;
  }
  slot_state_[sender] = std::make_pair(current_slot_, block_id);
}

bool ProposalManager::ReadyView(int slot){
  std::unique_lock<std::mutex> lk(slot_mutex_);
  //LOG(ERROR)<<"ready slot sender:"<<current_slot_<<" new blocks:"<<new_blocks_[current_slot_];
  return new_blocks_[current_slot_]>=2*f_+1;
}

int ProposalManager::GetCurrentView() {
  std::unique_lock<std::mutex> lk(slot_mutex_);
  return current_slot_;
}

void ProposalManager::IncreaseView() {
  std::unique_lock<std::mutex> lk(slot_mutex_);
  //LOG(ERROR)<<"increase slot:"<<current_slot_;
  current_slot_++;
}

std::pair<int, std::map<int, int64_t>> ProposalManager::GetCut() {
  std::map<int, int64_t> blocks;
  std::unique_lock<std::mutex> lk(slot_mutex_);
  for(auto it : slot_state_) {
    if(it.second.first == current_slot_) {
      blocks[it.first]=it.second.second;
      //LOG(ERROR)<<"get cut sender:"<<it.first<<" block:"<<it.second.second;
    }
  }
  current_slot_++;
  return std::make_pair(current_slot_-1, blocks);
}

std::unique_ptr<Proposal> ProposalManager::GenerateProposal(int slot, const std::map<int, int64_t>& blocks) {
  auto proposal = std::make_unique<Proposal>();
  std::string data_hash;
  {
    for (auto& it: blocks) {
      Block* block = proposal->add_block();
      Block* data_block = GetBlock(it.first, it.second);
      data_hash += data_block->hash();
      //LOG(ERROR)<<" gene proposal block from:"<<data_block->sender_id()<<" block id:"<<data_block->local_id();
      *block->mutable_sign_info() = data_block->sign_info();
      block->set_local_id(data_block->local_id());
      block->set_sender_id(data_block->sender_id());
    }
  }
  proposal->set_slot_id(slot);
  proposal->set_sender_id(id_);
  proposal->set_hash(data_hash);
  return proposal;
}

std::unique_ptr<Proposal> ProposalManager::GetProposalData(int slot) {
  std::unique_lock<std::mutex> lk(p_mutex_);
  //LOG(ERROR)<<" get proposal:"<<slot;
  return std::move(pending_proposals_[slot]);
}

void ProposalManager::AddProposalData(std::unique_ptr<Proposal> p) {
  std::unique_lock<std::mutex> lk(p_mutex_);
  int slot_id = p->slot_id();
  //LOG(ERROR)<<" add proposal:"<<slot_id;
  /*
  for(const auto& block : p->block()) {
    int block_owner = block.sender_id();
    int block_id = block.local_id();
    LOG(ERROR)<<" add proposal block:"<<block_owner<<" block id :"<<block_id<<" slot:"<<slot_id;
  }
  */

  pending_proposals_[slot_id] = std::move(p);
}

}  // namespace autobahn
}  // namespace resdb

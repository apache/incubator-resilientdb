#include "platform/consensus/ordering/autobahn/algorithm/proposal_manager.h"

#include <glog/logging.h>

#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"

#include "platform/proto/resdb.pb.h"
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

  /*
    auto batch_request = std::make_unique<BatchUserRequest>();
    if (!batch_request->ParseFromString(txn->data())) {
      LOG(ERROR) << "parse data fail";
    }
    LOG(ERROR)<<" receive block id:"<<local_block_id_<<" local id:"<<batch_request->local_id();
  */


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

bool ProposalManager::AddBlock(std::unique_ptr<Block> block) {
  std::unique_lock<std::mutex> lk(mutex_);
  int sender = block->sender_id();
  int block_id = block->local_id();

  //LOG(ERROR)<<"add block from sender:"<<sender<<" id:"<<block_id; 
  
  if(block_id>1) {
    //assert(block->last_sign_info_size() >= f_+1);
    //assert(VerifyBlock(*block));
    auto prev_it = pending_blocks_[sender].find(block_id - 1);
    if (prev_it == pending_blocks_[sender].end()) {
      if (executed_height_[sender] < block_id - 1) {
        //LOG(ERROR)<<"fur block from sender:"<<sender<<" id:"<<block_id;
        fur_[sender][block_id] = std::move(block);
        return false;
      }
      //LOG(ERROR)<<"accept block with executed predecessor sender:"<<sender
       //         <<" id:"<<block_id
       //         <<" executed:"<<executed_height_[sender];
    } else {
      *prev_it->second->mutable_sign_info() = block->last_sign_info();
      certified_height_[sender] =
          std::max<int64_t>(certified_height_[sender], block_id - 1);
    }
  }
  pending_blocks_[sender][block_id] = std::move(block);

  while(fur_[sender].find(block_id+1) != fur_[sender].end()){
    //AddBlock(std::move(fur_[sender][block_id+1]));
    *pending_blocks_[sender][block_id]->mutable_sign_info() = fur_[sender][block_id+1]->last_sign_info();
    certified_height_[sender] =
        std::max<int64_t>(certified_height_[sender], block_id);
    pending_blocks_[sender][block_id+1] = std::move(fur_[sender][block_id+1]);
    fur_[sender].erase(fur_[sender].find(block_id+1));
    block_id++;
  }
  return true;
}
      
Block* ProposalManager::GetBlock(int sender, int64_t block_id) {
  std::unique_lock<std::mutex> lk(mutex_);
  //LOG(ERROR)<<" get block from sender:"<<sender<<" block id:"<<block_id;

  auto it = pending_blocks_[sender].find(block_id);
  if(it == pending_blocks_[sender].end()) {
    return nullptr;
  }
  assert(it != pending_blocks_[sender].end());
  return it->second.get();
}

Block* ProposalManager::GetLocalCandidateBlock(int64_t block_id) {
  std::unique_lock<std::mutex> lk(mutex_);
  auto it = blocks_candidates_.find(block_id);
  if (it == blocks_candidates_.end()) {
    return nullptr;
  }
  return it->second.get();
}

bool ProposalManager::CopyBlock(int sender, int64_t block_id, Block* out) {
  if (out == nullptr) {
    return false;
  }
  std::unique_lock<std::mutex> lk(mutex_);
  if (sender < 0 || sender >= 512) {
    return false;
  }
  auto it = pending_blocks_[sender].find(block_id);
  if (it == pending_blocks_[sender].end() || it->second == nullptr) {
    return false;
  }
  *out = *it->second;
  return true;
}

bool ProposalManager::CopyLocalCandidateBlock(int64_t block_id, Block* out) {
  if (out == nullptr) {
    return false;
  }
  std::unique_lock<std::mutex> lk(mutex_);
  auto it = blocks_candidates_.find(block_id);
  if (it == blocks_candidates_.end() || it->second == nullptr) {
    return false;
  }
  *out = *it->second;
  return true;
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
    //LOG(ERROR)<<" add last sign:"<<sit.second.sender_id();
  }

  //LOG(ERROR)<<" update block sender:"<<id_<<" local id:"<<local_id;
  assert(it->second != nullptr);
  pending_blocks_[id_][local_id] = std::move(it->second);
  blocks_candidates_.erase(it);
  current_height_ = std::max(current_height_, local_id);
  certified_height_[id_] = std::max<int64_t>(certified_height_[id_], local_id);
}

int64_t ProposalManager::GetCurrentBlockId() {
  std::unique_lock<std::mutex> lk(mutex_);
  return current_height_;
}

int64_t ProposalManager::GetCertifiedBlockHeight(int sender) {
  std::unique_lock<std::mutex> lk(mutex_);
  return certified_height_[sender];
}

void ProposalManager::SetExecutedBlockHeight(int sender, int64_t block_id) {
  std::unique_lock<std::mutex> lk(mutex_);
  executed_height_[sender] = std::max<int64_t>(executed_height_[sender], block_id);
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
  //LOG(ERROR)<<"update cut sender:"<<sender<<" block:"<<block_id;
  if (slot_state_.find(sender) == slot_state_.end()) {
    new_blocks_[current_slot_]++;
  }
  if (slot_state_.find(sender) == slot_state_.end() ||
      slot_state_[sender].second < block_id) {
    slot_state_[sender] = std::make_pair(current_slot_, block_id);
  }
}

bool ProposalManager::ReadyView(int slot){
  std::unique_lock<std::mutex> lk(slot_mutex_);
  //LOG(ERROR)<<"ready slot:"<<slot<<" known block senders:"<<slot_state_.size();
  return slot_state_.size()>=2*f_+1;
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

void ProposalManager::SetCurrentView(int slot) {
  std::unique_lock<std::mutex> lk(slot_mutex_);
  current_slot_ = slot;
}

std::pair<int, std::map<int, int64_t>> ProposalManager::GetCut(int slot) {
  std::map<int, int64_t> blocks;
  {
    std::unique_lock<std::mutex> lk(slot_mutex_);
    for(auto it : slot_state_) {
      blocks[it.first]=it.second.second;
      //LOG(ERROR)<<"get cut sender:"<<it.first<<" block:"<<it.second.second;
    }
    current_slot_ = std::max(current_slot_, slot);
    current_slot_++;
  }
  return std::make_pair(slot, blocks);
}

std::unique_ptr<Proposal> ProposalManager::GenerateProposal(int slot, const std::map<int, int64_t>& blocks) {
  auto proposal = std::make_unique<Proposal>();
  {
    for (auto& it: blocks) {
      Block* block = proposal->add_block();
      Block* data_block = GetBlock(it.first, it.second);
      if(data_block == nullptr){
        continue;
      }
      assert(data_block != nullptr);
      //LOG(ERROR)<<" gene proposal block from:"<<data_block->sender_id()<<" block id:"<<data_block->local_id();
      *block->mutable_sign_info() = data_block->sign_info();
      block->set_local_id(data_block->local_id());
      block->set_sender_id(data_block->sender_id());
      //block->set_create_time(time(nullptr));
    }
  }
  proposal->set_slot_id(slot);
  proposal->set_sender_id(id_);
  {
    std::unique_lock<std::mutex> lk(p_mutex_);
    if (!high_qc_.hash().empty()) {
      proposal->set_parent_hash(high_qc_.hash());
      *proposal->mutable_justify_qc() = high_qc_;
    }
  }
  proposal->set_hash(GetProposalHash(*proposal));
  return proposal;
}

std::string ProposalManager::GetProposalHash(const Proposal& proposal) {
  Proposal proposal_data(proposal);
  proposal_data.clear_hash();
  std::string data;
  proposal_data.SerializeToString(&data);
  return SignatureVerifier::CalculateHash(data);
}

bool ProposalManager::VerifyQC(const QC& qc) {
  if (qc.hash().empty() && qc.view() == 0) {
    return true;
  }
  if (qc.signatures_size() < 2 * f_ + 1) {
    //LOG(ERROR) << "qc size:" << qc.signatures_size() << " not enough";
    return false;
  }
  for (const auto& sign : qc.signatures()) {
    if (!verifier_->VerifyMessage(qc.hash(), sign)) {
      LOG(ERROR) << "Verify qc signature fail";
      return false;
    }
  }
  return true;
}

bool ProposalManager::VerifyProposal(const Proposal& proposal) {
  if (GetProposalHash(proposal) != proposal.hash()) {
    //LOG(ERROR) << "proposal hash mismatch";
    return false;
  }
  if (!VerifyQC(proposal.justify_qc())) {
    return false;
  }
  if (!proposal.justify_qc().hash().empty() &&
      proposal.justify_qc().hash() != proposal.parent_hash()) {
    LOG(ERROR) << "proposal qc does not justify parent";
    return false;
  }
  return SafeNode(proposal);
}

QC ProposalManager::GetHighQC() {
  std::unique_lock<std::mutex> lk(p_mutex_);
  return high_qc_;
}

QC ProposalManager::GetLockedQC() {
  std::unique_lock<std::mutex> lk(p_mutex_);
  return locked_qc_;
}

void ProposalManager::AddQC(std::unique_ptr<QC> qc) {
  if (qc == nullptr) {
    return;
  }
  std::unique_lock<std::mutex> lk(p_mutex_);
  if (high_qc_.hash().empty() || high_qc_.view() < qc->view()) {
    high_qc_ = *qc;
  }
}

void ProposalManager::UpdateLockedQC(const QC& qc) {
  if (qc.hash().empty()) {
    return;
  }
  std::unique_lock<std::mutex> lk(p_mutex_);
  if (locked_qc_.hash().empty() || locked_qc_.view() < qc.view()) {
    locked_qc_ = qc;
  }
}

std::unique_ptr<Proposal> ProposalManager::AddChainProposal(std::unique_ptr<Proposal> p) {
  if (p == nullptr) {
    return nullptr;
  }

  std::unique_lock<std::mutex> lk(p_mutex_);
  const std::string proposal_hash = p->hash();
  const int proposal_view = p->slot_id();
  if (!p->justify_qc().hash().empty() &&
      (high_qc_.hash().empty() || high_qc_.view() < p->justify_qc().view())) {
    high_qc_ = p->justify_qc();
  }
  pending_proposals_[proposal_view] = std::make_unique<Proposal>(*p);

  if (chain_proposals_.find(proposal_hash) == chain_proposals_.end()) {
    if (!p->parent_hash().empty()) {
      proposal_children_[p->parent_hash()].insert(proposal_hash);
    }
    chain_proposals_[proposal_hash] = std::move(p);
  }

  auto commit_ready = TryCommitChainLocked(proposal_hash);
  if (commit_ready != nullptr) {
    return commit_ready;
  }
  auto child_it = proposal_children_.find(proposal_hash);
  if (child_it != proposal_children_.end()) {
    for (const auto& child_hash : child_it->second) {
      commit_ready = TryCommitChainLocked(child_hash);
      if (commit_ready != nullptr) {
        return commit_ready;
      }
    }
  }
  return nullptr;
}

std::unique_ptr<Proposal> ProposalManager::TryCommitChainLocked(
    const std::string& hash) {
  const Proposal* proposal = GetProposalByHash(hash);
  if (proposal == nullptr) {
    return nullptr;
  }

  const Proposal* parent = GetProposalByHash(proposal->parent_hash());
  if (parent == nullptr || proposal->justify_qc().hash() != parent->hash() ||
      proposal->slot_id() != parent->slot_id() + 1) {
    return nullptr;
  }

  if (locked_qc_.hash().empty() ||
      locked_qc_.view() < proposal->justify_qc().view()) {
    locked_qc_ = proposal->justify_qc();
  }

  const Proposal* grandparent = GetProposalByHash(parent->parent_hash());
  if (grandparent == nullptr ||
      parent->justify_qc().hash() != grandparent->hash() ||
      parent->slot_id() != grandparent->slot_id() + 1) {
    return nullptr;
  }

  if (committed_proposals_.find(grandparent->hash()) !=
      committed_proposals_.end()) {
    return nullptr;
  }
  committed_proposals_.insert(grandparent->hash());
  return std::make_unique<Proposal>(*grandparent);
}

bool ProposalManager::HasChainProposal(const std::string& hash) {
  std::unique_lock<std::mutex> lk(p_mutex_);
  return chain_proposals_.find(hash) != chain_proposals_.end();
}

std::vector<Proposal> ProposalManager::GetProposalChain(const std::string& hash, int limit) {
  std::vector<Proposal> proposals;
  std::unique_lock<std::mutex> lk(p_mutex_);
  std::string current_hash = hash;
  while (!current_hash.empty() && proposals.size() < limit) {
    const Proposal* proposal = GetProposalByHash(current_hash);
    if (proposal == nullptr) {
      break;
    }
    proposals.push_back(*proposal);
    current_hash = proposal->parent_hash();
  }
  return proposals;
}

std::vector<Proposal> ProposalManager::GetRecentProposals(int limit) {
  std::vector<Proposal> proposals;
  std::unique_lock<std::mutex> lk(p_mutex_);
  for (auto it = pending_proposals_.rbegin();
       it != pending_proposals_.rend() && proposals.size() < limit; ++it) {
    if (it->second != nullptr) {
      proposals.push_back(*it->second);
    }
  }
  return proposals;
}

std::map<int, int64_t> ProposalManager::GetKnownBlockHeights() {
  std::map<int, int64_t> heights;
  std::unique_lock<std::mutex> lk(mutex_);
  for (int sender = 1; sender <= total_num_; ++sender) {
    if (!pending_blocks_[sender].empty()) {
      heights[sender] = pending_blocks_[sender].rbegin()->first;
    }
  }
  // Also expose our own un-certified candidate blocks so peers can pull
  // them via AskBlockBatch. Without this, a minority owner that produced
  // block N+1 but failed to collect f+1 acks during a partition will keep
  // block N+1 only in blocks_candidates_, the RecoveryState advertises
  // height=N, peers never request N+1, and the block is stuck forever.
  if (!blocks_candidates_.empty()) {
    int64_t high = blocks_candidates_.rbegin()->first;
    auto it = heights.find(id_);
    if (it == heights.end() || it->second < high) {
      heights[id_] = high;
    }
  }
  return heights;
}

int64_t ProposalManager::GetKnownBlockHeight(int sender) {
  std::unique_lock<std::mutex> lk(mutex_);
  int64_t known = 0;
  if (!pending_blocks_[sender].empty()) {
    known = pending_blocks_[sender].rbegin()->first;
  }
  if (sender == id_ && !blocks_candidates_.empty()) {
    known = std::max<int64_t>(known, blocks_candidates_.rbegin()->first);
  }
  return known;
}

void ProposalManager::GarbageCollect(int committed_slot) {
  const int keep_window = 128;
  const int self_keep_window = 8192;
  int min_slot = committed_slot - keep_window;
  if (min_slot <= 0) {
    return;
  }

  // GC pending_blocks_ for remote owners aggressively, but keep a much deeper
  // history for our own payloads. After a healed partition, lagging peers may
  // still need very old blocks from the original owner to bridge the first
  // missing hole; if the owner trims to just high-128, recovery asks for the
  // historical block return blocks:0 forever.
  {
    std::unique_lock<std::mutex> blk(mutex_);
    for (int sender = 1; sender < 512; ++sender) {
      if (pending_blocks_[sender].empty()) continue;
      int64_t high = pending_blocks_[sender].rbegin()->first;
      int64_t sender_keep_window =
          (sender == id_) ? self_keep_window : keep_window;
      int64_t low_keep = std::max<int64_t>(0, high - sender_keep_window);
      while (!pending_blocks_[sender].empty() &&
             pending_blocks_[sender].begin()->first < low_keep) {
        pending_blocks_[sender].erase(pending_blocks_[sender].begin());
      }
      // fur_ is a per-sender side buffer; bound it with the same retention
      // rule so owner-local out-of-order payloads survive long enough to serve
      // recovery pulls as well.
      if (!fur_[sender].empty()) {
        while (!fur_[sender].empty() &&
               fur_[sender].begin()->first < low_keep) {
          fur_[sender].erase(fur_[sender].begin());
        }
      }
    }
  }

  std::unique_lock<std::mutex> lk(p_mutex_);
  for (auto it = pending_proposals_.begin(); it != pending_proposals_.end();) {
    if (it->first < min_slot) {
      it = pending_proposals_.erase(it);
    } else {
      ++it;
    }
  }

  for (auto it = chain_proposals_.begin(); it != chain_proposals_.end();) {
    if (it->second != nullptr && it->second->slot_id() < min_slot) {
      it = chain_proposals_.erase(it);
    } else {
      ++it;
    }
  }

  for (auto it = proposal_children_.begin(); it != proposal_children_.end();) {
    if (chain_proposals_.find(it->first) == chain_proposals_.end()) {
      it = proposal_children_.erase(it);
    } else {
      for (auto child_it = it->second.begin(); child_it != it->second.end();) {
        if (chain_proposals_.find(*child_it) == chain_proposals_.end()) {
          child_it = it->second.erase(child_it);
        } else {
          ++child_it;
        }
      }
      ++it;
    }
  }
}

bool ProposalManager::SafeNode(const Proposal& proposal) {
  std::unique_lock<std::mutex> lk(p_mutex_);
  if (locked_qc_.hash().empty()) {
    return true;
  }
  if (proposal.justify_qc().view() > locked_qc_.view()) {
    return true;
  }
  return ExtendsLockedBlock(proposal);
}

bool ProposalManager::ExtendsLockedBlock(const Proposal& proposal) {
  std::string hash = proposal.parent_hash();
  while (!hash.empty()) {
    if (hash == locked_qc_.hash()) {
      return true;
    }
    const Proposal* parent = GetProposalByHash(hash);
    if (parent == nullptr) {
      return false;
    }
    hash = parent->parent_hash();
  }
  return false;
}

const Proposal* ProposalManager::GetProposalByHash(const std::string& hash) {
  auto it = chain_proposals_.find(hash);
  if (it == chain_proposals_.end()) {
    return nullptr;
  }
  return it->second.get();
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

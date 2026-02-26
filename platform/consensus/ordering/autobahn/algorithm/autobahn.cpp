#include "platform/consensus/ordering/autobahn/algorithm/autobahn.h"

#include <glog/logging.h>

#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"


namespace resdb {
namespace autobahn {

AutoBahn::AutoBahn(int id, int f, int total_num, int block_size, SignatureVerifier* verifier)
    : ProtocolBase(id, f, total_num), verifier_(verifier) {

  LOG(ERROR) << "get proposal graph";
  id_ = id;
  total_num_ = total_num;
  f_ = f;
  is_stop_ = false;
  //timeout_ms_ = 100;
  timeout_ms_ = 60000;
  batch_size_ = block_size;
  execute_id_ = 1;

  proposal_manager_ = std::make_unique<ProposalManager>(id, total_num_, f_, verifier);

  block_thread_ = std::thread(&AutoBahn::GenerateBlocks, this);
  dissemi_thread_ = std::thread(&AutoBahn::AsyncDissemination, this);
  consensus_thread_ = std::thread(&AutoBahn::AsyncConsensus, this);
  prepare_thread_ = std::thread(&AutoBahn::AsyncPrepare, this);
  commit_thread_ = std::thread(&AutoBahn::AsyncCommit, this);
}

AutoBahn::~AutoBahn() {
  is_stop_ = true;
  if (block_thread_.joinable()) {
    block_thread_.join();
  }
  if (dissemi_thread_.joinable()) {
    dissemi_thread_.join();
  }
}

bool AutoBahn::IsStop() {
  return is_stop_;
}

bool AutoBahn::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  // LOG(ERROR)<<"recv txn:";
  txn->set_create_time(GetCurrentTime());
  txns_.Push(std::move(txn));
  return true;
}

void AutoBahn::GenerateBlocks() {
  std::vector<std::unique_ptr<Transaction>> txns;
  while (!IsStop()) {
    std::unique_ptr<Transaction> txn = txns_.Pop();
    if (txn == nullptr) {
      continue;
    }
    txns.push_back(std::move(txn));
    for(int i = 1; i < batch_size_; ++i){
      std::unique_ptr<Transaction> txn = txns_.Pop(100);
      if(txn == nullptr){
        break;
      }
      txns.push_back(std::move(txn));
    }

    proposal_manager_->MakeBlock(txns);
    txns.clear();
  }
}

bool AutoBahn::WaitForResponse(int64_t block_id) {
  std::unique_lock<std::mutex> lk(bc_mutex_);
  //LOG(ERROR)<<"wait for block id :"<<block_id;
  bc_block_cv_.wait_for(lk, std::chrono::microseconds(timeout_ms_ * 1000),
      [&] { return block_id<= proposal_manager_->GetCurrentBlockId(); });
  if (block_id > proposal_manager_->GetCurrentBlockId()) {
    return false;
  }
  return true;
}

void AutoBahn::BlockDone() {
  bc_block_cv_.notify_all();
}

void AutoBahn::AsyncDissemination() {
  int next_block = 1;
  while (!IsStop()) {
    if(WaitForResponse(next_block-1)){
      next_block++;
    }
    if(next_block == 1) {
      next_block++;
    }
    LOG(ERROR)<<" get block :"<<next_block-1;
    const Block* block = proposal_manager_->GetLocalBlock(next_block-1);
    if(block == nullptr) {
      next_block--;
      continue;
    }
    LOG(ERROR)<<" broadcast block :"<<next_block-1<<" id:"<<block->local_id();
    Broadcast(MessageType::NewBlocks, *block);
    LOG(ERROR)<<" broadcast block :"<<next_block-1<<" id:"<<block->local_id()<<" done";
  }
}

void AutoBahn::NotifyView() {
  std::unique_lock<std::mutex> lk(view_mutex_);
  view_cv_.notify_all();
}

bool AutoBahn::WaitForNextView(int view) {
  std::unique_lock<std::mutex> lk(view_mutex_);
  //LOG(ERROR)<<"wait for next view:"<<view;
  view_cv_.wait_for(lk, std::chrono::microseconds(timeout_ms_ * 1000),
      [&] { return proposal_manager_->ReadyView(view); });
  return proposal_manager_->ReadyView(view);
}

void AutoBahn::AsyncConsensus() {
  while (!IsStop()) {
    if (id_ != 1) {
      break;
    }
    int view = proposal_manager_->GetCurrentView();
    if(!WaitForNextView(view)) {
      continue;
    }
    std::pair<int, std::map<int, int64_t>> blocks = proposal_manager_->GetCut();
    auto proposal = proposal_manager_->GenerateProposal(blocks.first, blocks.second);
    Broadcast(MessageType::NewProposal, *proposal);
  }
}


void AutoBahn::ReceiveBlock(std::unique_ptr<Block> block) {
  LOG(ERROR)<<"recv block from:"<<block->sender_id()<<" block id:"<<block->local_id();
  BlockACK block_ack;
  block_ack.set_hash(block->hash());
  block_ack.set_sender_id(block->sender_id());
  block_ack.set_local_id(block->local_id());
  block_ack.set_responder(id_);
  *block_ack.mutable_sign_info() = proposal_manager_->SignBlock(*block);

  proposal_manager_->AddBlock(std::move(block));
  SendMessage(MessageType::CMD_BlockACK, block_ack, block_ack.sender_id());

  proposal_manager_->UpdateView(block_ack.sender_id(), block_ack.local_id());
  NotifyView();
  LOG(ERROR)<<"send block ack to:"<<block_ack.sender_id()<<" block id:"<<block_ack.local_id();
}

void AutoBahn::ReceiveBlockACK(std::unique_ptr<BlockACK> block) {
  LOG(ERROR)<<"recv block ack:"<<block->local_id()<<" from:"<<block->responder()<<" block sign info:"<<block->sign_info().sender_id();
  std::unique_lock<std::mutex> lk(block_mutex_);
  block_ack_[block->local_id()].insert(std::make_pair(block->responder(), block->sign_info()));
  LOG(ERROR)<<"recv block ack:"<<block->local_id()
    <<" from:"<<block->responder()<< " num:"<<block_ack_[block->local_id()].size();
  if (block_ack_[block->local_id()].size() >= f_ + 1 &&
      block_ack_[block->local_id()].find(id_) !=
          block_ack_[block->local_id()].end()) {
    std::unique_lock<std::mutex> lk(bc_mutex_);
    proposal_manager_->BlockReady(block_ack_[block->local_id()], block->local_id());
    BlockDone();
  }
  LOG(ERROR)<<"recv block ack:"<<block->local_id()<<" done";
}

bool AutoBahn::ReceiveProposal(std::unique_ptr<Proposal> proposal) {
  LOG(ERROR)<<" receive proposal from:"<<proposal->sender_id()<<" slot:"<<proposal->slot_id()<<" block size:"<<proposal->block_size();
  Proposal vote;
  vote.set_slot_id(proposal->slot_id());
  vote.set_sender_id(id_);
  vote.set_hash(proposal->hash());

  auto hash_signature_or = verifier_->SignMessage(vote.hash());
  if (!hash_signature_or.ok()) {
    LOG(ERROR) << "Sign message fail";
    return false;
  }
  *vote.mutable_sign()=*hash_signature_or;

  int sender_id = proposal->sender_id();
  proposal_manager_->AddProposalData(std::move(proposal));
  //Broadcast(MessageType::ProposalAck, vote);
  SendMessage(MessageType::ProposalAck, vote, sender_id);

  return true;
}

bool AutoBahn::ReceiveVote(std::unique_ptr<Proposal> vote) {
  LOG(ERROR)<<"recv vote ack:"<<vote->slot_id()<<" from:"<<vote->sender_id(); 

  std::unique_ptr<Proposal> vote_cpy = std::make_unique<Proposal>(*vote);

  std::unique_lock<std::mutex> lk(vote_mutex_);
  int slot_id = vote->slot_id();
  int sender = vote->sender_id();
  vote_ack_[vote->slot_id()].insert(std::make_pair(vote->sender_id(), std::move(vote)));

  LOG(ERROR)<<"recv vote ack:"<<slot_id<<" from:"<<sender
    << " num:"<<vote_ack_[slot_id].size();

  if (vote_ack_[slot_id].size() >= 2*f_ + 1){
      PrepareDone(std::move(vote_cpy));
  }
  LOG(ERROR)<<"recv vote ack done";
  return true;
}


bool AutoBahn::IsFastCommit(const Proposal& proposal) {
  
  //LOG(ERROR)<<" is fast commit slot:"<<proposal.slot_id()<<" sign size:"<<proposal.cert().sign_size();
  if(proposal.cert().sign_size() != total_num_) {
    return false;
  }

  for(auto& sign : proposal.cert().sign()){
    bool valid = verifier_->VerifyMessage(proposal.hash(), sign);
    if (!valid) {
      LOG(ERROR)<<" sign info sign fail";
      return false;
    }
  }
  return true;
}

bool AutoBahn::ReceivePrepare(std::unique_ptr<Proposal> proposal) {
  //LOG(ERROR)<<"recv prepare:"<<proposal->slot_id()<<" from:"<<proposal->sender_id()
  //        <<" is fast commit:"<<proposal->fast_commit(); 
  // verify
  if (IsFastCommit(*proposal)){
 // proposal->fast_commit() ){
    CommitDone(std::move(proposal));
  }
  else {
    proposal->set_sender_id(id_);
    Broadcast(MessageType::Commit, *proposal);
  }
  LOG(ERROR)<<"recv vote ack done";
  return true;
}

bool AutoBahn::ReceiveCommit(std::unique_ptr<Proposal> proposal) {
  //LOG(ERROR)<<"recv commit:"<<proposal->slot_id()<<" from:"<<proposal->sender_id();

  std::unique_lock<std::mutex> lk(commit_mutex_);
  commit_ack_[proposal->slot_id()].insert(proposal->sender_id());
  LOG(ERROR)<<"recv commit ack:"<<proposal->slot_id()<<" from:"<<proposal->sender_id()
    << " num:"<<commit_ack_[proposal->slot_id()].size();
  if (commit_ack_[proposal->slot_id()].size() >= 2*f_ + 1){
    CommitDone(std::move(proposal));
  }
  LOG(ERROR)<<"recv vote ack done";
  return true;
}


void AutoBahn::PrepareDone(std::unique_ptr<Proposal> vote) {
  LOG(ERROR)<<" vote prepare done:"<<vote->slot_id();
  prepare_queue_.Push(std::move(vote));
}

void AutoBahn::CommitDone(std::unique_ptr<Proposal> proposal) {
  commit_queue_.Push(std::move(proposal));
}

void AutoBahn::AsyncPrepare() {
  int view = 1;
  std::map<int, std::pair<int64_t,std::unique_ptr<Proposal>> > votes;
  while (!IsStop()) {
    std::unique_ptr<Proposal> p = prepare_queue_.Pop(timeout_ms_ * 1000);
    if(p== nullptr) {
      continue;
    }
    assert(p != nullptr);
    //LOG(ERROR)<<" obtain slot vote:"<<p->slot_id();
    int slot_id = p->slot_id();
    votes[slot_id] = std::make_pair(GetCurrentTime(), std::move(p));
    while(!votes.empty() && votes.begin()->first <= view) {
      if(votes.begin()->first < view) {
        votes.erase(votes.begin());
        continue;
      }
      int delay = 1000;
      int wait_time = GetCurrentTime() - votes.begin()->second.first;
      wait_time = delay - wait_time;
      LOG(ERROR)<<" view :"<<view<<" wait time:"<<wait_time;
      if(wait_time> 0) {
        usleep(wait_time);
      }
      Prepare(std::move(votes.begin()->second.second));
      votes.erase(votes.begin());
      view++;
    }
  }
}

void AutoBahn::AsyncCommit() {
  int view = 1;
  std::map<int, std::unique_ptr<Proposal> > proposals;
  while (!IsStop()) {
    std::unique_ptr<Proposal> p = commit_queue_.Pop(timeout_ms_ * 1000);
    if(p== nullptr) {
      continue;
    }
    assert(p != nullptr);
    //LOG(ERROR)<<" obtain comit slot vote:"<<p->slot_id();
    int slot_id = p->slot_id();
    proposals[slot_id] = std::move(p);
    while(!proposals.empty() && proposals.begin()->first <= view) {
      if(proposals.begin()->first < view) {
        proposals.erase(proposals.begin());
        continue;
      }
      Commit(std::move(proposals.begin()->second));
      proposals.erase(proposals.begin());
      view++;
    }
  }
}

void AutoBahn::Prepare(std::unique_ptr<Proposal> vote) {
  //LOG(ERROR)<<" prepare vote:"<<vote->slot_id()<< " num:"<<vote_ack_[vote->slot_id()].size();
  if (vote_ack_[vote->slot_id()].size() == total_num_){
    // fast path
    //Commit(std::move(vote));
    vote->set_fast_commit(true);
    for(auto& it : vote_ack_[vote->slot_id()]){
      *vote->mutable_cert()->add_sign() = it.second->sign();
    }
  }

  // slot path
  //LOG(ERROR)<<" broadcast commit:"<<vote->slot_id()<<" is fast:"<<vote->fast_commit();
  vote->set_sender_id(id_);
  Broadcast(MessageType::Prepare, *vote);
}

void AutoBahn::Commit(std::unique_ptr<Proposal> proposal) {
  auto raw_proposal = proposal_manager_->GetProposalData(proposal->slot_id());
  assert(raw_proposal != nullptr);
  //LOG(ERROR)<<" proposal proposal slot id:"<<proposal->slot_id();
  for(const auto& block : raw_proposal->block()) {
    int block_owner = block.sender_id();
    int block_id = block.local_id();
    //LOG(ERROR)<<" commit :"<<block_owner<<" block id :"<<block_id;
  
    Block * data_block = proposal_manager_->GetBlock(block_owner, block_id);
    assert(data_block != nullptr);

    //LOG(ERROR)<<" txn size:"<<data_block->mutable_data()->transaction_size();
    for (Transaction& txn :
        *data_block->mutable_data()->mutable_transaction()) {
      txn.set_id(execute_id_++);
      commit_(txn);
    }
  }
}


}  // namespace autobahn
}  // namespace resdb

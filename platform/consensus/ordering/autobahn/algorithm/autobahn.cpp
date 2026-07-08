#include "platform/consensus/ordering/autobahn/algorithm/autobahn.h"

#include <algorithm>

#include <glog/logging.h>

#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"


namespace resdb {
namespace autobahn {

AutoBahn::AutoBahn(int id, int f, int total_num, int block_size, SignatureVerifier* verifier)
    : ProtocolBase(id, f, total_num), verifier_(verifier) {

  //LOG(ERROR) << "get proposal graph";
  id_ = id;
  total_num_ = total_num;
  f_ = f;
  is_stop_ = false;
  timeout_ms_ = 800;
  batch_size_ = block_size;
  execute_id_ = 1;
  is_leader_ = id_ == 1;
  cur_slot_ = 1;
  use_hs_ = true;
  leader_slot_ = 0;
  last_commit_time_.store(GetCurrentTime());
  last_progress_time_.store(last_commit_time_.load());
  recovery_target_slot_.store(1);

  proposal_manager_ = std::make_unique<ProposalManager>(id, total_num_, f_, verifier);
  global_stats_ = Stats::GetGlobalStats();

  //block_thread_ = std::thread(&AutoBahn::GenerateBlocks, this);
  dissemi_thread_ = std::thread(&AutoBahn::AsyncDissemination, this);
  consensus_thread_ = std::thread(&AutoBahn::AsyncConsensus, this);
  prepare_thread_ = std::thread(&AutoBahn::AsyncPrepare, this);
  commit_thread_ = std::thread(&AutoBahn::AsyncCommit, this);
  recovery_thread_ = std::thread(&AutoBahn::AsyncRecovery, this);
}

AutoBahn::~AutoBahn() {
  is_stop_ = true;
  if (block_thread_.joinable()) {
    block_thread_.join();
  }
  if (dissemi_thread_.joinable()) {
    dissemi_thread_.join();
  }
  if (consensus_thread_.joinable()) {
    consensus_thread_.join();
  }
  if (prepare_thread_.joinable()) {
    prepare_thread_.join();
  }
  if (commit_thread_.joinable()) {
    commit_thread_.join();
  }
  if (recovery_thread_.joinable()) {
    recovery_thread_.join();
  }
}

bool AutoBahn::IsStop() {
  return is_stop_;
}

void AutoBahn::MarkProgress() {
  last_progress_time_.store(GetCurrentTime());
  consecutive_recovery_timeout_count_.store(0);
}

bool AutoBahn::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  //LOG(ERROR) << "receive txn node:" << id_;
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
    if(!WaitForResponse(next_block-1)){
      // Snapshot the block under proposal_manager's lock to keep the
      // Broadcast (protobuf serialize) decoupled from concurrent mutations
      // of blocks_candidates_ / pending_blocks_.
      Block pending_block;
      if (proposal_manager_->CopyLocalCandidateBlock(next_block - 1,
                                                     &pending_block)) {
        //LOG(ERROR)<<" rebroadcast block :"<<next_block-1
        //          <<" id:"<<pending_block.local_id();
        Broadcast(MessageType::NewBlocks, pending_block);
      }
      continue;
    }
    //LOG(ERROR)<<" get block :"<<next_block;

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
      break;
    }
    const Block* block = proposal_manager_->GetLocalBlock(next_block);
    if(block == nullptr) {
      continue;
    }
    //LOG(ERROR)<<" broadcast block node:"<<id_<<" block:"<<next_block
    //          <<" id:"<<block->local_id();

    Broadcast(MessageType::NewBlocks, *block);
    //LOG(ERROR)<<" broadcast block :"<<next_block<<" id:"<<block->local_id()<<" done";
    next_block++;
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

bool AutoBahn::WaitForNextLeader() {
  std::unique_lock<std::mutex> lk(leader_mutex_);
  if(is_leader_){
    return true;
  }
  leader_cv_.wait_for(lk, std::chrono::microseconds(timeout_ms_ * 1000),
      [&] { return is_leader_; });
  LOG(ERROR)<<"wait for next leader:"<<is_leader_;
  return is_leader_;
}

int AutoBahn::NextLeader(int slot_id) {
  int leader = slot_id % total_num_;
  if (leader == 0) {
    leader = total_num_;
  }
  return leader;
}

void AutoBahn::StartNextLeader(int slot_id) {
  int leader = NextLeader(slot_id);
  if (leader != id_) {
    is_leader_ = false;
    return;
  }

  std::unique_lock<std::mutex> lk(leader_mutex_);
  if (slot_id < cur_slot_ || proposed_slots_.find(slot_id) != proposed_slots_.end() ||
      (slot_id == cur_slot_ && is_leader_)) {
    return;
  }
  LOG(ERROR)<<" start leader slot:"<<slot_id;
  cur_slot_ = slot_id;
  proposal_manager_->SetCurrentView(slot_id);
  is_leader_ = true;
  MarkProgress();
  recovery_target_slot_.store(slot_id + 1);
  leader_cv_.notify_all();
}

void AutoBahn::SendNewLeaderRequest(int slot_id) {
  int leader = NextLeader(slot_id);

  VoteMsg msg;
  msg.set_slot_id(slot_id);
  msg.set_sender_id(id_);
  msg.set_leader(leader);
  *msg.mutable_high_qc() = proposal_manager_->GetHighQC();
  Broadcast(MessageType::CMD_NewLeader, msg);
  ReceiveNewLeader(std::make_unique<VoteMsg>(msg));
  //LOG(ERROR)<<" broadcast new leader request leader:"<<leader<<" slot:"<<slot_id;
}

void AutoBahn::AsyncConsensus() {
  while (!IsStop()) {
    if(!WaitForNextLeader()){
      continue;
    }
    int view = 0;
    {
      std::unique_lock<std::mutex> lk(leader_mutex_);
      view = cur_slot_;
      if (proposed_slots_.find(view) != proposed_slots_.end()) {
        is_leader_ = false;
        continue;
      }
      proposed_slots_.insert(view);
      is_leader_ = false;
    }

    LOG(ERROR)<<"wait for next view:"<<view;
    if(!WaitForNextView(view)) {
      continue;
    }
    std::pair<int, std::map<int, int64_t>> blocks;
    blocks = proposal_manager_->GetCut(view);
    int slot_id = view;
    auto proposal = proposal_manager_->GenerateProposal(slot_id, blocks.second);
    Broadcast(MessageType::NewProposal, *proposal);
    ReceiveProposal(std::make_unique<Proposal>(*proposal));
    //LOG(ERROR)<<" broadcast chained hotstuff slot:"<<slot_id;
  }
}


void AutoBahn::ReceiveBlock(std::unique_ptr<Block> block) {
  //LOG(ERROR)<<"recv block from:"<<block->sender_id()<<" block id:"<<block->local_id();
  BlockACK block_ack;
  block_ack.set_hash(block->hash());
  block_ack.set_sender_id(block->sender_id());
  block_ack.set_local_id(block->local_id());
  block_ack.set_responder(id_);
  *block_ack.mutable_sign_info() = proposal_manager_->SignBlock(*block);
  int sender = block->sender_id();
  int block_id = block->local_id();

  bool add_ret = true;
  bool is_recovery = block->is_recovery();
  {
    std::unique_lock<std::mutex> lk(block_mutex_);
    add_ret = proposal_manager_->AddBlock(std::move(block));

    if(add_ret){
      // Always send an ack back to the block owner, including for blocks
      // we obtained through the recovery (AskBlockBatch / AskBlock) path.
      // The owner's BlockReady predicate counts unique acks: if a minority
      // owner can only collect acks from its own partition group during a
      // partition, recovery-path acks from the majority side are the ONLY
      // way to push the count past f+1 after the partition heals.
      // Suppressing them previously left blocks pinned in
      // blocks_candidates_ forever (see logs: rebroadcast block:74 every
      // 100ms with acks only from {10..19}).
      SendMessage(MessageType::CMD_BlockACK, block_ack, block_ack.sender_id());
      (void)is_recovery;

      proposal_manager_->UpdateView(sender,
                                    proposal_manager_->GetCertifiedBlockHeight(sender));
      NotifyView();
    }
    //LOG(ERROR)<<"recv block from:"<<sender<<" block id:"<<block_id<<" ret:"<<add_ret;
  }
  // add_ret == false means the predecessor block is missing; the new block
  // is parked in fur_ and will get linked when the missing predecessor
  // arrives. Ask for that predecessor directly; AskBlockBatch is rate-limited,
  // so repeated out-of-order recovery blocks coalesce instead of storming.
  if (!add_ret && block_id > 1) {
    int64_t next_execute_block = 0;
    {
      std::unique_lock<std::mutex> lk(execute_mutex_);
      next_execute_block = commit_block_[sender] + 1;
    }
    if (block_id <= next_execute_block + 1) {
      int prev_block_id = block_id - 1;
      if (sender != id_) {
        AskBlockBatch(sender, prev_block_id, prev_block_id, sender);
      }
    }
  }
  if (add_ret) {
    DrainCommittedBlocks(sender);
  }

  //LOG(ERROR)<<"send block ack to:"<<block_ack.sender_id()<<" block id:"<<block_ack.local_id();
}

void AutoBahn::ReceiveBlockACK(std::unique_ptr<BlockACK> block) {
  //LOG(ERROR)<<"recv block ack:"<<block->local_id()<<" from:"<<block->responder()<<" block sign info:"<<block->sign_info().sender_id();
  std::unique_lock<std::mutex> lk(block_mutex_);
  block_ack_[block->local_id()].insert(std::make_pair(block->responder(), block->sign_info()));
  //LOG(ERROR)<<"recv block ack:"<<block->local_id()
  //  <<" from:"<<block->responder()<< " num:"<<block_ack_[block->local_id()].size();
  if (block_ack_[block->local_id()].size() >= f_ + 1 &&
      block_ack_[block->local_id()].find(id_) !=
          block_ack_[block->local_id()].end()) {
    std::unique_lock<std::mutex> lk(bc_mutex_);
    proposal_manager_->BlockReady(block_ack_[block->local_id()], block->local_id());
    proposal_manager_->UpdateView(id_, proposal_manager_->GetCertifiedBlockHeight(id_));
    NotifyView();
    BlockDone();
  }
  //LOG(ERROR)<<"recv block ack:"<<block->local_id()<<" done";
}

void AutoBahn::ReceiveNewLeader(std::unique_ptr<VoteMsg> msg) {
  bool should_start = false;
  int slot_id = msg->slot_id();
  int leader = msg->leader();
  if (proposal_manager_->VerifyQC(msg->high_qc())) {
    proposal_manager_->AddQC(std::make_unique<QC>(msg->high_qc()));
  }
  {
    std::unique_lock<std::mutex> lk(recv_slot_mutex_);
    //LOG(ERROR)<<" recv leader from:"<<msg->sender_id()<<" slot:"<<slot_id<<" leader:"<<leader<<" self id:"<<id_<<" max recv:"<<max_recv_slot_;
    if (slot_id < max_recv_slot_) {
      //return;
    }
    if (slot_id > max_recv_slot_) {
      max_recv_slot_ = slot_id;
    }
  }
  {
    std::unique_lock<std::mutex> lk(leader_mutex_);
    new_leader_ack_[slot_id].insert(msg->sender_id());
    LOG(ERROR)<<" leader slot:"<<leader_slot_<<" slot id:"<<slot_id<<" num:"<<new_leader_ack_[slot_id].size();
    if (new_leader_ack_[slot_id].size() >= 2 * f_ + 1) {
      LOG(ERROR)<<" leader slot:"<<leader_slot_<<" slot id:"<<slot_id<<" leader:"<<leader<<" id:"<<id_;
      if(leader_slot_ < slot_id){
        leader_slot_ = slot_id;
        if(leader == id_){ 
          should_start = true;
        }
        else {
          is_leader_ = false;
        }
      }
      if (slot_id > max_recv_slot_) {
        max_recv_slot_ = slot_id;
      }
      //LOG(ERROR)<<" leader slot:"<<leader_slot_<<" slot id:"<<slot_id<<" leader:"<<leader<<" id:"<<id_<<" should start:"<<should_start;
    }
  }

  if (!should_start) {
    return;
  }

  StartNextLeader(slot_id);
}

/*
uint32_t fail_time = 0;
std::set<int> cur_fail;
*/

bool AutoBahn::ReceiveProposal(std::unique_ptr<Proposal> proposal) {
  LOG(ERROR)<<" receive proposal from:"<<proposal->sender_id()<<" slot:"<<proposal->slot_id()<<" block size:"<<proposal->block_size();

  if (proposal->sender_id() != NextLeader(proposal->slot_id())) {
    LOG(ERROR) << "proposal leader mismatch slot:" << proposal->slot_id()
               << " sender:" << proposal->sender_id();
    return true;
  }

  if (proposal_manager_->GetProposalHash(*proposal) != proposal->hash() ||
      !proposal_manager_->VerifyQC(proposal->justify_qc()) ||
      (!proposal->justify_qc().hash().empty() &&
       proposal->justify_qc().hash() != proposal->parent_hash())) {
    //LOG(ERROR) << "proposal data verify fail slot:" << proposal->slot_id();
    return false;
  }

  int proposal_slot = proposal->slot_id();
  std::string proposal_hash = proposal->hash();
  recovery_target_slot_.store(
      std::max(recovery_target_slot_.load(), proposal_slot + 1));
  auto committed_proposal =
      proposal_manager_->AddChainProposal(std::make_unique<Proposal>(*proposal));
  if (committed_proposal != nullptr) {
    CommitDone(std::move(committed_proposal));
  }
  if (!proposal->parent_hash().empty() &&
      !proposal_manager_->HasChainProposal(proposal->parent_hash())) {
    //LOG(ERROR) << "proposal parent missing slot:" << proposal->slot_id();
    AskProposal(proposal->sender_id(), proposal->parent_hash());
  }

  if (!proposal_manager_->VerifyProposal(*proposal)) {
    LOG(ERROR) << "proposal not safe yet slot:" << proposal->slot_id();
    return true;
  }
  MarkProgress();

  QC high_qc = proposal_manager_->GetHighQC();
  if (!high_qc.hash().empty() && high_qc.hash() == proposal->hash() &&
      NextLeader(high_qc.view() + 1) == id_) {
    StartNextLeader(high_qc.view() + 1);
  }

  Proposal vote;
  vote.set_slot_id(proposal_slot);
  vote.set_sender_id(id_);
  vote.set_hash(proposal_hash);

  auto hash_signature_or = verifier_->SignMessage(vote.hash());
  if (!hash_signature_or.ok()) {
    LOG(ERROR) << "Sign message fail";
    return false;
  }
  *vote.mutable_sign()=*hash_signature_or;

  SendMessage(MessageType::ProposalAck, vote, NextLeader(proposal_slot + 1));

  return true;
}

bool AutoBahn::ReceiveVote(std::unique_ptr<Proposal> vote) {
  LOG(ERROR)<<"recv vote ack:"<<vote->slot_id()<<" from:"<<vote->sender_id(); 

  if (!verifier_->VerifyMessage(vote->hash(), vote->sign())) {
    LOG(ERROR) << "Verify vote signature fail";
    return false;
  }

  std::unique_lock<std::mutex> lk(vote_mutex_);
  int slot_id = vote->slot_id();
  int sender = vote->sender_id();
  std::string hash = vote->hash();
  auto& votes = chain_vote_ack_[slot_id][hash];
  votes.insert(std::make_pair(sender, std::move(vote)));

  LOG(ERROR)<<"recv vote ack:"<<slot_id<<" from:"<<sender
    << " hash votes:"<<votes.size();

  const auto qc_key = std::make_pair(slot_id, hash);
  if (votes.size() >= 2*f_ + 1 &&
      formed_qc_.find(qc_key) == formed_qc_.end()){
    formed_qc_.insert(qc_key);
    auto qc = std::make_unique<QC>();
    qc->set_view(slot_id);
    qc->set_hash(hash);
    for(auto& it : votes){
      *qc->add_signatures() = it.second->sign();
    }
    proposal_manager_->AddQC(std::move(qc));
    MarkProgress();
    recovery_target_slot_.store(
        std::max(recovery_target_slot_.load(), slot_id + 1));
    if (proposal_manager_->HasChainProposal(hash)) {
      StartNextLeader(slot_id + 1);
    } else {
      LOG(ERROR) << "qc formed before proposal arrives slot:" << slot_id;
    }
  }
  //LOG(ERROR)<<"recv vote ack done";
  return true;
}


bool AutoBahn::IsFastCommit(const Proposal& proposal) {
  
  LOG(ERROR)<<" is fast commit slot:"<<proposal.slot_id()<<" sign size:"<<proposal.cert().sign_size();
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
  LOG(ERROR)<<"recv prepare:"<<proposal->slot_id()<<" from:"<<proposal->sender_id()
          <<" is fast commit:"<<proposal->fast_commit(); 
  // verify
  if (IsFastCommit(*proposal)){
 // proposal->fast_commit() ){
    CommitDone(std::move(proposal));
  }
  else {
    proposal->set_sender_id(id_);
    Broadcast(MessageType::Commit, *proposal);
  }
  //LOG(ERROR)<<"recv vote ack done";
  return true;
}

bool AutoBahn::ReceiveCommit(std::unique_ptr<Proposal> proposal) {
  LOG(ERROR)<<"recv commit:"<<proposal->slot_id()<<" from:"<<proposal->sender_id();

  std::unique_lock<std::mutex> lk(commit_mutex_);
  commit_ack_[proposal->slot_id()].insert(proposal->sender_id());
  LOG(ERROR)<<"recv commit ack:"<<proposal->slot_id()<<" from:"<<proposal->sender_id()
    << " num:"<<commit_ack_[proposal->slot_id()].size();
  if (commit_ack_[proposal->slot_id()].size() >= 2*f_ + 1){
    CommitDone(std::move(proposal));
  }
  //LOG(ERROR)<<"recv vote ack done";
  return true;
}


void AutoBahn::PrepareDone(std::unique_ptr<Proposal> vote) {
  //LOG(ERROR)<<" vote prepare done:"<<vote->slot_id();
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
    if(!use_hs_){
      view = std::max(view, leader_slot_);
    }
    //LOG(ERROR)<<" obtain slot vote:"<<p->slot_id()<<" view:"<<view;
    int slot_id = p->slot_id();
    votes[slot_id] = std::make_pair(GetCurrentTime(), std::move(p));
    if(use_hs_){
      view = slot_id;
    }
    while(!votes.empty() && votes.begin()->first <= view) {
      if(votes.begin()->first < view) {
        votes.erase(votes.begin());
        continue;
      }
      int delay = 0;
      int wait_time = GetCurrentTime() - votes.begin()->second.first;
      wait_time = delay - wait_time;
      //LOG(ERROR)<<" view :"<<view<<" wait time:"<<wait_time;
      if(wait_time> 0) {
        usleep(wait_time);
      }
      Prepare(std::move(votes.begin()->second.second));
      if(use_hs_){
        view = votes.begin()->first+1;
      }
      else {
        view++;
      }
      votes.erase(votes.begin());
    }
  }
}

void AutoBahn::AsyncCommit() {
  std::map<int, std::unique_ptr<Proposal> > proposals;
  while (!IsStop()) {
    std::unique_ptr<Proposal> p = commit_queue_.Pop(timeout_ms_ * 1000);
    if(p== nullptr && proposals.empty()) {
      LOG(ERROR)<<" no data";
      continue;
    }
    if(p != nullptr) {
      int slot_id = p->slot_id();
      //LOG(ERROR)<<"slot:"<<slot_id;
      proposals[slot_id] = std::move(p);
    //LOG(ERROR)<<" obtain chained commit node:"<<id_<<" slot:"<<slot_id;
      MarkProgress();
    }
    if(!proposals.empty()){
      //LOG(ERROR)<<" begin first commit slot:"<<proposals.begin()->first;
    }
    while(!proposals.empty()) {
      if(proposals.begin()->first <= last_committed_slot_.load()) {
        proposals.erase(proposals.begin());
        continue;
      }
      int committed_slot = proposals.begin()->first;
      LOG(ERROR)<<" commit:"<<committed_slot<<" null:"<<(proposals.begin()->second == nullptr);;
      assert(proposals.begin()->second != nullptr);
      if(!Commit(std::make_unique<Proposal>(*proposals.begin()->second))){
        break;
      }
      last_committed_slot_.store(committed_slot);
      last_commit_time_.store(GetCurrentTime());
      MarkProgress();
      proposals.erase(proposals.begin());
      proposal_manager_->GarbageCollect(committed_slot);
      // GC ack maps so they don't grow unbounded after a partition where
      // many block_ack_[N] never reached f+1 quorum.
      GarbageCollectAcks();
    }
    //LOG(ERROR)<<"done";
  }
}

void AutoBahn::GarbageCollectAcks() {
  // Keep acks for ~keep_window blocks behind the highest known local block.
  // Since BlockReady moves blocks from candidates into pending_blocks_[id_],
  // anything older than that no longer needs its ack record retained.
  const int keep_window = 256;
  int64_t self_known = proposal_manager_->GetKnownBlockHeight(id_);
  int64_t cutoff = self_known - keep_window;
  if (cutoff <= 0) return;

  std::unique_lock<std::mutex> lk(block_mutex_);
  while (!block_ack_.empty() && block_ack_.begin()->first < cutoff) {
    block_ack_.erase(block_ack_.begin());
  }
  // Bound batch-request dedup map (already capped to 4096 inside
  // AskBlockBatch but defensive trim here too).
  std::unique_lock<std::mutex> blk(b_mutex_);
  if (last_batch_req_time_.size() > 2048) {
    int64_t now = GetCurrentTime();
    for (auto it = last_batch_req_time_.begin();
         it != last_batch_req_time_.end();) {
      if (now - it->second > 2000000) {
        it = last_batch_req_time_.erase(it);
      } else {
        ++it;
      }
    }
  }
  if (last_block_resp_time_.size() > 2048) {
    int64_t now = GetCurrentTime();
    for (auto it = last_block_resp_time_.begin();
         it != last_block_resp_time_.end();) {
      if (now - it->second > 2000000) {
        it = last_block_resp_time_.erase(it);
      } else {
        ++it;
      }
    }
  }
  if (last_block_req_time_.size() > 2048) {
    int64_t now = GetCurrentTime();
    for (auto it = last_block_req_time_.begin();
         it != last_block_req_time_.end();) {
      if (now - it->second > 2000000) {
        it = last_block_req_time_.erase(it);
      } else {
        ++it;
      }
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

void AutoBahn::AskBlock(int sender, int block_id) {
  BlockACK block_ack;
  block_ack.set_sender_id(id_);
  block_ack.set_local_id(block_id);
  block_ack.set_responder(sender);
  int target = sender;
  {
    std::unique_lock<std::mutex> lk(b_mutex_);
    auto key = std::make_pair(sender, block_id);
    int64_t now = GetCurrentTime();
    auto it = last_block_req_time_.find(key);
    if (it != last_block_req_time_.end() && now - it->second < 1000000) {
      return;
    }
    last_block_req_time_[key] = now;
    int round = block_req_round_[key]++;
    if (round > 0) {
      target = (sender + round - 1) % total_num_ + 1;
    }
  }
  //LOG(ERROR)<<" ack block:"<<sender<<" block id:"<<block_id
  //          <<" target:"<<target;
  SendMessage(MessageType::CMD_BlockReq, block_ack, target);
}

void AutoBahn::AskBlockBatch(int owner, int start_id, int end_id, int target) {
  if (start_id <= 0 || end_id < start_id) {
    return;
  }
  const int batch_limit = 64;
  end_id = std::min(end_id, start_id + batch_limit - 1);

  // Send-side dedup + rate limit. Without this, every 2s state-sync round
  // produces O(N) RecvStateResponse callers each calling AskBlockBatch with
  // overlapping (owner,start,end) → owner gets hit dozens of times for the
  // same single-block range. Symptom seen in production logs:
  //   "send block batch ack owner:1 start:131 end:131 ..." repeated 15+
  //   times within 50ms.
  {
    std::unique_lock<std::mutex> lk(b_mutex_);
    auto key = std::make_tuple(owner, target, start_id, end_id);
    int64_t now = GetCurrentTime();
    auto it = last_batch_req_time_.find(key);
    if (it != last_batch_req_time_.end() && now - it->second < 200000) {
      return;
    }
    last_batch_req_time_[key] = now;
    // Periodic GC so the map doesn't grow unbounded.
    if (last_batch_req_time_.size() > 4096) {
      for (auto mit = last_batch_req_time_.begin();
           mit != last_batch_req_time_.end();) {
        if (now - mit->second > 5000000) {
          mit = last_batch_req_time_.erase(mit);
        } else {
          ++mit;
        }
      }
    }
  }

  BlockBatch block_batch;
  block_batch.set_sender_id(id_);
  block_batch.set_owner_id(owner);
  block_batch.set_start_id(start_id);
  block_batch.set_end_id(end_id);

  //LOG(ERROR) << "ask block batch owner:" << owner << " start:" << start_id
  //           << " end:" << end_id << " target:" << target;
  SendMessage(MessageType::CMD_BlockBatchReq, block_batch, target);
}

void AutoBahn::RecvAskBlock(std::unique_ptr<BlockACK> ask_block) {
  
  int sender = ask_block->sender_id();
  int block_id = ask_block->local_id();
  int proposer = ask_block->responder();

  //LOG(ERROR)<<" receive ack block:"<<ask_block->sender_id()<<" block id:"<<ask_block->local_id()<<" proposer:"<<proposer;
  {
    std::unique_lock<std::mutex> lk(b_mutex_);
    auto key = std::make_tuple(sender, proposer, block_id);
    int64_t now = GetCurrentTime();
    auto it = last_block_resp_time_.find(key);
    // Tightened from 1s to 100ms so that legitimate retry storms during
    // recovery aren't all silently dropped. The previous 1s window meant
    // recovery could only respond once per second per (sender,proposer,id).
    if (it != last_block_resp_time_.end() && now - it->second < 100000) {
      return;
    }
  }
  // Take a defensive deep copy under proposal_manager's lock so the
  // subsequent SendMessage path (protobuf serialize) is decoupled from any
  // concurrent mutation of pending_blocks_ / blocks_candidates_. Returning
  // a raw pointer from GetBlock and then calling protobuf MergeFrom on it
  // racing with AddBlock / GarbageCollect / BlockReady triggered crashes
  // inside ArenaStringPtr::Set.
  Block data_block;
  bool got = proposal_manager_->CopyBlock(proposer, block_id, &data_block);
  if (!got && proposer == id_) {
    got = proposal_manager_->CopyLocalCandidateBlock(block_id, &data_block);
  }
  if (got) {
    //LOG(ERROR) << "send block req ack owner:" << data_block.sender_id()
    //           << " block:" << data_block.local_id() << " to:" << sender;
    SendMessage(MessageType::CMD_BlockReqAck, data_block, sender);
    {
      std::unique_lock<std::mutex> lk(b_mutex_);
      last_block_resp_time_[std::make_tuple(sender, proposer, block_id)] =
          GetCurrentTime();
    }
  }
}

void AutoBahn::RecvAskBlockBatch(std::unique_ptr<BlockBatch> block_batch) {
  if (block_batch == nullptr) {
    return;
  }
  const int requester = block_batch->sender_id();
  const int owner = block_batch->owner_id();
  const int start_id = block_batch->start_id();
  const int end_id = block_batch->end_id();

  BlockBatch response;
  response.set_sender_id(id_);
  response.set_owner_id(owner);
  response.set_start_id(start_id);
  response.set_end_id(end_id);

  const int batch_limit = 64;
  for (int block_id = start_id;
       block_id <= end_id && response.block_size() < batch_limit; ++block_id) {
    // CopyBlock holds proposal_manager's lock for the whole copy, so the
    // resulting Block is independent of pending_blocks_ and safe to pass
    // into protobuf MergeFrom even if another thread mutates that map.
    Block data_block;
    bool got = proposal_manager_->CopyBlock(owner, block_id, &data_block);
    if (!got && owner == id_) {
      got = proposal_manager_->CopyLocalCandidateBlock(block_id, &data_block);
    }
    if (got) {
      *response.add_block() = std::move(data_block);
    }
  }

  //LOG(ERROR) << "send block batch ack owner:" << owner << " start:" << start_id
  //           << " end:" << end_id << " blocks:" << response.block_size()
  //           << " to:" << requester;
  if (response.block_size() > 0) {
    SendMessage(MessageType::CMD_BlockBatchResp, response, requester);
  }
}

void AutoBahn::RecvAskBlockBatchAck(std::unique_ptr<BlockBatch> block_batch) {
  if (block_batch == nullptr) {
    return;
  }
  //LOG(ERROR) << "receive block batch ack owner:" << block_batch->owner_id()
  //           << " start:" << block_batch->start_id()
  //           << " end:" << block_batch->end_id()
  //           << " blocks:" << block_batch->block_size();
  MarkProgress();
  for (const auto& block : block_batch->block()) {
    auto recovered_block = std::make_unique<Block>(block);
    recovered_block->set_is_recovery(true);
    ReceiveBlock(std::move(recovered_block));
  }
}

void AutoBahn::RecvAskBlockAck(std::unique_ptr<Block> block) {

  //LOG(ERROR)<<" receive block ack sender:"<<block->sender_id()<<" block id:"<<block->local_id()<<" sign:"<<block->last_sign_info_size()<<" proposer:"<<block->responder();;
  int sender = block->sender_id();
  int block_id = block->local_id();
  int proposer = block->responder();

  {
    std::unique_lock<std::mutex> lk(b_mutex_);
    last_block_req_time_.erase(std::make_pair(sender, block_id));
    block_req_round_.erase(std::make_pair(sender, block_id));
    /*
    std::unique_lock<std::mutex> lk(b_mutex_);
    if(recv_.find(std::make_pair(sender, block_id)) != recv_.end()){
      return;
    }
    recv_.insert(std::make_pair(sender, block_id));
    */
    block->set_is_recovery(true);
  }
  MarkProgress();
  ReceiveBlock(std::move(block));
  //proposal_manager_->AddBlock(std::move(block));
}

void AutoBahn::AskProposal(int sender, const std::string& hash) {
  if (hash.empty()) {
    return;
  }
  ProposalQuery query;
  query.set_hash(hash);
  query.set_sender(id_);
  query.set_proposer(sender);
  //LOG(ERROR) << "ask proposal hash:" << hash << " from:" << sender;
  SendMessage(MessageType::CMD_ProposalQuery, query, sender);
}

void AutoBahn::RecvAskProposal(std::unique_ptr<ProposalQuery> proposal_query) {
  if (proposal_query == nullptr) {
    return;
  }
  ProposalQueryResp response;
  for (const auto& proposal :
       proposal_manager_->GetProposalChain(proposal_query->hash(), 3)) {
    *response.add_proposal() = proposal;
  }
  //LOG(ERROR) << "receive proposal query from:" << proposal_query->sender()
            // << " response size:" << response.proposal_size();
  if (response.proposal_size() > 0) {
    SendMessage(MessageType::CMD_ProposalQueryResponse, response,
                proposal_query->sender());
  }
}

void AutoBahn::RecvAskProposalAck(std::unique_ptr<ProposalQueryResp> proposals) {
  if (proposals == nullptr) {
    return;
  }
  for (const auto& proposal : proposals->proposal()) {
    RecoverProposal(proposal);
  }
}

bool AutoBahn::RecoverProposal(const Proposal& proposal) {
  if (proposal_manager_->GetProposalHash(proposal) != proposal.hash() ||
      !proposal_manager_->VerifyQC(proposal.justify_qc()) ||
      (!proposal.justify_qc().hash().empty() &&
       proposal.justify_qc().hash() != proposal.parent_hash())) {
    LOG(ERROR) << "recovered proposal verify fail slot:" << proposal.slot_id();
    return false;
  }

  auto committed_proposal =
      proposal_manager_->AddChainProposal(std::make_unique<Proposal>(proposal));
  if (committed_proposal != nullptr) {
    CommitDone(std::move(committed_proposal));
  }
  if (!proposal.parent_hash().empty() &&
      !proposal_manager_->HasChainProposal(proposal.parent_hash())) {
    AskProposal(proposal.sender_id(), proposal.parent_hash());
  }
  QC high_qc = proposal_manager_->GetHighQC();
  if (!high_qc.hash().empty() && high_qc.hash() == proposal.hash() &&
      NextLeader(high_qc.view() + 1) == id_) {
    StartNextLeader(high_qc.view() + 1);
  }
  return true;
}

RecoveryState AutoBahn::BuildRecoveryState() {
  RecoveryState recovery_state;
  recovery_state.set_sender_id(id_);
  *recovery_state.mutable_high_qc() = proposal_manager_->GetHighQC();
  *recovery_state.mutable_locked_qc() = proposal_manager_->GetLockedQC();
  recovery_state.set_committed_slot(last_committed_slot_.load());

  std::set<std::string> proposals;
  for (const auto& proposal : proposal_manager_->GetRecentProposals(8)) {
    if (proposals.insert(proposal.hash()).second) {
      *recovery_state.add_proposal() = proposal;
    }
  }
  if (!recovery_state.high_qc().hash().empty()) {
    for (const auto& proposal :
         proposal_manager_->GetProposalChain(recovery_state.high_qc().hash(), 8)) {
      if (proposals.insert(proposal.hash()).second) {
        *recovery_state.add_proposal() = proposal;
      }
    }
  }

  for (const auto& height : proposal_manager_->GetKnownBlockHeights()) {
    BlockHeight* block_height = recovery_state.add_block_height();
    block_height->set_sender_id(height.first);
    block_height->set_block_id(height.second);
  }
  return recovery_state;
}

void AutoBahn::BroadcastRecoveryState() {
  RecoveryState recovery_state = BuildRecoveryState();
  Broadcast(MessageType::CMD_StateReq, recovery_state);
}

void AutoBahn::AsyncRecovery() {
  const int64_t recovery_timeout_us = timeout_ms_ * 1000 * 2;
  // View change must skip a failed leader quickly enough for the majority
  // partition to keep committing while f consecutive leaders are isolated.
  const int64_t view_change_interval_us = timeout_ms_ * 1000;
  const int64_t state_sync_interval_us = 2000000;
  const int64_t dump_interval_us = 1000000;
  int64_t last_view_change_time = 0;
  int64_t last_state_sync_time = 0;
  int64_t last_dump_time = 0;
  int64_t last_observed_progress_time = GetCurrentTime();
  int observed_high_qc_view = 0;
  int observed_committed_slot = 0;
  while (!IsStop()) {
    usleep(timeout_ms_ * 1000);
    int64_t now = GetCurrentTime();
    if (now - last_state_sync_time > state_sync_interval_us) {
      BroadcastRecoveryState();
      last_state_sync_time = now;
    }
    if (now - last_dump_time > dump_interval_us) {
      // Per-owner dump used for partition-recovery diagnosis. Look for
      // "dump owner" lines to identify the OWNER whose certified_height
      // and pending_blocks_ are not advancing after partition heals; the
      // gap (known - cert) tells us whether we are missing the next block
      // (gap=1, owner stuck producing) or a batch tail (gap>1, recovery
      // pull is failing).
      std::map<int, int64_t> known_heights =
          proposal_manager_->GetKnownBlockHeights();
      for (const auto& kv : known_heights) {
        int sender = kv.first;
        int64_t known = kv.second;
        int64_t cert = proposal_manager_->GetCertifiedBlockHeight(sender);
        //LOG(ERROR) << "dump owner:" << sender << " known:" << known
        //           << " cert:" << cert << " gap:" << (known - cert);
      }
      QC dump_qc = proposal_manager_->GetHighQC();
      //LOG(ERROR) << "dump self_id:" << id_
      //           << " high_qc_view:" << dump_qc.view()
      //           << " committed_slot:" << last_committed_slot_.load()
      //           << " recovery_target:" << recovery_target_slot_.load();
      last_dump_time = now;
    }
    QC high_qc = proposal_manager_->GetHighQC();
    int high_qc_view = high_qc.view();
    int committed_slot = last_committed_slot_.load();
    if (high_qc_view > observed_high_qc_view ||
        committed_slot > observed_committed_slot) {
      observed_high_qc_view = high_qc_view;
      observed_committed_slot = committed_slot;
      last_observed_progress_time = now;
      consecutive_recovery_timeout_count_.store(0);
      recovery_target_slot_.store(
          std::max(recovery_target_slot_.load(), high_qc_view + 1));
      continue;
    }
    if (now - last_progress_time_.load() > recovery_timeout_us &&
        now - last_observed_progress_time > recovery_timeout_us &&
        now - last_view_change_time > view_change_interval_us) {
      int target_slot =
          std::max(recovery_target_slot_.load(),
                   high_qc_view + 1);
      recovery_target_slot_.store(target_slot + 1);
  //LOG(ERROR) << "trigger view recovery node:" << id_
  //           << " after commit stall slot:"
  //               << committed_slot << " target:" << target_slot;
      SendNewLeaderRequest(target_slot);
      if (consecutive_recovery_timeout_count_.load() % total_num_ == 0) {
        BroadcastRecoveryState();
      }
      last_view_change_time = now;
      consecutive_recovery_timeout_count_.fetch_add(1);
    }
  }
}

void AutoBahn::RecvStateRequest(std::unique_ptr<RecoveryState> recovery_state) {
  if (recovery_state == nullptr) {
    return;
  }
  RecoveryState response = BuildRecoveryState();
  SendMessage(MessageType::CMD_StateResp, response, recovery_state->sender_id());
}

void AutoBahn::RecvStateResponse(std::unique_ptr<RecoveryState> recovery_state) {
  if (recovery_state == nullptr) {
    return;
  }

  if (proposal_manager_->VerifyQC(recovery_state->high_qc())) {
    proposal_manager_->AddQC(std::make_unique<QC>(recovery_state->high_qc()));
  }
  if (proposal_manager_->VerifyQC(recovery_state->locked_qc())) {
    proposal_manager_->UpdateLockedQC(recovery_state->locked_qc());
  }

  for (const auto& proposal : recovery_state->proposal()) {
    RecoverProposal(proposal);
  }
  if (!recovery_state->high_qc().hash().empty() &&
      !proposal_manager_->HasChainProposal(recovery_state->high_qc().hash())) {
    AskProposal(recovery_state->sender_id(), recovery_state->high_qc().hash());
  }

  RequestMissingBlocks(*recovery_state);

  int next_slot = recovery_state->high_qc().view() + 1;
  if (next_slot > 0 && NextLeader(next_slot) == id_) {
    StartNextLeader(next_slot);
  }
}

void AutoBahn::RequestMissingBlocks(const RecoveryState& recovery_state) {
  // Do not issue block pulls from every RecoveryState response. Each state
  // message lists every owner's height, so N peers x N owners x multi-block
  // ranges creates a recovery request storm after a partition heals. Payload
  // pulls are driven only by the execution frontier in DrainCommittedBlocks(),
  // which targets the exact owner that is blocking progress.
  (void)recovery_state;
}


bool AutoBahn::Commit(std::unique_ptr<Proposal> proposal) {
  // chained-hotstuff semantics: a proposal carries only block IDs (a cut).
  // Slot-level commit MUST NOT be blocked by missing block payload, because
  // during a partition the owner of some block may be unreachable and the
  // chain itself keeps moving (a new leader from the majority will keep
  // forming QCs). We always advance the commit slot; if a block's
  // transaction payload is not yet locally available, committed_cut_block_
  // records the logical cut and DrainCommittedBlocks() fetches/executes the
  // payload once it arrives.
  auto raw_proposal = std::move(proposal);
  if (raw_proposal == nullptr) {
    return false;
  }
  int slot_id = raw_proposal->slot_id();
  //LOG(ERROR) << " commit proposal node:" << id_ << " slot id:" << slot_id;
  for (const auto& block : raw_proposal->block()) {
    int block_owner = block.sender_id();
    int block_id = block.local_id();
    {
      std::unique_lock<std::mutex> lk(execute_mutex_);
      committed_cut_block_[block_owner] =
          std::max<int64_t>(committed_cut_block_[block_owner], block_id);
    }
    DrainCommittedBlocks(block_owner);
  }
  return true;
}

void AutoBahn::DrainCommittedBlocks(int owner) {
  std::unique_lock<std::mutex> lk(execute_mutex_);
  while (!IsStop()) {
    int64_t block_id = commit_block_[owner] + 1;
    int64_t target = committed_cut_block_[owner];
    if (block_id > target) {
      return;
    }

    Block data_block;
    if (!proposal_manager_->CopyBlock(owner, block_id, &data_block)) {
      bool already_missing = false;
      {
        std::unique_lock<std::mutex> missing_lk(missing_mutex_);
        already_missing =
            missing_payloads_.find(std::make_pair(owner, block_id)) !=
            missing_payloads_.end();
      }
      if (!already_missing) {
        LOG(ERROR) << " missing block payload node:" << id_
                   << " owner:" << owner
                   << " id:" << block_id << " up_to:" << target;
      }
      lk.unlock();
      QueueMissingPayload(owner, block_id, target);
      return;
    }

    global_stats_->AddCommitTxn(data_block.mutable_data()->transaction_size());
    global_stats_->AddLatency(GetCurrentTime() - data_block.create_time());
    //LOG(ERROR) << " commit block node:" << id_ << " ownder:" << owner
    //           << " id:" << block_id
    //           << " delay:" << (GetCurrentTime() - data_block.create_time());
    for (Transaction& txn : *data_block.mutable_data()->mutable_transaction()) {
      txn.set_id(execute_id_++);
      commit_(txn);
    }
    commit_block_[owner] = block_id;
    proposal_manager_->SetExecutedBlockHeight(owner, block_id);
    {
      std::unique_lock<std::mutex> missing_lk(missing_mutex_);
      auto missing_key = std::make_pair(owner, block_id);
      missing_payloads_.erase(missing_key);
      last_missing_payload_req_time_.erase(missing_key);
    }
  }
}

// Records a missing payload range and pulls a small contiguous window from
// the execution frontier. Recovering one block at a time made post-partition
// throughput saw-tooth: commit one payload, stall on the next, request again.
// Pulling up to one batch lets DrainCommittedBlocks consume a run of blocks
// once the response arrives.
void AutoBahn::QueueMissingPayload(int owner, int64_t block_id,
                                   int64_t target_block_id) {
  if (block_id <= 0 || target_block_id < block_id) {
    return;
  }

  const int batch_limit = 8;
  const int64_t end_id = std::min<int64_t>(target_block_id,
                                           block_id + batch_limit - 1);
  const int64_t now = GetCurrentTime();
  {
    std::unique_lock<std::mutex> lk(missing_mutex_);
    auto request_key = std::make_pair(owner, block_id);
    auto it = last_missing_payload_req_time_.find(request_key);
    if (it != last_missing_payload_req_time_.end() &&
        now - it->second < 500000) {
      return;
    }
    for (int64_t id = block_id; id <= end_id; ++id) {
      missing_payloads_.insert(std::make_pair(owner, id));
      last_missing_payload_req_time_[std::make_pair(owner, id)] = now;
    }
  }

  if (owner != id_) {
    AskBlockBatch(owner, block_id, end_id, owner);
    return;
  }

  int peer = (id_ % total_num_) + 1;
  if (peer == id_) {
    peer = (peer % total_num_) + 1;
  }
  if (peer != id_) {
    AskBlockBatch(owner, block_id, end_id, peer);
  }
}


}  // namespace autobahn
}  // namespace resdb

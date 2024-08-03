#include "platform/consensus/ordering/slot_hs1/algorithm/slot_hs1.h"

#include <glog/logging.h>
#include "common/utils/utils.h"


namespace resdb {
namespace slot_hs1 {

SlotHotStuff1::SlotHotStuff1(int id, int f, int total_num, SignatureVerifier * verifier, int non_responsive_num, int fork_tail_num)
  : ProtocolBase(id, f, total_num), verifier_(verifier), non_responsive_num_(non_responsive_num), fork_tail_num_(fork_tail_num){

    LOG(ERROR)<<"id:"<<id<<" f:"<<f<<" total:"<<total_num_ << " non_responsive_num_: " << non_responsive_num_;

  global_stats_ = Stats::GetGlobalStats();
  proposal_manager_ = std::make_unique<ProposalManager>(id, 2*f_+1, slot_num_, verifier, total_num_, fork_tail_num_);
  has_sent_ = false;
  batch_size_ = 1;
  timer_length_ = 100000;
  
  ready_ = IsLeader(1);
    send_thread_ = std::thread(&SlotHotStuff1::AsyncSend, this);
    commit_thread_ = std::thread(&SlotHotStuff1::AsyncCommit, this);
   
}

SlotHotStuff1::~SlotHotStuff1() {
}

int SlotHotStuff1::NextLeader(int view){
  //LOG(ERROR)<<" view:"<<view<<" next leader:"<<(view+1)%total_num_ + 1;
  return (view+1)%total_num_ + 1;
}

bool SlotHotStuff1::IsLeader(int view){
  return (view % total_num_)+1 == id_;
}

bool SlotHotStuff1::Ready() {
  int view = proposal_manager_->CurrentView();
  // return IsLeader(view) && !has_sent_ && ready_;
  return IsLeader(view) && ready_;
}

void SlotHotStuff1::StartNewRound() {
  std::unique_lock<std::mutex> lk(n_mutex_);
  has_sent_ = false;
  vote_cv_.notify_one();
  //LOG(ERROR)<<" start new round";
}

void SlotHotStuff1::AsyncSend() {
  int slot = 0;
  uint64_t start_time = GetCurrentTime();
  while (!IsStop()) {
    auto txn = txns_.Pop();
    if(txn == nullptr){
      continue;
    }

    while(!IsStop()){
      std::unique_lock<std::mutex> lk(n_mutex_);
      vote_cv_.wait_for(lk, std::chrono::microseconds(1000),
          [&] { return Ready(); });

      if(Ready()){
        if (slot == 0) {
          start_time = GetCurrentTime();
          if (id_ % 3 == 1 && id_ < non_responsive_num_ * 3) {
            usleep(timer_length_);
          }
        }
        break;
      }
    }
    if(IsStop()){
      return;
    }

    std::vector<std::unique_ptr<Transaction> > txns;
    txns.push_back(std::move(txn));
    for(int i = 1; i < batch_size_; ++i){
      auto txn = txns_.Pop();
      if(txn == nullptr){
        continue;
        //break;
      }
      txns.push_back(std::move(txn));
    }

    std::unique_ptr<Proposal> proposal =  nullptr;
    {
      std::unique_lock<std::mutex> lk(mutex_);
      bool is_final = GetCurrentTime() - start_time >= timer_length_ ? true : false;
      proposal = proposal_manager_ -> GenerateProposal(txns, slot, is_final);
      slot = is_final ? 0 : slot + 1;
      //LOG(ERROR)<<"propose view:"<<proposal->header().view();
    }
    
    // LOG(ERROR) << "slot: " << slot;
    ready_ = false;
    // has_sent_ = true;
    broadcast_call_(MessageType::NewProposal, *proposal);
  }
}

void SlotHotStuff1::AsyncCommit() {
  int seq = 1;
  while (!IsStop()) {
    auto p = commit_q_.Pop();
    if(p == nullptr){
      continue;
    }
    // LOG(ERROR)<<"[X]commit proposal view: "<<p->header().view()<<" slot: "<<p->header().slot();
    global_stats_->AddConsensusLatency(GetCurrentTime()-p->createtime());
    for(Transaction& txn : *p->mutable_transactions()){
      txn.set_id(seq++);
      Commit(txn);
    }
  }
}


bool SlotHotStuff1::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  //  std::unique_lock<std::mutex> lk(txn_mutex_);
  txn->set_proposer(id_);
  txns_.Push(std::move(txn));
  return true;
}

bool SlotHotStuff1::ReceiveProposal(std::unique_ptr<Proposal> proposal) {
    int view = proposal->header().view();
    int slot = proposal->header().slot();
    bool is_final = proposal->header().is_final();
    int sender = proposal->sender();
    std::unique_ptr<Certificate> cert = nullptr;
  {
    // LOG(ERROR)<<"[X]process proposer view:"<<view<<" slot:"<<slot << " is_final: " << is_final;
    //LOG(ERROR)<<"RECEIVE proposer view:"<<proposal->header().view();
    std::unique_lock<std::mutex> lk(mutex_);
    if(!proposal_manager_->Verify(*proposal)){
      LOG(ERROR)<<" proposal invalid";
      return false;
    }

    cert = GenerateCertificate(*proposal);
    assert(cert != nullptr);

    
    std::unique_ptr<Proposal> committed_p = proposal_manager_->AddProposal(std::move(proposal));
    if(committed_p != nullptr){
      if (is_final) {
        for(Transaction& txn : *committed_p->mutable_transactions()){
          txn.set_next_primary(sender % total_num_ + 1);
        }
      }
      CommitProposal(std::move(committed_p));
    }

    //LOG(ERROR)<<"send cert view:"<<view<<" to:"<<NextLeader(view);
  }

  if (is_final) {
    SendMessage(MessageType::Vote, *cert, NextLeader(view));
  } else {
    SendMessage(MessageType::Vote, *cert, sender);
  }

  return true;
}

bool SlotHotStuff1::ReceiveCertificate(std::unique_ptr<Certificate> cert) {

  //LOG(ERROR)<<"RECEIVE proposer cert :"<<cert->view()<<" from:"<<cert->signer();
  std::unique_lock<std::mutex> lk(mutex_);
  //LOG(ERROR)<<"RECEIVE proposer cert :"<<cert->view()<<" from:"<<cert->signer();
  bool valid = proposal_manager_->VerifyCert(*cert);
  if(!valid){
    LOG(ERROR) << "Verify message fail";
    assert(1==0);
    return false;
  }

  int view = cert->view();
  std::string hash = cert->hash();
  int slot = cert->slot();
  bool is_final = cert->is_final();
  receive_[view][hash].insert(std::make_pair(cert->signer(), std::move(cert)));

  //LOG(ERROR)<<"RECEIVE proposer cert :"<<view<<" size:"<<receive_[view][hash].size();
  if(receive_[view][hash].size() == 2*f_+1){
    std::unique_ptr<QC> qc = std::make_unique<QC>();
    qc->set_hash(hash);
    qc->set_view(view);
    qc->set_slot(slot);
    qc->set_is_final(is_final);

    for(auto & it: receive_[view][hash]){
      *qc->add_signatures() = it.second->sign();
    }

    //LOG(ERROR)<<"add qc view:"<<view;
    proposal_manager_->AddQC(std::move(qc));
    ready_ = true;

    StartNewRound();
  }
  return true;
}

void SlotHotStuff1::CommitProposal(std::unique_ptr<Proposal> p){
  commit_q_.Push(std::move(p));
}

std::unique_ptr<Certificate> SlotHotStuff1::GenerateCertificate(const Proposal& proposal) {
  std::unique_ptr<Certificate> cert = std::make_unique<Certificate>();
  cert->set_hash(proposal.hash());
  cert->set_view(proposal.header().view());
  cert->set_slot(proposal.header().slot());
  cert->set_is_final(proposal.header().is_final());
  cert->set_signer(id_);

  std::string data_str = proposal.hash();
  auto hash_signature_or = verifier_->SignMessage(data_str);
  if (!hash_signature_or.ok()) {
    LOG(ERROR) << "Sign message fail";
    return nullptr;
  }
  *cert->mutable_sign()=*hash_signature_or;
  return cert;
}

}  // namespace slot_hs1
}  // namespace resdb
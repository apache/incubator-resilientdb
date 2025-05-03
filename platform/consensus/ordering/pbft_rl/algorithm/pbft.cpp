#include "platform/consensus/ordering/pbft_rl/algorithm/pbft.h"

#include <glog/logging.h>
#include "common/utils/utils.h"


namespace resdb {
namespace pbft_rl {

PBFT::PBFT(int id, int f, int total_num, SignatureVerifier* verifier, 
          common::ProtocolBase::SingleCallFuncType single_call,  
          common::ProtocolBase::BroadcastCallFuncType broadcast_call,
          std::function<void(std::unique_ptr<std::vector<std::unique_ptr<Transaction>>>)> commit)
  : ProtocolBase(id, f, total_num), verifier_(verifier), single_call_(single_call), broadcast_call_(broadcast_call), commit_(commit) {

    //f_ = (total_num_ + 2)/4;
    LOG(ERROR)<<"id:"<<id<<" f:"<<f_<<" total:"<<total_num_;

//  sleep(20);
    batch_size_ = 15;
    next_proposal_round_ = 1;
    next_lo_round_ = 1;
  proposal_manager_ = std::make_unique<ProposalManager>(id, 2*f_+1, f_,verifier);
  order_manager_ = std::make_unique<OrderManager>();
  has_sent_ = false;
  execute_id_ = 1;
  lo_thread_ = std::thread(&PBFT::AsyncSendLocalOrdering, this);
  send_thread_ = std::thread(&PBFT::AsyncSend, this);
  commit_thread_ = std::thread(&PBFT::AsyncCommit, this);
  wait_commit_thread_ = std::thread(&PBFT::AsyncWaitCommit, this);
  global_stats_ = Stats::GetGlobalStats();
}

PBFT::~PBFT() {
}

std::unique_ptr<Transaction> PBFT::FetchTxn(const std::string& hash){
  return proposal_manager_->FetchTxn(hash);
}

int PBFT::NextLeader(int view){
  //LOG(ERROR)<<" view:"<<view<<" next leader:"<<(view+1)%total_num_ + 1;
  return (view+1)%total_num_ + 1;
}

int PBFT::CurrentLeader(int view){
  //LOG(ERROR)<<" view:"<<view<<" next leader:"<<(view+1)%total_num_ + 1;
  return 1;
  //return (view)%total_num_ + 1;
}

bool PBFT::IsLeader(int view){
  return 1 == id_;
  //return (view % total_num_)+1 == id_;
}

bool PBFT::Ready() {
  int view = proposal_manager_->CurrentView();
  // LOG(ERROR)<<" manager view:"<<view<<" current view:"<<next_proposal_round_<<" leader:"<<IsLeader(view)<<" ready:"<< proposal_manager_->Ready(view) << " has_sent: "<< has_sent_;
  if(view != next_proposal_round_){
    return false; 
  }

  return IsLeader(view) && !has_sent_&& proposal_manager_->Ready(view);
}

void PBFT::StartNewRound() {
  //LOG(ERROR)<<" start new round"; 
  std::unique_lock<std::mutex> lk(n_mutex_);
  has_sent_ = false;
  vote_cv_.notify_one();
  //LOG(ERROR)<<" start new round";
}

void PBFT::AsyncSendLocalOrdering() {
  while (!IsStop()) {
    //std::unique_lock<std::mutex> lk(txn_mutex_);
    auto txn = txns_.Pop();
    if(txn == nullptr){
      continue;
    }

    if(IsStop()){
      return;
    }

    std::vector<std::unique_ptr<Transaction> > txns;
    txns.push_back(std::move(txn));
    for(int i = 1; ; ++i){
      auto txn = txns_.Pop();
      if(txn == nullptr){
        continue;
        //break;
      }
      txns.push_back(std::move(txn));
      if(txns.size()>=batch_size_){
        break;
      }
    }

    if (faulty_test_) {
      if (id_ % 3 == 1 && id_ < 3 * faulty_replica_num_) {
        std::reverse(txns.begin(), txns.end());
      }
    }

    while(true) {
      // LOG(ERROR) << "view comparison: " << next_lo_round_ << " " << proposal_manager_->CurrentView();
      if (next_lo_round_ == proposal_manager_->CurrentView()) {
        break;
      }
      usleep(1000);
    }

    std::unique_ptr<Proposal>  proposal = proposal_manager_->GenerateLocalOrdering(txns);
    proposal->mutable_header()->set_create_time(GetCurrentTime());
    LOG(ERROR)<<" bc local ordering size:"<<txns.size()<<" to leader:"<<CurrentLeader(proposal->header().view())<<" view:"<<proposal->header().view();
    fflush(stdout);
    single_call_(MessageType::NewOrdering, *proposal, CurrentLeader(proposal->header().view()));
    
    next_lo_round_++;
  }
}

void PBFT::AsyncSend() {
  while (!IsStop()) {
    while(!IsStop()){
      std::unique_lock<std::mutex> lk(n_mutex_);
      vote_cv_.wait_for(lk, std::chrono::microseconds(1000),
          [&] { return this->Ready(); });

      if(Ready()){
        break;
      }
    }
    if(IsStop()){
      return;
    }
  
    SendProposal();
  }
}


void PBFT::SendProposal() {
    std::unique_ptr<Proposal> proposal =  nullptr;
    proposal = proposal_manager_ -> GenerateProposal();
    next_proposal_round_++;
    has_sent_ = true;
    //LOG(ERROR)<<"propose view:"<<proposal->header().view()<<" local ordering:"<<proposal->local_ordering_size();
    broadcast_call_(MessageType::NewProposal, *proposal);
}

void PBFT::AsyncCommit() {
  int seq = 1;
  while (!IsStop()) {
    auto p = commit_q_.Pop();
    if(p == nullptr){
      continue;
    }

    LOG(ERROR)<<" commit proposal:"<<p->local_ordering_size()<<" view:"<<p->header().view()<<" queue time:"<<(GetCurrentTime() - p->header().create_time());
    int view = p->header().view();

    std::unique_ptr<std::vector<std::unique_ptr<Transaction> >> committed_lo_txn_list =     
      std::make_unique<std::vector<std::unique_ptr<Transaction> >>();
    int pro = 0;
    for(auto& lo : *p->mutable_local_ordering()){
      if (lo.sender() == id_) {
        proposal_manager_->ClearUncommittedLocalOrdering(p->header().view());
      }
      // LOG(ERROR)<<" commit proposal from:"<<lo.sender()<<" local view:"<<lo.header().view();
      for(Transaction& tx : *lo.mutable_transactions()){
        std::unique_ptr<Transaction > txn = std::make_unique<Transaction>();
        txn->set_hash(tx.hash());
        txn->set_proxy_id(tx.proxy_id());
        txn->set_proposer(lo.sender());
        txn->set_user_seq(tx.user_seq());
        // LOG(ERROR) << "txn with proxy_id: " << txn->proxy_id() << " user_seq: " <<  txn->user_seq();
        txn->set_create_time(tx.create_time());
        txn->set_queuing_time(tx.queuing_time());
        txn->set_proposal_id(pro);
        txn->set_round(view);
        committed_lo_txn_list->push_back(std::move(txn));  
      } 
      pro++;
    }
    (*committed_lo_txn_list)[0]->set_queuing_time(GetCurrentTime());
    commit_(std::move(committed_lo_txn_list));
    proposal_manager_->IncView();
    if (next_proposal_round_ == proposal_manager_->CurrentView() && 
        proposal_manager_->Ready(next_proposal_round_)) {
      has_sent_ = false;
    }
  }
}


bool PBFT::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
    //std::unique_lock<std::mutex> lk(txn_mutex_);
  // LOG(ERROR)<<" recv txn:";
  proposal_manager_->AddData(*txn);
  txn->set_proposer(id_);
  {
    std::unique_lock<std::mutex> lk(lo_mutex_);
    order_manager_->AddLocalOrderingRecord(txn->proxy_id(), txn->user_seq());
    txns_.Push(std::move(txn));
  }
  return true;
}

bool PBFT::ReceiveLocalOrdering(std::unique_ptr<Proposal> proposal) {
  //  std::unique_lock<std::mutex> lk(txn_mutex_);
  // LOG(ERROR)<<"receive local ordering from:"<<proposal->sender()<<" txn size:"<<proposal->transactions_size()<<" view:"<<proposal->header().view();
  
  uint64_t sender = proposal->sender();

  if (faulty_test_) {
    if (sender % 3 == 2 && sender < 3 * f_ && sender < 3 * faulty_replica_num_) {
      return true;
    }
  }  

  if(proposal_manager_->AddLocalOrdering(std::move(proposal))){
    //LOG(ERROR)<<" notify new round";
    StartNewRound();
  }
  return true;
}

void PBFT::CommitProposal(std::unique_ptr<Proposal> p){
  p->mutable_header()->set_create_time(GetCurrentTime());
  commit_q_.Push(std::move(p));
}

std::unique_ptr<Certificate> PBFT::GenerateCertificate(const Proposal& proposal) {
  std::unique_ptr<Certificate> cert = std::make_unique<Certificate>();
  cert->set_hash(proposal.hash());
  cert->set_view(proposal.header().view());
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


bool PBFT::ReceiveProposal(std::unique_ptr<Proposal> proposal) {
  std::string hash = proposal->hash();
  int64_t seq = proposal->header().view();
  Proposal new_proposal;
  new_proposal.set_hash(hash);
  *new_proposal.mutable_header() = proposal->header();
  {
    // LOG(ERROR)<<"recv proposal";
//    LOG(ERROR)<<"recv proposal from:"<<proposal->sender()<<" seq:"<<proposal->header().view()<<" proxy:"<<proposal->header().proposer_id();
    assert(proposal->header().proposer_id()>0);
    std::unique_lock<std::mutex> lk(mutex_);
    auto key = std::make_pair(proposal->header().proposer_id(), proposal->header().view());
    data_[key]=std::move(proposal);
  }

  {
    new_proposal.mutable_header()->set_view(seq);
    new_proposal.set_sender(id_);
    broadcast_call_(MessageType::Prepare, new_proposal);
  }
 // LOG(ERROR)<<"receive proposal done";
  return true;
}

bool PBFT::ReceivePrepare(std::unique_ptr<Proposal> proposal) {
  {
    int seq = proposal->header().view();
    std::string hash = proposal->hash();
    //LOG(ERROR)<<"recv proposal from:"<<proposal->sender()<<" id:"<<proposal->header().view();
    std::unique_lock<std::mutex> lk(mutex_);
    received_prepare_[hash].insert(proposal->sender());
    if(received_prepare_[hash].size()==3*f_+1){
        Proposal new_proposal;
        new_proposal.set_hash(hash);
        *new_proposal.mutable_header() = proposal->header();
        new_proposal.mutable_header()->set_view(seq);
        new_proposal.set_sender(id_);
        broadcast_call_(MessageType::CommitProposal, new_proposal);
        //LOG(ERROR)<<"receive proposal done";
    }
  }
   //LOG(ERROR)<<"receive proposal done";
  return true;
}

void PBFT::AsyncWaitCommit() {
  while (!IsStop()) {
    auto proposal = wait_commit_.Pop();
    if(proposal == nullptr){
      continue;
    }
    {
      std::unique_ptr<Proposal> commit_p= nullptr;
      while(commit_p == nullptr){
        std::unique_lock<std::mutex> lk(mutex_);
        auto key = std::make_pair(proposal->header().proposer_id(), proposal->header().view());
        auto it = data_.find(key);
        if(it != data_.end()){
          commit_p = std::move(it->second);
          data_.erase(it);
        }
      }
      CommitProposal(std::move(commit_p));
    }
  }
}

bool PBFT::ReceiveCommit(std::unique_ptr<Proposal> proposal) {
  std::unique_ptr<Proposal> commit_p= nullptr;
  {
    // LOG(ERROR)<<"recv commit proposal from:"<<proposal->sender()<<" seq:"<<proposal->header().view()<<" proxyid:"<<proposal->header().proposer_id();
    std::unique_lock<std::mutex> lk(mutex_);
    received_[proposal->hash()].insert(proposal->sender());
      if(received_[proposal->hash()].size()==3*f_+1){
         //LOG(ERROR)<<" commit";
         wait_commit_.Push(std::move(proposal));
      }
  }
  // LOG(ERROR)<<"receive proposal done";
  return true;
}


}  // namespace tusk
}  // namespace resdb

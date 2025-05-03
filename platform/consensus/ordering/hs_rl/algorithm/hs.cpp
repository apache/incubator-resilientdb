#include "platform/consensus/ordering/hs_rl/algorithm/hs.h"

#include <glog/logging.h>
#include "common/utils/utils.h"


namespace resdb {
namespace hs_rl {

HotStuff::HotStuff(int id, int f, int total_num, SignatureVerifier * verifier, 
                  common::ProtocolBase::SingleCallFuncType single_call,  
                  common::ProtocolBase :: BroadcastCallFuncType broadcast_call,
                  std::function<void(std::vector<Transaction*>& txns)> commit)
  : ProtocolBase(id, f, total_num), verifier_(verifier), single_call_(single_call), broadcast_call_(broadcast_call), commit_(commit){

    //f_ = (total_num_ + 2)/4;
    LOG(ERROR)<<"id:"<<id<<" f:"<<f_<<" total:"<<total_num_;

//  sleep(20);
    batch_size_ = 10;
  proposal_manager_ = std::make_unique<ProposalManager>(id, 2*f_+1, f_,verifier);
  has_sent_ = false;
  execute_id_ = 1;
    send_thread_ = std::thread(&HotStuff::AsyncSend, this);
    lo_thread_ = std::thread(&HotStuff::AsyncSendLocalOrdering, this);
    commit_thread_ = std::thread(&HotStuff::AsyncCommit, this);
  global_stats_ = Stats::GetGlobalStats();
  LOG(ERROR) << "Initiated HotStuff";
}

HotStuff::~HotStuff() {
}

int HotStuff::NextLeader(int view){
  //LOG(ERROR)<<" view:"<<view<<" next leader:"<<(view+1)%total_num_ + 1;
  return (view+1)%total_num_ + 1;
}

int HotStuff::CurrentLeader(int view){
  //LOG(ERROR)<<" view:"<<view<<" next leader:"<<(view+1)%total_num_ + 1;
  return (view)%total_num_ + 1;
}



bool HotStuff::IsLeader(int view){
  return (view % total_num_)+1 == id_;
}

bool HotStuff::Ready() {
  int view = proposal_manager_->CurrentView();
  return IsLeader(view) && !has_sent_&& proposal_manager_->Ready(view);
}

void HotStuff::StartNewRound() {
  //LOG(ERROR)<<" start new round"; 
  std::unique_lock<std::mutex> lk(n_mutex_);
  has_sent_ = false;
  vote_cv_.notify_one();
  //LOG(ERROR)<<" start new round";
}

void HotStuff::AsyncSendLocalOrdering() {
  while (!IsStop()) {
    //std::unique_lock<std::mutex> lk(txn_mutex_);
    auto txn = txns_.Pop();
    if(txn == nullptr){
      continue;
    }
    LOG(ERROR) << "Try to AsyncSendLocalOrdering";
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

    std::unique_ptr<Proposal>  proposal = proposal_manager_->GenerateLocalOrdering(txns);
    LOG(ERROR)<<" bc local ordering size:"<<txns.size()<<" to leader:"<<CurrentLeader(proposal->header().view());
    single_call_(MessageType::NewOrdering, *proposal, CurrentLeader(proposal->header().view()));
    LOG(ERROR) << "AsyncSendLocalOrdering Done";
  }
}

void HotStuff::AsyncSend() {
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
    //LOG(ERROR)<<"?????";

    std::unique_ptr<Proposal> proposal =  nullptr;
    {
      //std::unique_lock<std::mutex> lk(mutex_);
      //proposal_manager_ -> GenerateProposal(txns);
      proposal = proposal_manager_ -> GenerateProposal();
      //LOG(ERROR)<<"propose view:"<<proposal->header().view()<<" local ordering:"<<proposal->local_ordering_size();
    }
    has_sent_ = true;
    broadcast_call_(MessageType::NewProposal, *proposal);
  }
}

void HotStuff::AsyncCommit() {
  int seq = 1;
  while (!IsStop()) {
    auto p = commit_q_.Pop();
    if(p == nullptr){
      continue;
    }
    int round = p->header().view();;
   //LOG(ERROR)<<" commit proposal:"<<p->local_ordering_size()<<" view:"<<p->header().view();

    std::vector<Transaction* > txns;
    for(auto& lo : *p->mutable_local_ordering()){
      //LOG(ERROR)<<" commit proposal from:"<<lo.sender()<<" local view:"<<lo.header().view();
      for(Transaction& txn : *lo.mutable_transactions()){
        txn.set_round(round);
        txns.push_back(&txn);
      }
    }
    commit_(txns);
  }
}

bool HotStuff::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
    //std::unique_lock<std::mutex> lk(txn_mutex_);
    //LOG(ERROR)<<" recv txn:";
  txn->set_proposer(id_);
  txns_.Push(std::move(txn));
  return true;
}

bool HotStuff::ReceiveLocalOrdering(std::unique_ptr<Proposal> proposal) {
  //  std::unique_lock<std::mutex> lk(txn_mutex_);
  //LOG(ERROR)<<"receive local ordering from:"<<proposal->sender()<<" txn size:"<<proposal->transactions_size()<<" view:"<<proposal->header().view();
  if(proposal_manager_->AddLocalOrdering(std::move(proposal))){
    //LOG(ERROR)<<" notify new round";
    StartNewRound();
  }
  return true;
}


bool HotStuff::ReceiveProposal(std::unique_ptr<Proposal> proposal) {
    int view = proposal->header().view();
    std::unique_ptr<Certificate> cert = nullptr;
  {
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
      //LOG(ERROR)<<" commit proposal";
      CommitProposal(std::move(committed_p));
    }

    //LOG(ERROR)<<"send cert view:"<<view<<" to:"<<NextLeader(view);
  }

  single_call_(MessageType::Vote, *cert, NextLeader(view));

  return true;
}

bool HotStuff::ReceiveCertificate(std::unique_ptr<Certificate> cert) {

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
  receive_[view][hash].insert(std::make_pair(cert->signer(), std::move(cert)));

  //LOG(ERROR)<<"RECEIVE proposer cert :"<<view<<" size:"<<receive_[view][hash].size();
  if(receive_[view][hash].size() == 2*f_+1){
    std::unique_ptr<QC> qc = std::make_unique<QC>();
    qc->set_hash(hash);
    qc->set_view(view);

    for(auto & it: receive_[view][hash]){
      *qc->add_signatures() = it.second->sign();
    }

    //LOG(ERROR)<<"add qc view:"<<view;
    proposal_manager_->AddQC(std::move(qc));

    StartNewRound();
  }
  return true;
}

void HotStuff::CommitProposal(std::unique_ptr<Proposal> p){
  commit_q_.Push(std::move(p));
}

std::unique_ptr<Certificate> HotStuff::GenerateCertificate(const Proposal& proposal) {
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

std::unique_ptr<Transaction> HotStuff::FetchData(const std::string& hash){
  return proposal_manager_->FetchData(hash);
}


}  // namespace tusk
}  // namespace resdb

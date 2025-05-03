#include "platform/consensus/ordering/hs/algorithm/hs.h"

#include <glog/logging.h>
#include "common/utils/utils.h"


namespace resdb {
namespace hs {

HotStuff::HotStuff(int id, int f, int total_num, SignatureVerifier * verifier)
  : ProtocolBase(id, f, total_num), verifier_(verifier){

    LOG(ERROR)<<"id:"<<id<<" f:"<<f<<" total:"<<total_num_;

  proposal_manager_ = std::make_unique<ProposalManager>(id, 2*f_+1, verifier);
  has_sent_ = false;
    send_thread_ = std::thread(&HotStuff::AsyncSend, this);
    commit_thread_ = std::thread(&HotStuff::AsyncCommit, this);
    batch_size_ = 64;
}

HotStuff::~HotStuff() {
}

int HotStuff::NextLeader(int view){
  //LOG(ERROR)<<" view:"<<view<<" next leader:"<<(view+1)%total_num_ + 1;
  return (view+1)%total_num_ + 1;
}

bool HotStuff::IsLeader(int view){
  return (view % total_num_)+1 == id_;
}

bool HotStuff::Ready() {
  int view = proposal_manager_->CurrentView();
  return IsLeader(view) && !has_sent_;
}

void HotStuff::StartNewRound() {
  std::unique_lock<std::mutex> lk(n_mutex_);
  has_sent_ = false;
  vote_cv_.notify_one();
  //LOG(ERROR)<<" start new round";
}

void HotStuff::AsyncSend() {
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
      proposal = proposal_manager_ -> GenerateProposal(txns);
      //LOG(ERROR)<<"propose view:"<<proposal->header().view();
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

    for(Transaction& txn : *p->mutable_transactions()){
      txn.set_id(seq++);
      Commit(txn);
    }
  }
}


bool HotStuff::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  //  std::unique_lock<std::mutex> lk(txn_mutex_);
  txn->set_proposer(id_);
  txns_.Push(std::move(txn));
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
      CommitProposal(std::move(committed_p));
    }

    //LOG(ERROR)<<"send cert view:"<<view<<" to:"<<NextLeader(view);
  }

  SendMessage(MessageType::Vote, *cert, NextLeader(view));

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

}  // namespace tusk
}  // namespace resdb

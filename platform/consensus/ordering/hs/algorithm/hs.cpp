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
  batch_size_ = 10;
  timeout_ms_ = 100;
  tracked_view_ = 0;
  timeout_sent_view_ = 0;
  view_start_time_ = GetCurrentTime();
  send_thread_ = std::thread(&HotStuff::AsyncSend, this);
  commit_thread_ = std::thread(&HotStuff::AsyncCommit, this);
  timeout_thread_ = std::thread(&HotStuff::AsyncViewTimeout, this);
}

HotStuff::~HotStuff() {
  Stop();
  vote_cv_.notify_all();
  if (send_thread_.joinable()) {
    send_thread_.join();
  }
  if (commit_thread_.joinable()) {
    commit_thread_.join();
  }
  if (timeout_thread_.joinable()) {
    timeout_thread_.join();
  }
}

int HotStuff::NextLeader(int view){
  //LOG(ERROR)<<" view:"<<view<<" next leader:"<<(view+1)%total_num_ + 1;
  return (view+1)%total_num_ + 1;
}

bool HotStuff::IsLeader(int view){
  //LOG(ERROR)<<"view:"<<view<<" leader:"<<(view % total_num_)+1;
  return (view % total_num_)+1 == id_;
}

bool HotStuff::Ready() {
  int view = proposal_manager_->CurrentView();
  //LOG(ERROR)<<"view:"<<view<<" is leader:"<<IsLeader(view)<<" has sent:"<<has_sent_;
  return IsLeader(view) && !has_sent_;
}

void HotStuff::StartNewRound() {
  std::unique_lock<std::mutex> lk(n_mutex_);
  has_sent_ = false;
  UpdateViewTimer(proposal_manager_->CurrentView());
  vote_cv_.notify_one();
  LOG(ERROR)<<" start new round";
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
      auto txn = txns_.Pop(100);
      if(txn == nullptr){
        break;
      }
      txns.push_back(std::move(txn));
    }

    std::unique_ptr<Proposal> proposal =  nullptr;
    {
      std::unique_lock<std::mutex> lk(mutex_);
      proposal = proposal_manager_ -> GenerateProposal(txns);
      LOG(ERROR)<<"propose view:"<<proposal->header().view();
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
    LOG(ERROR)<<"RECEIVE proposer view:"<<proposal->header().view()<<" from:"<<proposal->header().proposer_id();
    std::unique_lock<std::mutex> lk(mutex_);
    if(!proposal_manager_->Verify(*proposal)){
      LOG(ERROR)<<" proposal invalid";
      return false;
    }
    received_proposal_views_.insert(view);
    timeout_cv_.notify_all();
    if (view >= proposal_manager_->CurrentView()) {
      UpdateViewTimer(view);
    }

    cert = GenerateCertificate(*proposal);
    assert(cert != nullptr);

    std::unique_ptr<Proposal> committed_p = proposal_manager_->AddProposal(std::move(proposal));
    if(committed_p != nullptr){
      CommitProposal(std::move(committed_p));
    }

    LOG(ERROR)<<"send cert view:"<<view<<" to:"<<NextLeader(view);
  }

  SendMessage(MessageType::Vote, *cert, NextLeader(view));

  return true;
}

bool HotStuff::ReceiveStartView(std::unique_ptr<StartView> start_view) {
  bool should_start = false;
  int view = start_view->view();
  {
    std::unique_lock<std::mutex> lk(mutex_);
    if (view < proposal_manager_->CurrentView()) {
      return true;
    }
    LOG(ERROR)<<"receive start view:"<<view<<" from:"<<start_view->sender_id();
    start_view_ack_[view].insert(start_view->sender_id());
    if (start_view_ack_[view].size() >= 2 * f_ + 1) {
      proposal_manager_->AdvanceView(view);
      has_sent_ = false;
      UpdateViewTimer(view);
      should_start = true;
    }
  }

  if (should_start) {
    std::unique_lock<std::mutex> lk(n_mutex_);
    vote_cv_.notify_one();
  }
  return true;
}

bool HotStuff::ReceiveCertificate(std::unique_ptr<Certificate> cert) {

  LOG(ERROR)<<"RECEIVE proposer cert :"<<cert->view()<<" from:"<<cert->signer();
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

  LOG(ERROR)<<"RECEIVE proposer cert :"<<view<<" size:"<<receive_[view][hash].size();
  if(receive_[view][hash].size() == 2*f_+1){
    std::unique_ptr<QC> qc = std::make_unique<QC>();
    qc->set_hash(hash);
    qc->set_view(view);

    for(auto & it: receive_[view][hash]){
      *qc->add_signatures() = it.second->sign();
    }

    LOG(ERROR)<<"add qc view:"<<view;
    proposal_manager_->AddQC(std::move(qc));

    StartNewRound();
  }
  return true;
}

void HotStuff::AsyncViewTimeout() {
  int timeout_ms = 200000;
  int current_view = 1;
  int count = 0;
  while (!IsStop()) {
    bool has_proposal = false;
    bool already_sent = false;
    uint64_t start_time = 0;
    {
      std::unique_lock<std::mutex> lk(mutex_);
      timeout_cv_.wait_for(lk, std::chrono::microseconds(timeout_ms),
          [&] { return received_proposal_views_.find(current_view) != received_proposal_views_.end(); });

      while(current_view<=tracked_view_) {
        if(received_proposal_views_.find(current_view) !=
            received_proposal_views_.end()){
          received_proposal_views_.erase(received_proposal_views_.find(current_view));
          current_view++;
          has_proposal= true;
          count = 0;
        }
        else {
          break;
        }
      }
    }
    
    //LOG(ERROR)<<" timeout:"<<timeout_ms<<" view:"<<current_view<<" track:"<<tracked_view_<<" has_proposal:"<<has_proposal<<" count:"<<count;
    if (current_view>1 && !has_proposal){ 
      SendStartView(current_view  + count);
      count++;
    }
  }
}

void HotStuff::SendStartView(int view) {
  StartView start_view;
  start_view.set_view(view);
  start_view.set_sender_id(id_);
  LOG(ERROR)<<"send to next leader:"<<NextLeader(view - 1);
  SendMessage(MessageType::StartViewMsg, start_view, NextLeader(view - 1));
}

void HotStuff::UpdateViewTimer(int view) {
  LOG(ERROR)<<"update view:"<<view<<" tracked:"<<tracked_view_;
  tracked_view_ = std::max(tracked_view_, view);
  view_start_time_ = GetCurrentTime();
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

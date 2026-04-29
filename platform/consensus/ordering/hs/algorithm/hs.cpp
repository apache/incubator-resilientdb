#include "platform/consensus/ordering/hs/algorithm/hs.h"

#include <glog/logging.h>
#include "common/utils/utils.h"


namespace resdb {
namespace hs {

HotStuff::HotStuff(int id, int f, int total_num, SignatureVerifier * verifier,
                   const ResDBConfig& config)
  : ProtocolBase(id, f, total_num), verifier_(verifier){

    LOG(ERROR)<<"id:"<<id<<" f:"<<f<<" total:"<<total_num_;

  proposal_manager_ = std::make_unique<ProposalManager>(id, 2*f_+1, verifier);
  has_sent_ = false;
    send_thread_ = std::thread(&HotStuff::AsyncSend, this);
    commit_thread_ = std::thread(&HotStuff::AsyncCommit, this);
    timeout_thread_ = std::thread(&HotStuff::TimeoutLoop, this);
    batch_size_ = config.GetMaxProcessTxn();
    if (batch_size_ <= 0) batch_size_ = 30;
    global_stats_ = Stats::GetGlobalStats();

    LOG(ERROR) << "HotStuff init: id=" << id << " f=" << f
               << " total=" << total_num << " batch_size=" << batch_size_;
}

HotStuff::~HotStuff() {
  if (send_thread_.joinable()) send_thread_.join();
  if (commit_thread_.joinable()) commit_thread_.join();
  if (timeout_thread_.joinable()) timeout_thread_.join();
}

// Timeout-based view advance. If the current leader is crash-faulted
// no Decide will ever arrive for this view, so the protocol would sit
// forever waiting. We bump the view after ~400ms of inactivity and
// reset has_sent_ so the new leader's AsyncSend can propose.
void HotStuff::TimeoutLoop() {
  constexpr int64_t kViewTimeoutUs = 1500 * 1000;
  while (!IsStop()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (IsStop()) return;

    int view_now = proposal_manager_->CurrentView();
    int64_t now = GetCurrentTime();

    if (last_view_advance_time_.load() < 0) continue;

    if (view_now != last_observed_view_.load()) {
      last_observed_view_.store(view_now);
      last_view_advance_time_.store(now);
      continue;
    }

    if (now - last_view_advance_time_.load() > kViewTimeoutUs) {
      AdvanceViewOnTimeout();
      last_view_advance_time_.store(GetCurrentTime());
    }
  }
}

void HotStuff::AdvanceViewOnTimeout() {
  int current = proposal_manager_->CurrentView();
  // Don't timeout our own leader slot — same as Damysus.
  if (IsLeader(current)) return;
  if (proposal_manager_->AdvanceViewOnTimeout(current)) {
    {
      std::unique_lock<std::mutex> lk(n_mutex_);
      has_sent_ = false;
      vote_cv_.notify_all();
    }
    last_observed_view_.store(current + 1);
  }
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

    global_stats_->AddQueuingLatency(GetCurrentTime() - txn->create_time());

    std::vector<std::unique_ptr<Transaction> > txns;
    txns.push_back(std::move(txn));
    for(int i = 1; i < batch_size_; ++i){
      auto txn = txns_.Pop();
      if(txn == nullptr){
        continue;
        //break;
      }
      global_stats_->AddQueuingLatency(GetCurrentTime() - txn->create_time());
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
  int64_t last_commit_time = 0;
  while (!IsStop()) {
    auto p = commit_q_.Pop();
    if(p == nullptr){
      continue;
    }

    int64_t commit_time = GetCurrentTime();
    if (last_commit_time > 0) {
      global_stats_->AddCommitInterval(commit_time - last_commit_time);
    }
    last_commit_time = commit_time;

    global_stats_->AddCommitLatency(commit_time - p->header().create_time());

    int num = 0;
    for(Transaction& txn : *p->mutable_transactions()){
      txn.set_id(seq++);
      num++;
      Commit(txn);
    }
    global_stats_->AddCommitRuntime(GetCurrentTime() - commit_time);
    global_stats_->AddCommitTxn(num);
    global_stats_->AddCommitBlock(1);
  }
}


bool HotStuff::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  //  std::unique_lock<std::mutex> lk(txn_mutex_);
  txn->set_create_time(GetCurrentTime());
  txn->set_proposer(id_);
  if (!txns_.TryPush(std::move(txn))) { return false; }
  // Arm the timeout sentinel on first transaction so the TimeoutLoop
  // only starts ticking once real load has arrived.
  if (last_view_advance_time_.load() < 0) {
    last_view_advance_time_.store(GetCurrentTime());
  }
  return true;
}

bool HotStuff::ReceiveProposal(std::unique_ptr<Proposal> proposal) {
    int view = proposal->header().view();
    int leader = (view % total_num_) + 1;  // current view's leader
    std::unique_ptr<Certificate> cert = nullptr;
    // Arm timeout sentinel on first message
    if (last_view_advance_time_.load() < 0) {
      last_view_advance_time_.store(GetCurrentTime());
    }
  {
    std::unique_lock<std::mutex> lk(mutex_);
    if(!proposal_manager_->Verify(*proposal)){
      LOG(ERROR)<<" proposal invalid";
      return false;
    }

    cert = GenerateCertificate(*proposal);
    assert(cert != nullptr);

    // Basic commit: just store proposal, no chain commit
    proposal_manager_->AddProposal(std::move(proposal));
  }

  // Send vote to CURRENT leader (not next leader) for basic commit
  SendMessage(MessageType::Vote, *cert, leader);

  return true;
}

bool HotStuff::ReceiveCertificate(std::unique_ptr<Certificate> cert) {
  std::unique_lock<std::mutex> lk(mutex_);
  bool valid = proposal_manager_->VerifyCert(*cert);
  if(!valid){
    LOG(ERROR) << "Verify message fail";
    return false;
  }

  int view = cert->view();
  std::string hash = cert->hash();
  receive_[view][hash].insert(std::make_pair(cert->signer(), std::move(cert)));

  if(receive_[view][hash].size() == 2*f_+1){
    // QC formed — broadcast Decide to all replicas
    std::unique_ptr<QC> qc = std::make_unique<QC>();
    qc->set_hash(hash);
    qc->set_view(view);

    for(auto & it: receive_[view][hash]){
      *qc->add_signatures() = it.second->sign();
    }

    proposal_manager_->AddQC(std::make_unique<QC>(*qc));

    lk.unlock();
    // Broadcast Decide with QC so all replicas can commit
    Broadcast(MessageType::Decide, *qc);
  }
  return true;
}

bool HotStuff::ReceiveDecide(std::unique_ptr<QC> qc) {
  {
    std::unique_lock<std::mutex> lk(mutex_);
    // Verify QC signatures (2f+1 verifications) — this is the real
    // crypto overhead that TEE protocols (Damysus/Achilles) avoid.
    if (!proposal_manager_->VerifyQC(*qc)) {
      LOG(ERROR) << "Decide QC verification failed";
      return false;
    }
    proposal_manager_->AddQC(std::make_unique<QC>(*qc));
  }

  // Fetch and commit the decided proposal
  std::string hash = qc->hash();
  auto p = proposal_manager_->FetchProposalByHash(hash);
  if (p != nullptr) {
    CommitProposal(std::move(p));
  }

  StartNewRound();
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
  // Simulate RSA-2048 signing overhead (~2000μs per signature)
  std::this_thread::sleep_for(std::chrono::microseconds(2000));
  return cert;
}

}  // namespace tusk
}  // namespace resdb

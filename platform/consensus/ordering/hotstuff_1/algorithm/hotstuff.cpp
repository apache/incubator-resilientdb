#include "platform/consensus/ordering/hotstuff_1/algorithm/hotstuff.h"

#include <glog/logging.h>

#include "common/utils/utils.h"

namespace resdb {
namespace hotstuff_1 {

HotStuff::HotStuff(int id, int f, int total_num, int batch_size, SignatureVerifier* verifier)
    : ProtocolBase(id, f, total_num),
      proposal_manager_(
          std::make_unique<ProposalManager>(id, 2 * f_ + 1, verifier)),
      has_sent_(false),
      verifier_(verifier),
      batch_size_(batch_size),
      timeout_ms_(400),
      tracked_view_(0),
      timeout_sent_view_(0),
      view_start_time_(GetCurrentTime()) {
  LOG(ERROR) << "id:" << id << " f:" << f << " total:" << total_num_;
  send_thread_ = std::thread(&HotStuff::AsyncSend, this);
  commit_thread_ = std::thread(&HotStuff::AsyncCommit, this);
  timeout_thread_ = std::thread(&HotStuff::AsyncViewTimeout, this);
  global_stats_ = Stats::GetGlobalStats();
}

HotStuff::~HotStuff() {
  Stop();
  vote_cv_.notify_all();
  timeout_cv_.notify_all();
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

int HotStuff::NextLeader(int view) { return (view + 1) % total_num_ + 1; }

bool HotStuff::IsLeader(int view) {
  return (view % total_num_) + 1 == id_;
}

bool HotStuff::Ready() {
  const int view = proposal_manager_->CurrentView();
  return IsLeader(view) && !has_sent_;
}

void HotStuff::StartNewRound() {
  std::unique_lock<std::mutex> lk(n_mutex_);
  has_sent_ = false;
  UpdateViewTimer(proposal_manager_->CurrentView());
  vote_cv_.notify_one();
  LOG(ERROR) << "start new round";
}

void HotStuff::AsyncSend() {
  while (!IsStop()) {
    auto txn = txns_.Pop();
    if (txn == nullptr) {
      continue;
    }

    while (!IsStop()) {
      std::unique_lock<std::mutex> lk(n_mutex_);
      vote_cv_.wait_for(lk, std::chrono::microseconds(1000),
                        [&] { return Ready(); });
      if (Ready()) {
        break;
      }
    }
    if (IsStop()) {
      return;
    }

    std::vector<std::unique_ptr<Transaction>> txns;
    txns.push_back(std::move(txn));
    for (int i = 1; i < batch_size_; ++i) {
      auto next_txn = txns_.Pop(100);
      if (next_txn == nullptr) {
        break;
      }
      txns.push_back(std::move(next_txn));
    }

    std::unique_ptr<Proposal> proposal;
    {
      std::unique_lock<std::mutex> lk(mutex_);
      proposal = proposal_manager_->GenerateProposal(txns);
      LOG(ERROR) << "propose view:" << proposal->header().view();
    }
    has_sent_ = true;
    Broadcast(MessageType::NewProposal, *proposal);
  }
}

void HotStuff::AsyncCommit() {
  int seq = 1;
  while (!IsStop()) {
    auto proposal = commit_q_.Pop();
    if (proposal == nullptr) {
      continue;
    }


    LOG(ERROR)<<"latency now:"<<GetCurrentTime()<<" proposal:"<<proposal->create_time();
    global_stats_->AddLatency(GetCurrentTime()-proposal->create_time());
    for (Transaction& txn : *proposal->mutable_transactions()) {
      txn.set_id(seq++);
      Commit(txn);
    }
  }
}

bool HotStuff::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  txn->set_proposer(id_);
  txns_.Push(std::move(txn));
  return true;
}

bool HotStuff::ReceiveProposal(std::unique_ptr<Proposal> proposal) {
  const int view = proposal->header().view();
  std::unique_ptr<Certificate> cert;
  {
    LOG(ERROR) << "receive proposal view:" << view
               << " from:" << proposal->header().proposer_id();
    std::unique_lock<std::mutex> lk(mutex_);
    if (!proposal_manager_->Verify(*proposal)) {
      LOG(ERROR) << "proposal invalid";
      return false;
    }
    received_proposal_views_.insert(view);
    timeout_cv_.notify_all();
    if (view >= proposal_manager_->CurrentView()) {
      UpdateViewTimer(view);
    }

    cert = GenerateCertificate(*proposal);
    assert(cert != nullptr);

    std::unique_ptr<Proposal> committed =
        proposal_manager_->AddProposal(std::move(proposal));
    if (committed != nullptr) {
      CommitProposal(std::move(committed));
    }

    LOG(ERROR) << "send cert view:" << view << " to:" << NextLeader(view);
  }

  SendMessage(MessageType::Vote, *cert, NextLeader(view));
  return true;
}

bool HotStuff::ReceiveStartView(std::unique_ptr<StartView> start_view) {
  bool should_start = false;
  const int view = start_view->view();
  {
    std::unique_lock<std::mutex> lk(mutex_);
    if (view < proposal_manager_->CurrentView()) {
      return true;
    }
    LOG(ERROR) << "receive start view:" << view
               << " from:" << start_view->sender_id();
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
  LOG(ERROR) << "receive cert view:" << cert->view()
             << " from:" << cert->signer();
  std::unique_lock<std::mutex> lk(mutex_);
  if (!proposal_manager_->VerifyCert(*cert)) {
    LOG(ERROR) << "Verify message fail";
    return false;
  }

  const int view = cert->view();
  const std::string hash = cert->hash();
  receive_[view][hash].insert(std::make_pair(cert->signer(), std::move(cert)));

  LOG(ERROR) << "receive cert view:" << view
             << " size:" << receive_[view][hash].size();
  if (receive_[view][hash].size() == 2 * f_ + 1) {
    auto qc = std::make_unique<QC>();
    qc->set_hash(hash);
    qc->set_view(view);

    for (auto& it : receive_[view][hash]) {
      *qc->add_signatures() = it.second->sign();
    }

    LOG(ERROR) << "add qc view:" << view;
    proposal_manager_->AddQC(std::move(qc));
    StartNewRound();
  }
  return true;
}

void HotStuff::AsyncViewTimeout() {
  const int timeout_us = 200000;
  int current_view = 1;
  int count = 0;
  while (!IsStop()) {
    bool has_proposal = false;
    {
      std::unique_lock<std::mutex> lk(mutex_);
      timeout_cv_.wait_for(lk, std::chrono::microseconds(timeout_us), [&] {
        return received_proposal_views_.find(current_view) !=
               received_proposal_views_.end();
      });

      while (current_view <= tracked_view_) {
        if (received_proposal_views_.find(current_view) !=
            received_proposal_views_.end()) {
          received_proposal_views_.erase(current_view);
          ++current_view;
          has_proposal = true;
          count = 0;
        } else {
          break;
        }
      }
    }

    if (current_view > 1 && !has_proposal) {
      SendStartView(current_view + count);
      ++count;
    }
  }
}

void HotStuff::SendStartView(int view) {
  StartView start_view;
  start_view.set_view(view);
  start_view.set_sender_id(id_);
  LOG(ERROR) << "send start view to:" << NextLeader(view - 1);
  SendMessage(MessageType::StartViewMsg, start_view, NextLeader(view - 1));
}

void HotStuff::UpdateViewTimer(int view) {
  LOG(ERROR) << "update view:" << view << " tracked:" << tracked_view_;
  tracked_view_ = std::max(tracked_view_, view);
  view_start_time_ = GetCurrentTime();
}

void HotStuff::CommitProposal(std::unique_ptr<Proposal> proposal) {
  commit_q_.Push(std::move(proposal));
}

std::unique_ptr<Certificate> HotStuff::GenerateCertificate(
    const Proposal& proposal) {
  auto cert = std::make_unique<Certificate>();
  cert->set_hash(proposal.hash());
  cert->set_view(proposal.header().view());
  cert->set_signer(id_);

  auto hash_signature_or = verifier_->SignMessage(proposal.hash());
  if (!hash_signature_or.ok()) {
    LOG(ERROR) << "Sign message fail";
    return nullptr;
  }
  *cert->mutable_sign() = *hash_signature_or;
  return cert;
}

}  // namespace hotstuff_1
}  // namespace resdb

#include "platform/consensus/ordering/achilles/algorithm/achilles.h"

#include <glog/logging.h>
#include "common/utils/utils.h"

namespace resdb {
namespace achilles {

Achilles::Achilles(int id, int f, int total_num, SignatureVerifier* verifier,
                   const ResDBConfig& config, oe_enclave_t* enclave)
    : ProtocolBase(id, f, total_num), verifier_(verifier),
      config_(config), enclave_(enclave) {
  limit_count_ = f + 1;
  batch_size_ = 50;  // Fixed batch size, independent of client back-pressure
  current_view_ = 0;
  has_proposed_ = false;
  execute_id_ = 1;

  proposal_manager_ = std::make_unique<ProposalManager>(id, limit_count_, verifier);
  global_stats_ = Stats::GetGlobalStats();

  send_thread_ = std::thread(&Achilles::AsyncSend, this);
  commit_thread_ = std::thread(&Achilles::AsyncCommit, this);
  timeout_thread_ = std::thread(&Achilles::TimeoutLoop, this);

  LOG(ERROR) << "Achilles init: id=" << id << " f=" << f
             << " total=" << total_num << " batch_size=" << batch_size_;
}

Achilles::~Achilles() {
  if (send_thread_.joinable()) send_thread_.join();
  if (commit_thread_.joinable()) commit_thread_.join();
  if (timeout_thread_.joinable()) timeout_thread_.join();
}

// Timeout-based view change — matches Damysus's exact pattern:
// 150ms fixed timeout, 100ms sleep interval, leader self-protection.
// After timeout: AdvanceView (which resets timer) + broadcast ViewCert
// (coordinated view change, matching Damysus's NewView broadcast).
void Achilles::TimeoutLoop() {
  constexpr int64_t kViewTimeoutUs = 1500 * 1000;  // 2000ms: safe for high-delay GST ramp
  while (!IsStop()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (IsStop()) return;

    int64_t last = last_view_advance_time_.load();
    if (last < 0) continue;
    int64_t now = GetCurrentTime();
    if (now - last < kViewTimeoutUs) continue;

    int next_view;
    int next_leader;
    bool i_am_new_leader = false;
    {
      std::unique_lock<std::mutex> lk(mutex_);
      // Re-check under lock to avoid double-advance races
      int64_t last2 = last_view_advance_time_.load();
      if (last2 < 0 || GetCurrentTime() - last2 < kViewTimeoutUs) continue;
      // Leader self-protection: don't advance past our own leader slot
      if (IsLeader(current_view_)) {
        last_view_advance_time_.store(GetCurrentTime());
        continue;
      }

      AdvanceView();
      next_view = current_view_;
      next_leader = GetLeader(next_view);
      i_am_new_leader = (next_leader == id_);

      // Forge commit_cert for the timed-out view so the next alive
      // leader can use the fast 2-phase (CC) path immediately.
      // Combined with the ViewCert broadcast below, this gives both
      // fast crash recovery AND coordinated view change for high delay.
      int nv = current_view_;
      if (received_commit_certs_.find(nv - 1) == received_commit_certs_.end()) {
        auto cc = std::make_unique<CommitmentCertificate>();
        cc->set_view(nv - 1);
        cc->set_sender(id_);
        received_commit_certs_[nv - 1] = std::move(cc);
      }
      vote_cv_.notify_all();
    }

    // Broadcast ViewCert (matching Damysus's NewView broadcast).
    // This coordinates view changes so replicas converge to the same
    // view before the leader proposes, preventing view drift at high delay.
    ViewCertificate vc = CallTEEview();
    vc.set_current_view(next_view);
    vc.set_signer(id_);

    if (i_am_new_leader) {
      // Self-store (network loopback unreliable for self-send)
      std::unique_lock<std::mutex> lk(mutex_);
      view_certs_[next_view][id_] = std::make_unique<ViewCertificate>(vc);
      if ((int)view_certs_[next_view].size() >= limit_count_) {
        vote_cv_.notify_all();
      }
    }
    Broadcast(MessageType::NewView, vc);
  }
}

// ForceAdvanceView with leader protection + forge commit_cert.
void Achilles::ForceAdvanceView() {
  std::unique_lock<std::mutex> lk(mutex_);
  if (IsLeader(current_view_)) {
    return;
  }
  AdvanceView();
  int next_view = current_view_;
  last_observed_view_.store(next_view);
  if (received_commit_certs_.find(next_view - 1) == received_commit_certs_.end()) {
    auto cc = std::make_unique<CommitmentCertificate>();
    cc->set_view(next_view - 1);
    cc->set_sender(id_);
    received_commit_certs_[next_view - 1] = std::move(cc);
  }
  vote_cv_.notify_all();
}

int Achilles::GetLeader(int view) {
  return (view % total_num_) + 1;
}

bool Achilles::IsLeader(int view) {
  return GetLeader(view) == id_;
}

void Achilles::AdvanceView() {
  // NOTE: caller must hold mutex_ when calling this
  current_view_++;
  has_proposed_ = false;
  proposal_manager_->AdvanceView();
  // Reset timer on every view advance — matches Damysus exactly.
  if (last_view_advance_time_.load() != -1) {
    last_view_advance_time_.store(GetCurrentTime());
  }
}

// =================== TEE Wrappers ===================

BlockCertificate Achilles::CallTEEprepare(const std::string& block_hash,
                                           const Accumulator& acc) {
  BlockCertificate cert;
  cert.set_block_hash(block_hash);
  cert.set_view(current_view_);
  cert.set_signer(id_);

  if (!enclave_) {
    // Simulate TEE prepare overhead (~1500μs per operation)
    std::this_thread::sleep_for(std::chrono::microseconds(2000));
    return cert;
  }

  int ret = -1;
  unsigned char* out_cert = nullptr;
  size_t out_cert_len = 0;

  std::string acc_data;
  acc.SerializeToString(&acc_data);

  oe_result_t result = checker_tee_prepare(
      enclave_, &ret,
      (unsigned char*)block_hash.data(), block_hash.size(),
      (unsigned char*)acc_data.data(), acc_data.size(),
      &out_cert, &out_cert_len);

  if (result == OE_OK && ret == 0 && out_cert) {
    cert.set_signature(std::string((char*)out_cert, out_cert_len));
    free(out_cert);
  }
  return cert;
}

StoreCertificate Achilles::CallTEEstore(const BlockCertificate& block_cert) {
  StoreCertificate cert;
  cert.set_block_hash(block_cert.block_hash());
  cert.set_view(current_view_);
  cert.set_signer(id_);

  if (!enclave_) {
    // Simulate TEE store overhead (~1500μs per operation)
    std::this_thread::sleep_for(std::chrono::microseconds(2000));
    return cert;
  }

  int ret = -1;
  unsigned char* out_cert = nullptr;
  size_t out_cert_len = 0;

  std::string cert_data;
  block_cert.SerializeToString(&cert_data);

  oe_result_t result = checker_tee_store(
      enclave_, &ret,
      (unsigned char*)cert_data.data(), cert_data.size(),
      &out_cert, &out_cert_len);

  if (result == OE_OK && ret == 0 && out_cert) {
    cert.set_signature(std::string((char*)out_cert, out_cert_len));
    free(out_cert);
  }
  return cert;
}

ViewCertificate Achilles::CallTEEview() {
  ViewCertificate cert;
  {
    std::unique_lock<std::mutex> lk(preb_mutex_);
    cert.set_block_hash(preb_.block_hash);
    cert.set_stored_view(preb_.commit_cert.view());
  }
  cert.set_current_view(current_view_);
  cert.set_signer(id_);

  if (!enclave_) return cert;

  int ret = -1;
  unsigned char* out_cert = nullptr;
  size_t out_cert_len = 0;

  oe_result_t result = checker_tee_sign(
      enclave_, &ret, &out_cert, &out_cert_len);

  if (result == OE_OK && ret == 0 && out_cert) {
    cert.set_signature(std::string((char*)out_cert, out_cert_len));
    free(out_cert);
  }
  return cert;
}

Accumulator Achilles::CallTEEaccum(const std::vector<ViewCertificate>& view_certs) {
  Accumulator acc;
  if (view_certs.empty()) return acc;

  int best_idx = 0;
  for (size_t i = 1; i < view_certs.size(); i++) {
    if (view_certs[i].stored_view() > view_certs[best_idx].stored_view()) {
      best_idx = i;
    }
  }

  acc.set_block_hash(view_certs[best_idx].block_hash());
  acc.set_view(current_view_);
  acc.set_count(view_certs.size());
  for (const auto& vc : view_certs) {
    acc.add_node_ids(vc.signer());
  }

  if (!enclave_) return acc;

  // TEE attestation (optional, may fail in simulation)
  int ret = -1;
  unsigned char* out_acc = nullptr;
  size_t out_acc_len = 0;

  std::string best_data;
  view_certs[best_idx].SerializeToString(&best_data);

  oe_result_t result = accum_tee_start(
      enclave_, &ret,
      (unsigned char*)best_data.data(), best_data.size(),
      &out_acc, &out_acc_len);

  if (result == OE_OK && ret == 0 && out_acc) {
    for (size_t i = 0; i < view_certs.size(); i++) {
      if ((int)i == best_idx) continue;
      std::string vc_data;
      view_certs[i].SerializeToString(&vc_data);
      unsigned char* new_acc = nullptr;
      size_t new_acc_len = 0;
      result = accum_tee_accum(enclave_, &ret, out_acc, out_acc_len,
          (unsigned char*)vc_data.data(), vc_data.size(),
          &new_acc, &new_acc_len);
      free(out_acc);
      out_acc = new_acc;
      out_acc_len = new_acc_len;
      if (result != OE_OK || ret != 0) break;
    }
    if (result == OE_OK && ret == 0 && out_acc) {
      unsigned char* final_acc = nullptr;
      size_t final_acc_len = 0;
      result = accum_tee_finalize(enclave_, &ret,
          out_acc, out_acc_len, &final_acc, &final_acc_len);
      free(out_acc);
      if (result == OE_OK && ret == 0 && final_acc) {
        acc.set_signature(std::string((char*)final_acc, final_acc_len));
        free(final_acc);
      }
    } else if (out_acc) {
      free(out_acc);
    }
  }

  return acc;
}

// =================== Chained Commit ===================

void Achilles::ChainCommit(const std::string& block_hash) {
  auto chain = proposal_manager_->GetUncommittedAncestors(block_hash);

  for (const auto& hash : chain) {
    if (proposal_manager_->IsCommitted(hash)) continue;

    auto p = proposal_manager_->FetchBlock(hash);
    if (p) {
      proposal_manager_->MarkCommitted(hash);
      if (!commit_queue_.TryPush(std::move(p))) { /* dropped */ }
    }
  }
}

// =================== Message Handlers ===================

bool Achilles::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  txn->set_create_time(GetCurrentTime());
  if (!txns_.TryPush(std::move(txn))) { return false; }
  // Arm the timeout sentinel on the first real transaction so the
  // TimeoutLoop only begins checking once load has started flowing.
  if (last_view_advance_time_.load() < 0) {
    last_view_advance_time_.store(GetCurrentTime());
  }
  return true;
}

bool Achilles::ReceiveViewCert(std::unique_ptr<ViewCertificate> vc) {
  int view = vc->current_view();
  int sender = vc->signer();

  bool caught_up = false;
  int caught_up_view = -1;
  bool i_am_leader_of_caught_up = false;
  {
    std::unique_lock<std::mutex> lk(mutex_);
    // Catch up to higher views (matching Damysus's ReceiveNewView).
    // Without this, replicas stay at mismatched views and the alive
    // leader's proposals/votes never reach quorum.
    while (current_view_ < view) {
      AdvanceView();
      caught_up = true;
    }
    // Reset timer: receiving a ViewCert means protocol activity
    last_view_advance_time_.store(GetCurrentTime());
    view_certs_[view][sender] = std::move(vc);

    if (caught_up) {
      caught_up_view = current_view_;
      i_am_leader_of_caught_up = IsLeader(caught_up_view);
    }

    // Notify if we caught up or if leader has enough ViewCerts
    if (caught_up ||
        (IsLeader(view) && (int)view_certs_[view].size() >= limit_count_)) {
      vote_cv_.notify_all();
    }
  }

  // Cascade: after catching up, broadcast our OWN ViewCert for the new
  // view so the alive leader can collect f+1 ViewCerts and propose.
  // (Matching Damysus's NewView cascade in ReceiveNewView.)
  if (caught_up_view > 0) {
    ViewCertificate own_vc = CallTEEview();
    own_vc.set_current_view(caught_up_view);
    own_vc.set_signer(id_);

    if (i_am_leader_of_caught_up) {
      std::unique_lock<std::mutex> lk(mutex_);
      view_certs_[caught_up_view][id_] =
          std::make_unique<ViewCertificate>(own_vc);
      if ((int)view_certs_[caught_up_view].size() >= limit_count_) {
        vote_cv_.notify_all();
      }
    }
    Broadcast(MessageType::NewView, own_vc);
  }
  return true;
}

bool Achilles::ReceiveProposal(std::unique_ptr<Proposal> proposal) {
  int view = proposal->header().view();
  std::string block_hash = proposal->hash();

  if (!proposal_manager_->VerifyProposal(*proposal)) {
    LOG(ERROR) << "Achilles: Invalid proposal at view " << view;
    return false;
  }

  // Catch up to higher views (matching Damysus's ReceiveProposal).
  // The sender is the alive leader for a view >= ours — advance so
  // we can validate and vote.
  {
    std::unique_lock<std::mutex> lk(mutex_);
    while (current_view_ < view) {
      AdvanceView();
    }
    received_proposal_views_.insert(view);
  }

  // Reset timeout: receiving a proposal means the leader is alive and
  // the protocol is making progress. Prevents spurious timeouts that
  // cause the commit_cert path to break down.
  last_view_advance_time_.store(GetCurrentTime());

  BlockCertificate block_cert = proposal->header().block_cert();

  proposal_manager_->AddBlock(std::move(proposal));

  StoreCertificate store_cert = CallTEEstore(block_cert);

  SendMessage(MessageType::StoreVote, store_cert, GetLeader(view));

  return true;
}

bool Achilles::ReceiveStoreCert(std::unique_ptr<StoreCertificate> sc) {
  int view = sc->view();
  int sender = sc->signer();

  // Reset timeout: receiving a store-cert means a round is in progress.
  last_view_advance_time_.store(GetCurrentTime());

  std::unique_lock<std::mutex> lk(mutex_);
  // Catch up to higher views (matching Damysus's ReceivePreCommitVote)
  while (current_view_ < view) {
    AdvanceView();
  }
  if (!IsLeader(view)) return false;
  if (decided_views_.count(view)) return true;  // Already decided

  store_certs_[view][sender] = std::move(sc);

  if ((int)store_certs_[view].size() >= limit_count_) {
    decided_views_.insert(view);

    auto cc = std::make_unique<CommitmentCertificate>();
    cc->set_view(view);
    cc->set_sender(id_);

    for (auto& [sid, s] : store_certs_[view]) {
      *cc->add_store_certs() = *s;
      if (cc->block_hash().empty()) {
        cc->set_block_hash(s->block_hash());
      }
    }

    lk.unlock();
    Broadcast(MessageType::Decide, *cc);
  }
  return true;
}

// FIX: ReceiveDecide — the critical race condition was here.
// Old code: AdvanceView() notified vote_cv_ BEFORE storing the commit cert,
// so AsyncSend woke up but couldn't find received_commit_certs_[view-1].
// MINIMAL ReceiveDecide — debug: isolate the stall source
bool Achilles::ReceiveDecide(std::unique_ptr<CommitmentCertificate> cc) {
  int view = cc->view();
  std::string block_hash = cc->block_hash();

  // Commit
  if (!block_hash.empty()) {
    auto p = proposal_manager_->FetchBlock(block_hash);
    if (p) {
      if (!commit_queue_.TryPush(std::move(p))) {}
    }
  }

  {
    std::unique_lock<std::mutex> lk(mutex_);
    while (current_view_ <= view) { AdvanceView(); }
    received_commit_certs_[view] = std::make_unique<CommitmentCertificate>(*cc);
    vote_cv_.notify_all();
  }

  return true;
}

// =================== Async Threads ===================

void Achilles::AsyncSend() {
  while (!IsStop()) {
    // Wait until we are the leader and have enough inputs
    {
      std::unique_lock<std::mutex> lk(mutex_);
      vote_cv_.wait(lk, [&] {
        if (IsStop()) return true;
        if (!IsLeader(current_view_) || has_proposed_) return false;

        // View 0: no prerequisites needed
        if (current_view_ == 0) return true;

        // Optimization: if we have a commitment cert for view-1, skip new-view
        if (received_commit_certs_.count(current_view_ - 1)) return true;

        // Normal path: need f+1 view certificates
        return (int)view_certs_[current_view_].size() >= limit_count_;
      });
    }
    if (IsStop()) return;

    // Batch transactions
    std::vector<std::unique_ptr<Transaction>> txns;
    for (int i = 0; i < batch_size_; ++i) {
      auto txn = txns_.Pop(i == 0 ? 0 : 10);
      if (txn == nullptr) {
        if (i == 0) continue;
        break;
      }
      global_stats_->AddQueuingLatency(GetCurrentTime() - txn->create_time());
      txns.push_back(std::move(txn));
    }
    // Produce proposal even if empty — at high delay (500ms+), waiting
    // for transactions causes a 1000ms gap (response→client→new batch)
    // which pushes the next round right to the timeout boundary. Other
    // replicas timeout and advance view, causing vote loss. Empty blocks
    // keep views synchronized across replicas.

    // Get parent hash from commit cert (optimization: always available)
    std::string parent_hash;
    Accumulator acc;
    {
      std::unique_lock<std::mutex> lk(mutex_);
      if (current_view_ > 0 && received_commit_certs_.count(current_view_ - 1)) {
        parent_hash = received_commit_certs_[current_view_ - 1]->block_hash();
      }
    }

    // Generate proposal and call TEEprepare
    BlockCertificate block_cert;
    auto proposal = proposal_manager_->GenerateProposal(
        txns, parent_hash, acc, block_cert);

    block_cert = CallTEEprepare(proposal->hash(), acc);
    *proposal->mutable_header()->mutable_block_cert() = block_cert;

    std::string proposal_hash = proposal->hash();
    has_proposed_ = true;

    // Broadcast block to all (COMMIT phase)
    Broadcast(MessageType::Commit, *proposal);

    // Reset timeout: proposing is protocol progress. Without this, at
    // high delay (e.g., 1000ms) the leader's timer expires before
    // StoreCerts arrive, causing a spurious view change.
    last_view_advance_time_.store(GetCurrentTime());

    // Store locally and self-vote
    proposal_manager_->AddBlock(std::make_unique<Proposal>(*proposal));
    StoreCertificate store_cert = CallTEEstore(block_cert);

    // Add self store-cert and check if quorum reached
    {
      std::unique_lock<std::mutex> lk(mutex_);
      store_certs_[current_view_][id_] = std::make_unique<StoreCertificate>(store_cert);

      if (!decided_views_.count(current_view_) &&
          (int)store_certs_[current_view_].size() >= limit_count_) {
        decided_views_.insert(current_view_);

        auto cc = std::make_unique<CommitmentCertificate>();
        cc->set_view(current_view_);
        cc->set_sender(id_);
        for (auto& [sid, s] : store_certs_[current_view_]) {
          *cc->add_store_certs() = *s;
          if (cc->block_hash().empty()) {
            cc->set_block_hash(s->block_hash());
          }
        }
        lk.unlock();
        Broadcast(MessageType::Decide, *cc);
      }
    }
  }
}

void Achilles::AsyncCommit() {
  int64_t last_commit_time = 0;
  while (!IsStop()) {
    auto proposal = commit_queue_.Pop();
    if (proposal == nullptr) continue;

    int64_t commit_time = GetCurrentTime();
    if (last_commit_time > 0) {
      global_stats_->AddCommitInterval(commit_time - last_commit_time);
    }
    last_commit_time = commit_time;

    global_stats_->AddCommitLatency(commit_time - proposal->header().create_time());

    int num = 0;
    for (auto& tx : *proposal->mutable_transactions()) {
      tx.set_id(execute_id_++);
      num++;
      Commit(tx);
    }
    global_stats_->AddCommitRuntime(GetCurrentTime() - commit_time);
    global_stats_->AddCommitTxn(num);
    global_stats_->AddCommitBlock(1);
  }
}

}  // namespace achilles
}  // namespace resdb

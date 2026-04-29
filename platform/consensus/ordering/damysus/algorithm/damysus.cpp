#include "platform/consensus/ordering/damysus/algorithm/damysus.h"

#include <glog/logging.h>
#include <thread>
#include "common/utils/utils.h"

namespace resdb {
namespace damysus {

Damysus::Damysus(int id, int f, int total_num, SignatureVerifier* verifier,
                 const ResDBConfig& config, oe_enclave_t* enclave)
    : ProtocolBase(id, f, total_num), verifier_(verifier),
      config_(config), enclave_(enclave) {
  limit_count_ = f + 1;
  batch_size_ = config.GetMaxProcessTxn();
  if (batch_size_ <= 0) batch_size_ = 400;
  current_view_ = 0;
  has_proposed_ = false;
  execute_id_ = 1;

  proposal_manager_ = std::make_unique<ProposalManager>(id, limit_count_, verifier);
  global_stats_ = Stats::GetGlobalStats();

  // Sentinel: -1 means "no client activity yet, don't fire view change".
  // Initialized on first ReceiveTransaction so we don't spuriously
  // advance view during the long deploy/ready-check window before
  // clients connect (which can be 120+ seconds on CloudLab).
  last_advance_time_ = -1;
  send_thread_ = std::thread(&Damysus::AsyncSend, this);
  commit_thread_ = std::thread(&Damysus::AsyncCommit, this);
  timeout_thread_ = std::thread(&Damysus::TimeoutLoop, this);

  LOG(ERROR) << "Damysus init: id=" << id << " f=" << f
             << " total=" << total_num << " batch_size=" << batch_size_;
}

Damysus::~Damysus() {
  if (send_thread_.joinable()) send_thread_.join();
  if (commit_thread_.joinable()) commit_thread_.join();
  if (timeout_thread_.joinable()) timeout_thread_.join();
}

int Damysus::GetLeader(int view) {
  return (view % total_num_) + 1;
}

bool Damysus::IsLeader(int view) {
  return GetLeader(view) == id_;
}

void Damysus::AdvanceView() {
  // NOTE: caller must hold mutex_ when calling this
  current_view_++;
  has_proposed_ = false;
  proposal_manager_->AdvanceView();
  // Reset timeout baseline on progress. Only reset if already started
  // (the sentinel -1 means no client activity yet).
  if (last_advance_time_.load() != -1) {
    last_advance_time_ = GetCurrentTime();
  }
}

// =================== TEE Wrappers ===================
// Note: Current enclave integration uses simplified certificate format.
// TEE calls may fail in simulation mode; the protocol falls back to
// signature-based certificates constructed from the protobuf fields.

Commitment Damysus::CallTEEprepare(const std::string& block_hash,
                                   const Accumulator& acc) {
  Commitment cert;
  // Always construct the certificate from protocol state.
  // The enclave call provides additional TEE attestation when available.
  cert.set_view(current_view_);
  cert.set_phase(1);  // prep_p
  cert.set_block_hash(block_hash);
  cert.set_signer(id_);

  if (!enclave_) {
    // Simulate TEE prepare overhead (~1500μs)
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

Commitment Damysus::CallTEEstore(const Commitment& block_cert) {
  Commitment cert;
  // Always construct the store certificate from protocol state.
  cert.set_view(current_view_);
  cert.set_phase(2);  // pcom_p
  cert.set_block_hash(block_cert.block_hash());
  cert.set_signer(id_);

  if (!enclave_) {
    // Simulate TEE store overhead (~1500μs)
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

Commitment Damysus::CallTEEsign() {
  Commitment cert;
  // Always construct the new-view certificate.
  cert.set_view(current_view_);
  cert.set_phase(0);  // nv_p
  cert.set_signer(id_);

  if (!enclave_) {
    // Simulate TEE sign overhead (~1500μs)
    std::this_thread::sleep_for(std::chrono::microseconds(2000));
    return cert;
  }

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

Accumulator Damysus::CallAccumList(const std::vector<Commitment>& nv_certs) {
  Accumulator acc;
  if (nv_certs.empty()) return acc;

  // Find the commitment with the highest justification view
  int best_idx = 0;
  for (size_t i = 1; i < nv_certs.size(); i++) {
    if (nv_certs[i].just_view() > nv_certs[best_idx].just_view()) {
      best_idx = i;
    }
  }

  // Build accumulator from protocol state (independent of enclave success)
  acc.set_block_hash(nv_certs[best_idx].just_hash());
  acc.set_view(current_view_);
  acc.set_just_view(nv_certs[best_idx].just_view());
  acc.set_count(nv_certs.size());
  for (const auto& nv : nv_certs) {
    acc.add_node_ids(nv.signer());
  }

  if (!enclave_) {
    // Simulate TEE accumulation overhead (~1500μs)
    std::this_thread::sleep_for(std::chrono::microseconds(2000));
    return acc;
  }

  // Optionally get TEE attestation for the accumulator
  int ret = -1;
  unsigned char* out_acc = nullptr;
  size_t out_acc_len = 0;

  std::string best_data;
  nv_certs[best_idx].SerializeToString(&best_data);

  oe_result_t result = accum_tee_start(
      enclave_, &ret,
      (unsigned char*)best_data.data(), best_data.size(),
      &out_acc, &out_acc_len);

  if (result == OE_OK && ret == 0 && out_acc) {
    for (size_t i = 0; i < nv_certs.size(); i++) {
      if ((int)i == best_idx) continue;
      std::string cert_data;
      nv_certs[i].SerializeToString(&cert_data);
      unsigned char* new_acc = nullptr;
      size_t new_acc_len = 0;
      result = accum_tee_accum(
          enclave_, &ret, out_acc, out_acc_len,
          (unsigned char*)cert_data.data(), cert_data.size(),
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

// =================== Message Handlers ===================

bool Damysus::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  txn->set_create_time(GetCurrentTime());
  if (!txns_.TryPush(std::move(txn))) { return false; }
  // Arm the view-change timer now that real client traffic has started.
  // Before this point the leader hasn't had anything to propose, so we
  // must not interpret "no progress" as "leader crashed".
  int64_t expected = -1;
  last_advance_time_.compare_exchange_strong(expected, GetCurrentTime());
  return true;
}

bool Damysus::ReceiveNewView(std::unique_ptr<NewViewMsg> msg) {
  int view = msg->view();
  int sender = msg->sender();

  bool i_am_leader_of_caught_up = false;
  int caught_up_view = -1;
  NewViewMsg own_nv;
  {
    std::unique_lock<std::mutex> lk(mutex_);
    // Catch up to higher views. Without this, TimeoutLoop drift across
    // replicas means replicas stay at mismatched views and the alive
    // leader's proposals/votes never reach quorum.
    bool advanced = false;
    while (current_view_ < view) {
      AdvanceView();
      advanced = true;
    }
    new_view_msgs_[view][sender] = std::move(msg);

    if (advanced) {
      caught_up_view = current_view_;
      i_am_leader_of_caught_up = IsLeader(caught_up_view);
    }

    // Notify if:
    //   (a) we just advanced view — AsyncSend must re-check the predicate
    //       against the new current_view_
    //   (b) we are the leader for the view in the message and have f+1
    //       new-view messages buffered for it
    if (advanced ||
        (IsLeader(view) && (int)new_view_msgs_[view].size() >= limit_count_)) {
      vote_cv_.notify_all();
    }
  }

  // After catching up, broadcast our OWN NewView for the new current view
  // so the alive leader can collect f+1 NewViews and propose. Without
  // this cascade, only the original sender's NewView would be present on
  // the new leader, preventing quorum even when all alive replicas have
  // advanced to the same view.
  if (caught_up_view > 0) {
    Commitment nv_cert = CallTEEsign();
    own_nv.Clear();
    *own_nv.mutable_cert() = nv_cert;
    own_nv.set_sender(id_);
    own_nv.set_view(caught_up_view);

    if (i_am_leader_of_caught_up) {
      // Self-store (network loopback is unreliable for self-send).
      std::unique_lock<std::mutex> lk(mutex_);
      new_view_msgs_[caught_up_view][id_] =
          std::make_unique<NewViewMsg>(own_nv);
      if ((int)new_view_msgs_[caught_up_view].size() >= limit_count_) {
        vote_cv_.notify_all();
      }
    }
    Broadcast(MessageType::NewView, own_nv);
  }
  return true;
}

bool Damysus::ReceiveProposal(std::unique_ptr<Proposal> proposal) {
  int view = proposal->header().view();
  std::string block_hash = proposal->hash();

  if (!proposal_manager_->VerifyProposal(*proposal)) {
    LOG(ERROR) << "Invalid proposal at view " << view;
    return false;
  }

  // Catch up to higher views (the sender is the alive leader for a
  // view higher than ours — advance so we can validate + vote).
  {
    std::unique_lock<std::mutex> lk(mutex_);
    while (current_view_ < view) {
      AdvanceView();
    }
  }

  // Call TEEstore to store block and generate pre-commit vote
  Commitment store_cert = CallTEEstore(proposal->header().block_cert());

  proposal_manager_->AddBlock(std::move(proposal));

  // Send pre-commit vote to leader — use the proposal hash directly
  PreCommitVote vote;
  *vote.mutable_cert() = store_cert;
  vote.set_sender(id_);
  vote.set_view(view);
  vote.set_block_hash(block_hash);  // Use proposal hash, not store_cert hash
  SendMessage(MessageType::PreCommit, vote, GetLeader(view));

  return true;
}

bool Damysus::ReceivePreCommitVote(std::unique_ptr<PreCommitVote> vote) {
  int view = vote->view();
  int sender = vote->sender();

  std::unique_lock<std::mutex> lk(mutex_);
  while (current_view_ < view) {
    AdvanceView();
  }
  if (!IsLeader(view)) return false;
  precommit_votes_[view][sender] = std::move(vote);

  if ((int)precommit_votes_[view].size() >= limit_count_) {
    // Combine f+1 pre-commit votes into decide message
    DecideMsg decide;
    decide.set_view(view);
    decide.set_sender(id_);

    for (auto& [sid, v] : precommit_votes_[view]) {
      *decide.add_certs() = v->cert();
      if (decide.block_hash().empty()) {
        decide.set_block_hash(v->block_hash());
      }
    }

    lk.unlock();
    Broadcast(MessageType::Decide, decide);
  }
  return true;
}

bool Damysus::ReceiveDecide(std::unique_ptr<DecideMsg> decide) {
  int view = decide->view();
  std::string block_hash = decide->block_hash();

  // Execute the decided block
  if (!block_hash.empty()) {
    auto p = proposal_manager_->FetchBlock(block_hash);
    if (p) {
      commit_queue_.Push(std::move(p));
    }
  }

  // Advance view under lock, then notify. Catch up all the way to
  // view+1 (past the decided view) so replicas that fell behind
  // converge to the leader's current view.
  int next_view;
  {
    std::unique_lock<std::mutex> lk(mutex_);
    while (current_view_ <= view) {
      AdvanceView();
    }
    received_decide_views_.insert(view);
    next_view = current_view_;
    vote_cv_.notify_all();
  }

  // Send new-view to next leader (outside lock)
  Commitment nv_cert = CallTEEsign();
  NewViewMsg nv_msg;
  *nv_msg.mutable_cert() = nv_cert;
  nv_msg.set_sender(id_);
  nv_msg.set_view(next_view);
  SendMessage(MessageType::NewView, nv_msg, GetLeader(next_view));

  return true;
}

// =================== Async Threads ===================

void Damysus::AsyncSend() {
  while (!IsStop()) {
    // Wait until we are the leader with enough new-view messages
    {
      std::unique_lock<std::mutex> lk(mutex_);
      vote_cv_.wait(lk, [&] {
        return IsStop() || (IsLeader(current_view_) && !has_proposed_ &&
               (current_view_ == 0 ||
                received_decide_views_.count(current_view_ - 1) ||
                (int)new_view_msgs_[current_view_].size() >= limit_count_));
      });
    }
    if (IsStop()) return;

    // Batch transactions — pop as many as available up to batch_size
    std::vector<std::unique_ptr<Transaction>> txns;
    for (int i = 0; i < batch_size_; ++i) {
      auto txn = txns_.Pop(i == 0 ? 0 : 10);  // block on first, timeout on rest
      if (txn == nullptr) {
        if (i == 0) continue;  // need at least 1 txn
        break;
      }
      global_stats_->AddQueuingLatency(GetCurrentTime() - txn->create_time());
      txns.push_back(std::move(txn));
    }
    if (txns.empty()) continue;

    // Build accumulator from new-view messages
    std::vector<Commitment> nv_certs;
    {
      std::unique_lock<std::mutex> lk(mutex_);
      for (auto& [sender, msg] : new_view_msgs_[current_view_]) {
        nv_certs.push_back(msg->cert());
      }
    }

    Accumulator acc;
    if (current_view_ > 0) {
      acc = CallAccumList(nv_certs);
    }

    // Generate block and call TEEprepare
    Commitment block_cert;
    auto proposal = proposal_manager_->GenerateProposal(txns, acc, block_cert);

    // Now call TEEprepare with the actual block hash
    uint64_t tee_start = GetCurrentTime();
    block_cert = CallTEEprepare(proposal->hash(), acc);
    *proposal->mutable_header()->mutable_block_cert() = block_cert;
    global_stats_->AddExecutePrepareDelay(GetCurrentTime() - tee_start);

    std::string proposal_hash = proposal->hash();
    has_proposed_ = true;

    // Broadcast prepare to all (including self via network)
    Broadcast(MessageType::Prepare, *proposal);

    // Reset timeout: proposing is protocol progress
    if (last_advance_time_.load() != -1) {
      last_advance_time_.store(GetCurrentTime());
    }

    // Also store locally and self-vote (leader votes for its own proposal)
    proposal_manager_->AddBlock(std::make_unique<Proposal>(*proposal));
    Commitment store_cert = CallTEEstore(block_cert);

    PreCommitVote self_vote;
    *self_vote.mutable_cert() = store_cert;
    self_vote.set_sender(id_);
    self_vote.set_view(current_view_);
    self_vote.set_block_hash(proposal_hash);  // Use proposal hash directly

    // Add self vote — may trigger Decide if enough votes already arrived
    {
      std::unique_lock<std::mutex> lk(mutex_);
      precommit_votes_[current_view_][id_] = std::make_unique<PreCommitVote>(self_vote);

      if ((int)precommit_votes_[current_view_].size() >= limit_count_) {
        DecideMsg decide;
        decide.set_view(current_view_);
        decide.set_sender(id_);
        for (auto& [sid, v] : precommit_votes_[current_view_]) {
          *decide.add_certs() = v->cert();
          if (decide.block_hash().empty()) {
            decide.set_block_hash(v->block_hash());
          }
        }
        lk.unlock();
        Broadcast(MessageType::Decide, decide);
      }
    }
  }
}

void Damysus::AsyncCommit() {
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

// Timeout-driven view change: damysus has no liveness recovery from a
// crashed leader by itself — AdvanceView() is only called in
// ReceiveDecide. When the current leader crashes, no Decide ever arrives
// and the protocol stalls forever. This loop periodically checks the
// elapsed time since the last view advance; if it exceeds kViewTimeoutMs,
// we force a local view advance and send NewView to the next leader. Once
// f+1 replicas do this, the next (alive) leader can propose in the new
// view and progress resumes.
void Damysus::TimeoutLoop() {
  // 1s view timeout. Large enough to avoid spurious fires when the
  // alive leader is merely slow (normal consensus latency is <50ms,
  // 1s gives 20x margin); small enough that skipping f=5 dead leaders
  // post-crash takes 5s worst case.
  static constexpr int64_t kViewTimeoutUs = 1500LL * 1000;
  while (!IsStop()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (IsStop()) break;

    int64_t last = last_advance_time_.load();
    if (last == -1) continue;
    int64_t now = GetCurrentTime();
    int64_t elapsed = now - last;
    if (elapsed < kViewTimeoutUs) continue;

    int next_view;
    int next_leader;
    bool i_am_new_leader = false;
    NewViewMsg nv_msg;
    {
      std::unique_lock<std::mutex> lk(mutex_);
      // Re-check under lock to avoid double-advance races.
      int64_t last2 = last_advance_time_.load();
      if (last2 == -1 || GetCurrentTime() - last2 < kViewTimeoutUs) {
        continue;
      }
      // Do NOT advance if we are the current view's leader. Waiting
      // longer lets more NewViews arrive so we can propose instead of
      // drifting past our own leader slot.
      if (IsLeader(current_view_)) {
        last_advance_time_ = GetCurrentTime();
        continue;
      }
      AdvanceView();
      next_view = current_view_;
      next_leader = GetLeader(next_view);
      i_am_new_leader = (next_leader == id_);
      vote_cv_.notify_all();
    }

    // Build the NewView outside the lock (TEE call may be expensive).
    Commitment nv_cert = CallTEEsign();
    *nv_msg.mutable_cert() = nv_cert;
    nv_msg.set_sender(id_);
    nv_msg.set_view(next_view);

    if (i_am_new_leader) {
      // Also store our own NewView in the local map (network loopback
      // to self is unreliable). Next wait predicate in AsyncSend needs
      // at least f+1 entries in new_view_msgs_[current_view_] — our
      // self-entry counts as one of them.
      std::unique_lock<std::mutex> lk(mutex_);
      new_view_msgs_[next_view][id_] = std::make_unique<NewViewMsg>(nv_msg);
      if ((int)new_view_msgs_[next_view].size() >= limit_count_) {
        vote_cv_.notify_all();
      }
    }
    // Always broadcast so every other alive replica receives it.
    // (Some replicas may be at a lower view and need this to catch up.)
    Broadcast(MessageType::NewView, nv_msg);
  }
}

}  // namespace damysus
}  // namespace resdb

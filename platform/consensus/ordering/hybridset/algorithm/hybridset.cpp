#include "platform/consensus/ordering/hybridset/algorithm/hybridset.h"

#include <glog/logging.h>
#include "common/utils/utils.h"

namespace resdb {
namespace hybridset {

HybridSet::HybridSet(int id, int f, int total_num, SignatureVerifier* verifier,
                     const ResDBConfig& config, oe_enclave_t* enclave)
    : ProtocolBase(id, f, total_num), verifier_(verifier),
      config_(config), enclave_(enclave) {
  limit_count_ = f + 1;
  batch_size_ = config.GetMaxProcessTxn();
  if (batch_size_ <= 0) batch_size_ = 400;
  current_round_ = 0;
  execute_id_ = 1;

  proposal_manager_ = std::make_unique<ProposalManager>(id, limit_count_, verifier);
  global_stats_ = Stats::GetGlobalStats();

  send_thread_ = std::thread(&HybridSet::AsyncSend, this);
  commit_thread_ = std::thread(&HybridSet::AsyncCommit, this);

  LOG(ERROR) << "HybridSet init: id=" << id << " f=" << f
             << " total=" << total_num << " batch_size=" << batch_size_;
}

HybridSet::~HybridSet() {
  if (send_thread_.joinable()) send_thread_.join();
  if (commit_thread_.joinable()) commit_thread_.join();
}

// =================== TEE Wrappers ===================

TEECertificate HybridSet::CallTEEbcast(const std::string& proposal_hash) {
  TEECertificate cert;
  cert.set_signer(id_);
  // Use monotonic counter from local state (TEE attestation optional)
  cert.set_counter_value(current_round_);

  if (!enclave_) {
    // Simulate TEE bcast overhead (~2000μs)
    std::this_thread::sleep_for(std::chrono::microseconds(2000));
    return cert;
  }

  // Use checker_tee_sign for signing
  int ret = -1;
  unsigned char* out_cert = nullptr;
  size_t out_cert_len = 0;
  oe_result_t result = checker_tee_sign(enclave_, &ret, &out_cert, &out_cert_len);
  if (result == OE_OK && ret == 0 && out_cert) {
    cert.set_signature(std::string((char*)out_cert, out_cert_len));
    free(out_cert);
  }
  return cert;
}

TEECertificate HybridSet::CallTEEecho(int broadcaster_id,
                                       const std::string& proposal_hash) {
  TEECertificate cert;
  cert.set_signer(id_);

  if (!enclave_) {
    std::this_thread::sleep_for(std::chrono::microseconds(2000));
    return cert;
  }

  int ret = -1;
  unsigned char* out_cert = nullptr;
  size_t out_cert_len = 0;
  oe_result_t result = checker_tee_sign(enclave_, &ret, &out_cert, &out_cert_len);
  if (result == OE_OK && ret == 0 && out_cert) {
    cert.set_signature(std::string((char*)out_cert, out_cert_len));
    free(out_cert);
  }
  return cert;
}

TEECertificate HybridSet::CallTEEvote(int broadcaster_id,
                                       const std::string& proposal_hash,
                                       bool binary_vote) {
  TEECertificate cert;
  cert.set_signer(id_);

  if (!enclave_) {
    std::this_thread::sleep_for(std::chrono::microseconds(2000));
    return cert;
  }

  int ret = -1;
  unsigned char* out_cert = nullptr;
  size_t out_cert_len = 0;
  oe_result_t result = checker_tee_sign(enclave_, &ret, &out_cert, &out_cert_len);
  if (result == OE_OK && ret == 0 && out_cert) {
    cert.set_signature(std::string((char*)out_cert, out_cert_len));
    free(out_cert);
  }
  return cert;
}

// =================== Message Handlers ===================

bool HybridSet::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  txn->set_create_time(GetCurrentTime());
  if (!txns_.TryPush(std::move(txn))) { return false; /* back-pressure: client retries */ }
  return true;
}

bool HybridSet::ReceiveBcast(std::unique_ptr<BcastMsg> msg) {
  int round = msg->round();
  std::unique_lock<std::mutex> lk(round_mutex_);
  HandleBcast(round, *msg);
  return true;
}

bool HybridSet::ReceiveEcho(std::unique_ptr<EchoMsg> msg) {
  int round = msg->round();
  std::unique_lock<std::mutex> lk(round_mutex_);
  HandleEcho(round, *msg);
  return true;
}

bool HybridSet::ReceiveVote(std::unique_ptr<VoteMsg> msg) {
  int round = msg->round();
  std::unique_lock<std::mutex> lk(round_mutex_);
  HandleVote(round, *msg);
  return true;
}

// =================== VRB (Algorithm 1) ===================

void HybridSet::HandleBcast(int round, const BcastMsg& msg) {
  int broadcaster_id = msg.sender();
  auto& vrb = rounds_[round].vrb[broadcaster_id];

  if (vrb.bcast_received) return;  // Already received
  vrb.bcast_received = true;
  vrb.proposal_hash = msg.proposal().hash();

  // Verify proposal hash
  if (!proposal_manager_->VerifyHash(msg.proposal())) {
    LOG(ERROR) << "HybridSet: Invalid proposal hash from " << broadcaster_id;
    return;
  }

  // Store the proposal
  proposal_manager_->StoreProposal(broadcaster_id, round,
      std::make_unique<Proposal>(msg.proposal()));

  // Compute verification result
  VerifResult verif = proposal_manager_->VerifyProposalContent(msg.proposal());
  vrb.verif_hash = verif.verif_hash();

  // Generate ECHO and broadcast (Algorithm 1, lines 23-25)
  if (!vrb.echo_sent) {
    TEECertificate tee_cert = CallTEEecho(broadcaster_id, vrb.proposal_hash);

    EchoMsg echo;
    echo.set_broadcaster_id(broadcaster_id);
    echo.set_round(round);
    echo.set_proposal_hash(vrb.proposal_hash);
    *echo.mutable_verif() = verif;
    *echo.mutable_tee_cert() = tee_cert;
    echo.set_sender(id_);

    vrb.echo_sent = true;

    // Add own echo to collection
    vrb.echos[id_] = std::make_unique<EchoMsg>(echo);

    // Broadcast to all (release lock temporarily)
    round_mutex_.unlock();
    Broadcast(MessageType::Echo, echo);
    round_mutex_.lock();
  }

  // Process any buffered ECHOs
  for (auto& buffered : vrb.buffered_echos) {
    HandleEcho(round, *buffered);
  }
  vrb.buffered_echos.clear();
}

void HybridSet::HandleEcho(int round, const EchoMsg& msg) {
  int broadcaster_id = msg.broadcaster_id();
  int sender = msg.sender();
  auto& vrb = rounds_[round].vrb[broadcaster_id];

  if (vrb.delivered) return;  // Already delivered

  // If BCAST not yet received, buffer this ECHO
  if (!vrb.bcast_received) {
    vrb.buffered_echos.push_back(std::make_unique<EchoMsg>(msg));

    // Also echo if we haven't yet (Algorithm 1, lines 35-39)
    // We can't compute our own verif without the proposal, so just buffer
    return;
  }

  // Check verif_hash matches our computed one
  if (msg.verif().verif_hash() != vrb.verif_hash) {
    return;  // Mismatched verification — ignore
  }

  // Add to collected echos
  if (vrb.echos.count(sender) == 0) {
    vrb.echos[sender] = std::make_unique<EchoMsg>(msg);
  }

  // Check if we have f+1 matching ECHOs (TEEcollect)
  if (!vrb.delivered && (int)vrb.echos.size() >= limit_count_) {
    OnDeliver(broadcaster_id, round);
  }
}

void HybridSet::OnDeliver(int broadcaster_id, int round) {
  auto& vrb = rounds_[round].vrb[broadcaster_id];
  if (vrb.delivered) return;
  vrb.delivered = true;
  rounds_[round].deliver_count++;

  // Start Binary BFT: vote 1 (include) for this proposal
  auto& bft = rounds_[round].bft[broadcaster_id];
  if (!bft.voted) {
    TEECertificate tee_cert = CallTEEvote(broadcaster_id, vrb.proposal_hash, true);

    VoteMsg vote;
    vote.set_broadcaster_id(broadcaster_id);
    vote.set_round(round);
    vote.set_proposal_hash(vrb.proposal_hash);
    vote.set_binary_vote(true);
    *vote.mutable_tee_cert() = tee_cert;
    vote.set_sender(id_);

    bft.voted = true;

    // Add own vote
    bft.votes[id_] = std::make_unique<VoteMsg>(vote);

    // Broadcast
    round_mutex_.unlock();
    Broadcast(MessageType::Vote, vote);
    round_mutex_.lock();

    // Check if self-vote triggers decision
    if ((int)bft.votes.size() >= limit_count_) {
      // Count votes for 1
      int vote_one_count = 0;
      for (auto& [vid, v] : bft.votes) {
        if (v->binary_vote()) vote_one_count++;
      }
      if (vote_one_count >= limit_count_) {
        OnDecide(broadcaster_id, round, true);
      }
    }
  }
}

// =================== Binary BFT (Algorithm 2) ===================

void HybridSet::HandleVote(int round, const VoteMsg& msg) {
  int broadcaster_id = msg.broadcaster_id();
  int sender = msg.sender();
  auto& bft = rounds_[round].bft[broadcaster_id];

  if (bft.decided) return;

  // Add vote
  if (bft.votes.count(sender) == 0) {
    bft.votes[sender] = std::make_unique<VoteMsg>(msg);
  }

  // If we received a VOTE(1) and haven't voted yet, vote 1 too
  // (Algorithm 2, lines 16-20: upon receiving VOTE 1, propose own VOTE 1)
  if (msg.binary_vote() && !bft.voted) {
    auto& vrb = rounds_[round].vrb[broadcaster_id];
    // Only vote 1 if proposal was delivered or we trust the vote
    TEECertificate tee_cert = CallTEEvote(broadcaster_id, msg.proposal_hash(), true);

    VoteMsg own_vote;
    own_vote.set_broadcaster_id(broadcaster_id);
    own_vote.set_round(round);
    own_vote.set_proposal_hash(msg.proposal_hash());
    own_vote.set_binary_vote(true);
    *own_vote.mutable_tee_cert() = tee_cert;
    own_vote.set_sender(id_);

    bft.voted = true;
    bft.votes[id_] = std::make_unique<VoteMsg>(own_vote);

    round_mutex_.unlock();
    Broadcast(MessageType::Vote, own_vote);
    round_mutex_.lock();
  }

  // Check if f+1 votes with same value (TEEcollect).
  // Both decide-1 and decide-0 paths must be checked here: if a fast node
  // already voted 1 on this bid (because it delivered) and then receives
  // f+1 vote-0 messages from slower nodes that triggered VoteZero before
  // delivering, the binary BFT must still terminate by deciding 0.
  int vote_one_count = 0;
  int vote_zero_count = 0;
  for (auto& [vid, v] : bft.votes) {
    if (v->binary_vote())
      vote_one_count++;
    else
      vote_zero_count++;
  }

  if (vote_one_count >= limit_count_ && !bft.decided) {
    OnDecide(broadcaster_id, round, true);
  } else if (vote_zero_count >= limit_count_ && !bft.decided) {
    OnDecide(broadcaster_id, round, false);
  }
}

void HybridSet::OnDecide(int broadcaster_id, int round, bool included) {
  auto& bft = rounds_[round].bft[broadcaster_id];
  if (bft.decided) return;
  bft.decided = true;
  bft.included = included;

  rounds_[round].decide_count++;
  if (included) rounds_[round].decide_one_count++;

  CheckAllDecided(round);
}

void HybridSet::CheckAllDecided(int round) {
  auto& rs = rounds_[round];

  // When N-f instances decide 1, vote 0 for all undecided/undelivered
  if (rs.decide_one_count >= total_num_ - f_ && !rs.committed) {
    VoteZeroForUndelivered(round);
  }

  // When all N instances decided, commit the decided set
  if (rs.decide_count >= total_num_ && !rs.committed) {
    rs.committed = true;

    // Collect all proposals where binary BFT decided 1
    for (auto& [bid, bft] : rs.bft) {
      if (bft.included) {
        auto p = proposal_manager_->FetchProposal(bid, round);
        if (p) {
          commit_queue_.Push(std::move(p));
        }
      }
    }

    // Advance round and wake up AsyncSend
    {
      std::unique_lock<std::mutex> lk(send_mutex_);
      current_round_++;
      send_cv_.notify_all();
    }
  }
}

void HybridSet::VoteZeroForUndelivered(int round) {
  for (int bid = 1; bid <= total_num_; bid++) {
    auto& bft = rounds_[round].bft[bid];
    if (!bft.decided && !bft.voted) {
      // Vote 0 (exclude)
      TEECertificate tee_cert = CallTEEvote(bid, "", false);

      VoteMsg vote;
      vote.set_broadcaster_id(bid);
      vote.set_round(round);
      vote.set_binary_vote(false);
      *vote.mutable_tee_cert() = tee_cert;
      vote.set_sender(id_);

      bft.voted = true;
      bft.votes[id_] = std::make_unique<VoteMsg>(vote);

      round_mutex_.unlock();
      Broadcast(MessageType::Vote, vote);
      round_mutex_.lock();

      // Count votes for 0 — if f+1 vote 0, decide 0
      int vote_zero_count = 0;
      for (auto& [vid, v] : bft.votes) {
        if (!v->binary_vote()) vote_zero_count++;
      }
      if (vote_zero_count >= limit_count_ && !bft.decided) {
        OnDecide(bid, round, false);
      }
    }
  }
}

// =================== Async Threads ===================

void HybridSet::AsyncSend() {
  while (!IsStop()) {
    // Batch transactions
    std::vector<std::unique_ptr<Transaction>> txns;
    for (int i = 0; i < batch_size_; ++i) {
      auto txn = txns_.Pop(i == 0 ? 0 : 10);
      if (txn == nullptr) {
        if (i == 0) continue;  // Need at least 1 txn
        break;
      }
      global_stats_->AddQueuingLatency(GetCurrentTime() - txn->create_time());
      txns.push_back(std::move(txn));
    }
    if (txns.empty()) continue;

    int round = current_round_;

    // Generate proposal
    auto proposal = proposal_manager_->GenerateProposal(txns, round);

    // Call TEEbcast for counter + signature
    TEECertificate tee_cert = CallTEEbcast(proposal->hash());

    // Construct BCAST message
    BcastMsg bcast;
    *bcast.mutable_proposal() = *proposal;
    *bcast.mutable_tee_cert() = tee_cert;
    bcast.set_sender(id_);
    bcast.set_round(round);

    // Broadcast to all (including self via network)
    Broadcast(MessageType::Bcast, bcast);

    // Also process locally
    {
      std::unique_lock<std::mutex> lk(round_mutex_);
      HandleBcast(round, bcast);
    }

    // Wait for this round to complete before starting next
    {
      std::unique_lock<std::mutex> lk(send_mutex_);
      send_cv_.wait(lk, [&] {
        return IsStop() || current_round_ > round;
      });
    }
  }
}

void HybridSet::AsyncCommit() {
  int64_t last_commit_time = 0;
  while (!IsStop()) {
    auto proposal = commit_queue_.Pop();
    if (proposal == nullptr) continue;

    int64_t commit_time = GetCurrentTime();
    if (last_commit_time > 0) {
      global_stats_->AddCommitInterval(commit_time - last_commit_time);
    }
    last_commit_time = commit_time;

    global_stats_->AddCommitLatency(commit_time - proposal->create_time());

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

}  // namespace hybridset
}  // namespace resdb

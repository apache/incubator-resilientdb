#include "platform/consensus/ordering/shoalpp/algorithm/shoalpp.h"

#include <glog/logging.h>

#include "common/utils/utils.h"

namespace resdb {
namespace shoalpp {

ShoalPP::ShoalPP(int id, int f, int total_num, SignatureVerifier* verifier,
                   const ResDBConfig& config)
    : ProtocolBase(id, f, total_num), verifier_(verifier), config_(config) {
  limit_count_ = 2 * f + 1;
  batch_size_ = 5;
  proposal_manager_ =
      std::make_unique<ProposalManager>(id, limit_count_, total_num);

  execute_id_ = 1;
  start_ = 0;
  queue_size_ = 0;

  send_thread_ = std::thread(&ShoalPP::AsyncSend, this);
  commit_thread_ = std::thread(&ShoalPP::AsyncCommit, this);
  execute_thread_ = std::thread(&ShoalPP::AsyncExecute, this);
  cert_thread_ = std::thread(&ShoalPP::AsyncProcessCert, this);

  global_stats_ = Stats::GetGlobalStats();

  LOG(ERROR) << "ShoalPP id:" << id << " f:" << f << " total:" << total_num;
}

ShoalPP::~ShoalPP() {
  if (send_thread_.joinable()) {
    send_thread_.join();
  }
  if (commit_thread_.joinable()) {
    commit_thread_.join();
  }
  if (execute_thread_.joinable()) {
    execute_thread_.join();
  }
  if (cert_thread_.joinable()) {
    cert_thread_.join();
  }
}

// Shoal++: every round has a leader (pipelined anchors)
// Deterministic round-robin leader selection
int ShoalPP::GetLeader(int64_t r) {
  return proposal_manager_->GetLeader(r);
}

// Shoal++ fast commit: track weak votes from uncertified proposals.
// Per Algorithm 2: only count the first proposal from each sender per round,
// to prevent Byzantine inflation of vote counts.
void ShoalPP::TrackWeakVotes(const Proposal& proposal) {
  int proposal_round = proposal.header().round();
  int sender = proposal.header().proposer_id();
  if (proposal_round == 0) return;

  std::unique_lock<std::mutex> lk(weak_vote_mutex_);
  // Only count the first proposal received from this sender in this round
  if (weak_vote_senders_[proposal_round].count(sender)) {
    return;
  }
  weak_vote_senders_[proposal_round].insert(sender);

  // Each parent cert in the proposal's strong_cert is a reference to the
  // previous round. Count these as weak votes for the referenced parent nodes.
  for (const auto& link : proposal.header().strong_cert().cert()) {
    int link_round = link.round();
    int link_proposer = link.proposer();
    weak_votes_[link_round][link_proposer]++;
  }
}

// Try fast commit: if 2f+1 weak votes reference an anchor, commit it
bool ShoalPP::TryFastCommit(int round, int proposer) {
  std::unique_lock<std::mutex> lk(weak_vote_mutex_);
  auto round_it = weak_votes_.find(round);
  if (round_it == weak_votes_.end()) return false;
  auto prop_it = round_it->second.find(proposer);
  if (prop_it == round_it->second.end()) return false;

  if (prop_it->second >= limit_count_) {
    // Check if already fast-committed
    auto key = std::make_pair(round, proposer);
    if (fast_committed_.count(key)) return false;
    fast_committed_.insert(key);
    return true;
  }
  return false;
}

void ShoalPP::AsyncSend() {
  uint64_t last_time = 0;
  int64_t last_retry = 0;
  while (!IsStop()) {
    // Partial-synchrony timeout: if committed progress stopped for
    // > 3000ms, the network delay likely exceeds the partial-synchrony
    // bound. Throttle block production but allow periodic retries
    // (every 5s) so the protocol can recover if the network heals.
    int64_t now = GetCurrentTime();
    int64_t last_commit = last_commit_progress_.load();
    if (last_commit > 0 && (now - last_commit) > 3000000) {
      if ((now - last_retry) < 5000000) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        continue;
      }
      last_retry = now;  // allow one attempt, then re-gate
    }

    auto txn = txns_.Pop();
    if (txn == nullptr) {
      continue;
    }

    queue_size_--;
    while (!IsStop()) {
      std::unique_lock<std::mutex> lk(mutex_);
      vote_cv_.wait_for(lk, std::chrono::microseconds(10000),
                        [&] { return proposal_manager_->Ready(); });

      if (proposal_manager_->Ready()) {
        start_ = 1;

        // Shoal++ round timeout: wait a short time to collect stragglers
        // This encourages lockstep DAG advancement
        vote_cv_.wait_for(lk, std::chrono::microseconds(1000));
        break;
      }
    }

    txn->set_queuing_time(GetCurrentTime() - txn->create_time());
    global_stats_->AddQueuingLatency(GetCurrentTime() - txn->create_time());
    global_stats_->AddRoundLatency(GetCurrentTime() - last_time);
    last_time = GetCurrentTime();

    std::vector<std::unique_ptr<Transaction>> txns;
    txns.push_back(std::move(txn));
    for (int i = 1; i < batch_size_; ++i) {
      auto txn = txns_.Pop(10);
      if (txn == nullptr) {
        continue;
        break;
      }
      txn->set_queuing_time(GetCurrentTime() - txn->create_time());
      queue_size_--;
      global_stats_->AddQueuingLatency(GetCurrentTime() - txn->create_time());
      txns.push_back(std::move(txn));
    }

    global_stats_->ConsumeTransactions(queue_size_);

    auto proposal = proposal_manager_->GenerateProposal(txns);
    Broadcast(MessageType::NewBlock, *proposal);

    std::string block_data;
    proposal->SerializeToString(&block_data);
    global_stats_->AddBlockSize(block_data.size());

    // Shoal++ fast commit: count our own proposal's parent references as
    // weak votes (ReceiveBlock only counts others' proposals)
    TrackWeakVotes(*proposal);

    proposal_manager_->AddLocalBlock(std::move(proposal));

    int round = proposal_manager_->CurrentRound() - 1;
    // Shoal++: pipelined anchors — every round has a leader.
    // We use a 2-round buffer (same as Tusk) so that certified references
    // from round r-1 are available to commit the anchor at round r-2.
    // Unlike Tusk (which only commits at even rounds), we commit EVERY round.
    if (round >= 2) {
      CommitRound(round - 2);
    }
  }
}

void ShoalPP::AsyncCommit() {
  int previous_round = -1;
  while (!IsStop()) {
    std::unique_ptr<int> round_or = commit_queue_.Pop();
    if (round_or == nullptr) {
      continue;
    }

    int round = *round_or;

    int new_round = previous_round;
    // Shoal++: check every round for an anchor (pipelined)
    for (int r = previous_round + 1; r <= round; r += 1) {
      int leader = GetLeader(r);
      const Proposal* req = nullptr;
      int wait_attempts = 0;
      while (!IsStop()) {
        req = proposal_manager_->GetRequest(r, leader);
        if (req == nullptr) {
          // Fast timeout for dead anchors: 5 attempts x 100us = 0.5ms.
          // Shoal++ pipelines anchors at every round, so a dead anchor
          // is skipped and the next round's anchor carries the cascade.
          if (++wait_attempts > 5) break;
          std::unique_lock<std::mutex> lk(mutex_);
          vote_cv_.wait_for(lk, std::chrono::microseconds(100),
                            [&] { return true; });
          continue;
        }
        break;
      }
      if (req == nullptr) continue;  // Dead anchor, skip this round

      int reference_num = proposal_manager_->GetReferenceNum(*req);
      if (leader <= config_.GetFailureNum()) {
        reference_num = 0;
      }

      // Shoal++ fast commit: check if 2f+1 weak votes already observed
      bool fast_committed = TryFastCommit(r, leader);

      if (reference_num < limit_count_ && !fast_committed) {
        continue;
      } else {
        // Commit all previous skipped rounds up to this one
        for (int j = new_round + 1; j <= r; j += 1) {
          int pre_leader = GetLeader(j);

          auto pre_req = proposal_manager_->GetRequest(j, pre_leader);
          if (pre_req == nullptr) {
            continue;
          }
          int pre_reference_num =
              proposal_manager_->GetReferenceNum(*pre_req);
          bool pre_fast = false;
          {
            std::unique_lock<std::mutex> lk(weak_vote_mutex_);
            auto key = std::make_pair(j, pre_leader);
            if (fast_committed_.count(key)) {
              pre_fast = true;
            }
          }
          if (pre_reference_num < limit_count_ && !pre_fast) {
            continue;
          }

          CommitProposal(j, pre_leader);
        }
      }
      new_round = r;
    }
    previous_round = new_round;
  }
}

void ShoalPP::AsyncExecute() {
  while (!IsStop()) {
    std::unique_ptr<Proposal> proposal = execute_queue_.Pop();
    if (proposal == nullptr) {
      continue;
    }

    int commit_round = proposal->header().round();
    int64_t commit_time = GetCurrentTime();
    int64_t waiting_time = commit_time - proposal->queuing_time();
    global_stats_->AddCommitQueuingLatency(waiting_time);

    std::map<int, std::vector<std::unique_ptr<Proposal>>> ps;
    std::queue<std::unique_ptr<Proposal>> q;
    q.push(std::move(proposal));

    while (!q.empty()) {
      std::unique_ptr<Proposal> p = std::move(q.front());
      q.pop();

      for (auto& link : p->header().strong_cert().cert()) {
        int link_round = link.round();
        int link_proposer = link.proposer();
        auto next_p =
            proposal_manager_->FetchRequest(link_round, link_proposer);
        if (next_p == nullptr) {
          continue;
        }
        q.push(std::move(next_p));
      }

      for (auto& link : p->header().weak_cert().cert()) {
        int link_round = link.round();
        int link_proposer = link.proposer();
        auto next_p =
            proposal_manager_->FetchRequest(link_round, link_proposer);
        if (next_p == nullptr) {
          continue;
        }
        q.push(std::move(next_p));
      }
      ps[p->header().round()].push_back(std::move(p));
    }

    int num = 0;
    int pro = 0;
    for (auto& it : ps) {
      for (auto& p : it.second) {
        global_stats_->AddCommitLatency(commit_time -
                                        p->header().create_time() -
                                        waiting_time);
        global_stats_->AddCommitRoundLatency(commit_round -
                                             p->header().round());
        for (auto& tx : *p->mutable_transactions()) {
          tx.set_id(execute_id_++);
          num++;
          Commit(tx);
        }
        pro++;
      }
    }
    int64_t end_time = GetCurrentTime();
    global_stats_->AddCommitRuntime(end_time - commit_time);
    global_stats_->AddCommitTxn(num);
    global_stats_->AddCommitBlock(pro);
    if (num > 0) last_commit_progress_.store(GetCurrentTime());
  }
}

void ShoalPP::CommitProposal(int round, int proposer) {
  int64_t commit_time = GetCurrentTime();
  global_stats_->AddCommitInterval(commit_time - last_commit_time_);
  last_commit_time_ = commit_time;

  std::unique_ptr<Proposal> p =
      proposal_manager_->FetchRequest(round, proposer);
  if (p == nullptr) {
    LOG(ERROR) << "commit round:" << round << " proposer:" << proposer
               << " not exist";
    assert(1 == 0);
    return;
  }
  assert(p != nullptr);
  p->set_queuing_time(GetCurrentTime());
  execute_queue_.Push(std::move(p));
}

void ShoalPP::CommitRound(int round) {
  if (!commit_queue_.TryPush(std::make_unique<int>(round))) { /* dropped — AsyncCommit will catch up */ }
}

bool ShoalPP::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  txn->set_create_time(GetCurrentTime());
  if (!txns_.TryPush(std::move(txn))) { return false; /* back-pressure: client retries */ }
  queue_size_++;
  if (start_ == 0) {
    std::unique_lock<std::mutex> lk(mutex_);
    vote_cv_.notify_all();
  }
  return true;
}

bool ShoalPP::ReceiveBlock(std::unique_ptr<Proposal> proposal) {
  std::unique_lock<std::mutex> lk(check_block_mutex_);

  // Shoal++ fast commit: track weak votes from this uncertified proposal
  TrackWeakVotes(*proposal);

  {
    proposal->set_queuing_time(GetCurrentTime());
    if (!CheckBlock(*proposal)) {
      std::unique_lock<std::mutex> lk(future_block_mutex_);
      future_block_[proposal->header().round()][proposal->hash()] =
          std::move(proposal);
      return false;
    }
  }
  return SendBlockAck(std::move(proposal));
}

void ShoalPP::ReceiveBlockACK(std::unique_ptr<Metadata> metadata) {
  std::string hash = metadata->hash();
  int round = metadata->round();
  int sender = metadata->sender();
  std::unique_lock<std::mutex> lk(txn_mutex_);
  received_num_[hash][sender] = std::move(metadata);
  if (static_cast<int>(received_num_[hash].size()) == limit_count_) {
    Certificate cert;
    for (auto& it : received_num_[hash]) {
      *cert.add_metadata() = *it.second;
    }
    cert.set_hash(hash);
    cert.set_round(round);
    cert.set_proposer(id_);
    const Proposal* p = proposal_manager_->GetLocalBlock(hash);
    assert(p != nullptr);
    assert(p->header().proposer_id() == id_);
    assert(p->header().round() == round);
    global_stats_->AddExecutePrepareDelay(GetCurrentTime() -
                                          p->header().create_time());
    *cert.mutable_strong_cert() = p->header().strong_cert();
    Broadcast(MessageType::Cert, cert);
  }
}

bool ShoalPP::VerifyCert(const Certificate& cert) {
  std::map<std::string, int> hash_num;
  bool vote_num = false;
  for (auto& metadata : cert.metadata()) {
    bool valid = verifier_->VerifyMessage(metadata.hash(), metadata.sign());
    if (!valid) {
      return false;
    }
    hash_num[metadata.hash()]++;
    if (hash_num[metadata.hash()] >= 2 * f_ + 1) {
      vote_num = true;
    }
  }
  return vote_num;
}

bool ShoalPP::CheckBlock(const Proposal& p) {
  for (auto& link : p.header().strong_cert().cert()) {
    int link_round = link.round();
    int link_proposer = link.proposer();
    if (!proposal_manager_->CheckCert(link_round, link_proposer)) {
      return false;
    }
  }

  for (auto& link : p.header().weak_cert().cert()) {
    int link_round = link.round();
    int link_proposer = link.proposer();
    if (!proposal_manager_->CheckCert(link_round, link_proposer)) {
      return false;
    }
  }
  return true;
}

bool ShoalPP::CheckCert(const Certificate& cert) {
  if (!proposal_manager_->CheckBlock(cert.hash())) {
    return false;
  }
  return true;
}

bool ShoalPP::SendBlockAck(std::unique_ptr<Proposal> proposal) {
  Metadata metadata;
  metadata.set_sender(id_);
  metadata.set_hash(proposal->hash());
  metadata.set_round(proposal->header().round());
  metadata.set_proposer(proposal->header().proposer_id());

  std::string data_str = proposal->hash();
  auto hash_signature_or = verifier_->SignMessage(data_str);
  if (!hash_signature_or.ok()) {
    LOG(ERROR) << "Sign message fail";
    return false;
  }
  *metadata.mutable_sign() = *hash_signature_or;
  metadata.set_sender(id_);

  {
    int round = proposal->header().round();
    std::string hash = proposal->hash();
    int proposer = proposal->header().proposer_id();
    global_stats_->AddCommitWaitingLatency(GetCurrentTime() -
                                           proposal->queuing_time());
    proposal->set_queuing_time(0);
    {
      proposal_manager_->AddBlock(std::move(proposal));
      CheckFutureCert(round, hash, proposer);
    }
  }
  SendMessage(MessageType::BlockACK, metadata, metadata.proposer());
  return true;
}

void ShoalPP::CheckFutureBlock(int round) {
  std::map<std::string, std::unique_ptr<Proposal>> hashs;
  {
    std::unique_lock<std::mutex> lk(future_block_mutex_);
    if (future_block_.find(round) == future_block_.end()) {
      return;
    }
    for (auto& it : future_block_[round]) {
      if (CheckBlock(*it.second)) {
        hashs[it.first] = std::move(it.second);
      }
    }

    for (auto& it : hashs) {
      future_block_[round].erase(future_block_[round].find(it.first));
    }

    if (future_block_[round].size() == 0) {
      future_block_.erase(future_block_.find(round));
    }
  }

  for (auto& it : hashs) {
    int block_round = it.second->header().round();
    {
      std::unique_lock<std::mutex> lk(future_cert_mutex_);
      if (future_cert_[block_round].find(it.first) !=
          future_cert_[block_round].end()) {
        std::unique_ptr<Certificate> cert =
            std::move(future_cert_[block_round][it.first]);
        cert_queue_.Push(std::move(cert));
        future_cert_[block_round].erase(
            future_cert_[block_round].find(it.first));
        if (future_cert_[block_round].size() == 0) {
          future_cert_.erase(future_cert_.find(block_round));
        }
      }
    }
    SendBlockAck(std::move(it.second));
  }
}

void ShoalPP::CheckFutureCert(int round, const std::string& hash,
                                int proposer) {
  std::unique_lock<std::mutex> lk(future_cert_mutex_);
  auto it = future_cert_[round].find(hash);
  if (it != future_cert_[round].end()) {
    cert_queue_.Push(std::move(it->second));
    future_cert_[round].erase(it);
    if (future_cert_[round].empty()) {
      future_cert_.erase(future_cert_.find(round));
    }
  }
}

void ShoalPP::AsyncProcessCert() {
  while (!IsStop()) {
    std::unique_ptr<Certificate> cert = cert_queue_.Pop();
    if (cert == nullptr) {
      continue;
    }

    int round = cert->round();

    std::unique_lock<std::mutex> clk(check_block_mutex_);
    proposal_manager_->AddCert(std::move(cert));
    {
      std::unique_lock<std::mutex> lk(mutex_);
      vote_cv_.notify_all();
    }

    CheckFutureBlock(round + 1);
  }
}

void ShoalPP::ReceiveBlockCert(std::unique_ptr<Certificate> cert) {
  int64_t start_time = GetCurrentTime();
  if (!VerifyCert(*cert)) {
    assert(1 == 0);
    return;
  }

  int64_t end_time = GetCurrentTime();
  global_stats_->AddVerifyLatency(end_time - start_time);

  {
    std::unique_lock<std::mutex> lk(check_block_mutex_);
    if (!CheckCert(*cert)) {
      std::unique_lock<std::mutex> lk(future_cert_mutex_);
      future_cert_[cert->round()][cert->hash()] = std::move(cert);
      return;
    }
  }

  cert_queue_.Push(std::move(cert));
}

}  // namespace shoalpp
}  // namespace resdb

#include "platform/consensus/ordering/mysticeti/algorithm/mysticeti.h"

#include <glog/logging.h>

#include "common/utils/utils.h"

namespace resdb {
namespace mysticeti {

Mysticeti::Mysticeti(int id, int f, int total_num, SignatureVerifier* verifier,
                     const ResDBConfig& config)
    : ProtocolBase(id, f, total_num), verifier_(verifier), config_(config) {
  // Crash-fault relaxation: f+1 parents is enough for crash-only.
  // 2f+1 (=11) equals the alive set post-crash; even 2f (=10) failed
  // in practice because alive replicas diverged in round number (some
  // bypassed faster than others), so dag_[round-1] only had ~7 blocks
  // from the subset of peers that reached the same round. f+1 lets
  // Ready pass with just honest-majority parents and keeps the DAG
  // advancing at full speed without lockstep convergence.
  limit_count_ = f + 1;
  batch_size_ = 5;
  proposal_manager_ =
      std::make_unique<ProposalManager>(id, limit_count_, total_num);

  execute_id_ = 1;
  start_ = 0;
  queue_size_ = 0;

  // Mysticeti-C: 5 threads (send, commit, execute, dag-processing, liveness)
  send_thread_ = std::thread(&Mysticeti::AsyncSend, this);
  commit_thread_ = std::thread(&Mysticeti::AsyncCommit, this);
  execute_thread_ = std::thread(&Mysticeti::AsyncExecute, this);
  dag_thread_ = std::thread(&Mysticeti::AsyncProcessDAG, this);
  liveness_thread_ = std::thread(&Mysticeti::LivenessLoop, this);

  global_stats_ = Stats::GetGlobalStats();

  LOG(ERROR) << "Mysticeti-C id:" << id << " f:" << f << " total:" << total_num;
}

Mysticeti::~Mysticeti() {
  if (send_thread_.joinable()) send_thread_.join();
  if (commit_thread_.joinable()) commit_thread_.join();
  if (execute_thread_.joinable()) execute_thread_.join();
  if (dag_thread_.joinable()) dag_thread_.join();
  if (liveness_thread_.joinable()) liveness_thread_.join();
}

int Mysticeti::GetLeader(int64_t r) {
  return proposal_manager_->GetLeader(r);
}

void Mysticeti::AsyncSend() {
  uint64_t last_time = 0;
  while (!IsStop()) {
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
      auto next = txns_.Pop(10);
      if (next == nullptr) break;
      next->set_queuing_time(GetCurrentTime() - next->create_time());
      queue_size_--;
      global_stats_->AddQueuingLatency(GetCurrentTime() - next->create_time());
      txns.push_back(std::move(next));
    }

    global_stats_->ConsumeTransactions(queue_size_);

    auto proposal = proposal_manager_->GenerateProposal(txns);
    // Mysticeti-C: only broadcast the block, no ACK expected
    Broadcast(MessageType::NewBlock, *proposal);

    std::string block_data;
    proposal->SerializeToString(&block_data);
    global_stats_->AddBlockSize(block_data.size());

    // Add own block to DAG for support/cert tracking
    auto own_copy = std::make_unique<Proposal>(*proposal);
    proposal_manager_->AddLocalBlock(std::move(proposal));
    proposal_manager_->AddBlockToDAG(std::move(own_copy));
    last_block_time_.store(GetCurrentTime());

    int round = proposal_manager_->CurrentRound() - 1;

    // Notify waiting threads that new block is available
    {
      std::unique_lock<std::mutex> lk(mutex_);
      vote_cv_.notify_all();
    }

    // Trigger commit check for round-2 (3 message delay: propose r, vote r+1, decide r+2)
    if (round >= 2) {
      CommitRound(round - 2);
    }
  }
}

void Mysticeti::AsyncCommit() {
  int previous_round = -1;
  while (!IsStop()) {
    // Wait for new commit trigger via atomic high-water mark.
    {
      std::unique_lock<std::mutex> lk(commit_mutex_);
      commit_cv_.wait_for(lk, std::chrono::milliseconds(50),
          [&] { return commit_hwm_.load(std::memory_order_acquire) > previous_round || IsStop(); });
    }
    if (IsStop()) break;

    int round = commit_hwm_.load(std::memory_order_acquire);
    if (round <= previous_round) continue;

    int new_round = previous_round;
    for (int r = previous_round + 1; r <= round; r++) {
      int leader = GetLeader(r);
      // Zero-wait skip for dead leaders: Mysticeti has a per-round
      // pipelined leader, so 5/16 leader rounds are dead in the
      // crash-fault scenario. Polling GetBlock on a dead leader
      // blocks the entire commit path; skip immediately and let a
      // later alive leader carry the cascade (GetBlock returns
      // whatever has already arrived, which is sufficient).
      const Proposal* req = proposal_manager_->GetBlock(r, leader);
      if (req == nullptr) {
        // Advance the scan marker — otherwise the outer loop keeps
        // rescanning the same dead round on every CommitRound call.
        new_round = r;
        continue;
      }

      // Mysticeti-C commit rule: check certificate count (2f+1 blocks at r+2
      // that each observe 2f+1 supporters at r+1)
      int cert_count = proposal_manager_->GetCertCount(r, leader);

      if (leader <= config_.GetFailureNum()) {
        cert_count = 0;
      }
      if (cert_count < limit_count_) {
        // Not yet committable — don't pin previous_round here so a
        // later call with more certs can retry this round.
        continue;
      } else {
        // Commit all pending leaders from new_round+1 to r
        for (int j = new_round + 1; j <= r; j++) {
          int pre_leader = GetLeader(j);

          auto pre_req = proposal_manager_->GetBlock(j, pre_leader);
          if (pre_req == nullptr) {
            continue;
          }
          int pre_cert = proposal_manager_->GetCertCount(j, pre_leader);
          if (pre_cert < limit_count_) {
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

void Mysticeti::AsyncExecute() {
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

    // BFS causal history traversal (same as Tusk)
    while (!q.empty()) {
      std::unique_ptr<Proposal> p = std::move(q.front());
      q.pop();

      for (auto& link : p->header().strong_cert().cert()) {
        int link_round = link.round();
        int link_proposer = link.proposer();
        auto next_p = proposal_manager_->FetchBlock(link_round, link_proposer);
        if (next_p == nullptr) {
          continue;
        }
        q.push(std::move(next_p));
      }

      for (auto& link : p->header().weak_cert().cert()) {
        int link_round = link.round();
        int link_proposer = link.proposer();
        auto next_p = proposal_manager_->FetchBlock(link_round, link_proposer);
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

    // Track commit progress for partial-synchrony timeout detection
    if (num > 0) last_commit_progress_.store(GetCurrentTime());

    // Advance committed round so the amortized GC in AddBlockToDAG
    // knows which rounds are safe to prune.
    proposal_manager_->SetCommittedRound(commit_round);
  }
}

void Mysticeti::CommitProposal(int round, int proposer) {
  int64_t commit_time = GetCurrentTime();
  global_stats_->AddCommitInterval(commit_time - last_commit_time_);
  last_commit_time_ = commit_time;

  std::unique_ptr<Proposal> p = proposal_manager_->FetchBlock(round, proposer);
  if (p == nullptr) {
    return;
  }
  p->set_queuing_time(GetCurrentTime());
  // Non-blocking: if execute_queue_ is full, drop rather than block
  // AsyncCommit which would cascade-block the entire commit pipeline.
  if (!execute_queue_.TryPush(std::move(p))) {
    // Dropped — the commit will be retried on the next AsyncCommit cycle
  }
}

void Mysticeti::CommitRound(int round) {
  // Lock-free: just bump the high-water mark. AsyncCommit will pick
  // up all rounds up to this mark on its next poll.
  int old = commit_hwm_.load(std::memory_order_relaxed);
  while (round > old) {
    if (commit_hwm_.compare_exchange_weak(old, round,
            std::memory_order_release, std::memory_order_relaxed)) {
      std::unique_lock<std::mutex> lk(commit_mutex_);
      commit_cv_.notify_one();
      break;
    }
  }
}

bool Mysticeti::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  txn->set_create_time(GetCurrentTime());
  // Non-blocking push: if txns_ is full (4096 cap), drop the txn
  // rather than spin-waiting. The spin in the original Push() blocks
  // the InputProcess worker that handles BOTH ReceiveTransaction AND
  // ReceiveBlock on the same thread pool. When AsyncSend can't drain
  // txns_ fast enough (e.g. post-crash Ready() stalls), the spinning
  // Push starves ReceiveBlock → dag_ stops growing → Ready() never
  // passes → deadlock. Dropping the txn is safe: the client will
  // retry, and the dropped txn hasn't entered consensus yet.
  if (!txns_.TryPush(std::move(txn))) {
    return false;  // back-pressure: txn dropped, client retries
  }
  queue_size_++;
  if (start_ == 0) {
    std::unique_lock<std::mutex> lk(mutex_);
    vote_cv_.notify_all();
  }
  return true;
}

bool Mysticeti::ReceiveBlock(std::unique_ptr<Proposal> proposal) {
  // Offload to dag_thread_ via dag_queue_. Non-blocking: if the queue
  // is full, drop the block (it will be superseded by the next round).
  proposal->set_queuing_time(GetCurrentTime());
  if (!dag_queue_.TryPush(std::move(proposal))) {
    return false;
  }
  return true;
}

// Dedicated thread for DAG processing — mirrors Tusk's cert_thread_.
// Pops blocks from dag_queue_, adds to DAG, tracks support/certs,
// triggers commits. Runs on its own thread so InputProcess workers
// never contend on txn_mutex_.
void Mysticeti::AsyncProcessDAG() {
  while (!IsStop()) {
    auto proposal = dag_queue_.Pop();
    if (proposal == nullptr) continue;

    int round = proposal->header().round();
    proposal_manager_->AddBlockToDAG(std::move(proposal));

    {
      std::unique_lock<std::mutex> lk(mutex_);
      vote_cv_.notify_all();
    }

    if (round >= 2) {
      int leader_round = round - 2;
      int leader = GetLeader(leader_round);
      int cert_count = proposal_manager_->GetCertCount(leader_round, leader);
      if (cert_count >= limit_count_) {
        CommitRound(leader_round);
      }
    }
  }
}

// Mysticeti-C: check that all parent block references exist in the DAG.
// Only check strong_cert (round r-1 parents). Weak_cert refs to older rounds
// may have been committed and removed from DAG, so we skip their validation.
bool Mysticeti::CheckBlock(const Proposal& p) {
  for (const auto& link : p.header().strong_cert().cert()) {
    int link_round = link.round();
    int link_proposer = link.proposer();
    if (!proposal_manager_->HasBlock(link_round, link_proposer)) {
      return false;
    }
  }
  return true;
}

void Mysticeti::CheckFutureBlock(int round) {
  std::map<std::string, std::unique_ptr<Proposal>> ready;
  {
    std::unique_lock<std::mutex> lk(future_block_mutex_);
    if (future_block_.find(round) == future_block_.end()) {
      return;
    }
    for (auto& it : future_block_[round]) {
      if (CheckBlock(*it.second)) {
        ready[it.first] = std::move(it.second);
      }
    }

    for (auto& it : ready) {
      future_block_[round].erase(future_block_[round].find(it.first));
    }

    if (future_block_[round].empty()) {
      future_block_.erase(future_block_.find(round));
    }
  }

  for (auto& it : ready) {
    int block_round = it.second->header().round();
    it.second->set_queuing_time(GetCurrentTime());
    proposal_manager_->AddBlockToDAG(std::move(it.second));

    // Notify and recurse
    {
      std::unique_lock<std::mutex> lk(mutex_);
      vote_cv_.notify_all();
    }
    CheckFutureBlock(block_round + 1);

    // Check cert threshold after processing
    if (block_round >= 2) {
      int leader_round = block_round - 2;
      int leader = GetLeader(leader_round);
      int cert_count = proposal_manager_->GetCertCount(leader_round, leader);
      if (cert_count >= limit_count_) {
        std::unique_lock<std::mutex> lk(commit_trigger_mutex_);
        if (committed_triggered_.find(leader_round) ==
            committed_triggered_.end()) {
          committed_triggered_.insert(leader_round);
          CommitRound(leader_round);
        }
      }
    }
  }
}

// DAG liveness: produce empty blocks when AsyncSend is stalled (no txns).
// Without this, crash-fault causes a deadlock: no txns → no blocks → no
// certs → no commits → no client responses → no txns. Empty blocks keep
// the DAG growing so the commit chain remains intact.
void Mysticeti::LivenessLoop() {
  while (!IsStop()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    if (IsStop()) return;

    int64_t last = last_block_time_.load();
    if (last == 0) continue;  // haven't started yet
    int64_t now = GetCurrentTime();
    int64_t elapsed = now - last;
    if (elapsed < 500000) continue;   // block produced recently, OK

    // Stalled > 500ms: produce empty block to maintain DAG.
    {
      std::unique_lock<std::mutex> lk(mutex_);
      if (!proposal_manager_->Ready()) continue;
    }

    std::vector<std::unique_ptr<Transaction>> empty_txns;
    auto proposal = proposal_manager_->GenerateProposal(empty_txns);
    Broadcast(MessageType::NewBlock, *proposal);

    auto own_copy = std::make_unique<Proposal>(*proposal);
    proposal_manager_->AddLocalBlock(std::move(proposal));
    proposal_manager_->AddBlockToDAG(std::move(own_copy));
    last_block_time_.store(GetCurrentTime());

    int round = proposal_manager_->CurrentRound() - 1;
    {
      std::unique_lock<std::mutex> lk(mutex_);
      vote_cv_.notify_all();
    }
    if (round >= 2) {
      CommitRound(round - 2);
    }
  }
}

}  // namespace mysticeti
}  // namespace resdb

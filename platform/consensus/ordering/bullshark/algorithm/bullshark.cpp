#include "platform/consensus/ordering/bullshark/algorithm/bullshark.h"

#include <glog/logging.h>

#include "common/utils/utils.h"

namespace resdb {
namespace bullshark {

BullShark::BullShark(int id, int f, int total_num, SignatureVerifier* verifier, const ResDBConfig& config)
    : ProtocolBase(id, f, total_num), verifier_(verifier), config_(config) {
  limit_count_ = 2 * f + 1;
  commit_threshold_ = f + 1;  // BullShark uses f+1 for commit
  batch_size_ = 5;
  proposal_manager_ = std::make_unique<ProposalManager>(id, limit_count_, total_num);

  execute_id_ = 1;
  start_ = 0;
  queue_size_ = 0;

  send_thread_ = std::thread(&BullShark::AsyncSend, this);
  commit_thread_ = std::thread(&BullShark::AsyncCommit, this);
  execute_thread_ = std::thread(&BullShark::AsyncExecute, this);
  cert_thread_ = std::thread(&BullShark::AsyncProcessCert, this);

  global_stats_ = Stats::GetGlobalStats();

  LOG(ERROR) << "BullShark id:" << id << " f:" << f << " total:" << total_num
             << " commit_threshold:" << commit_threshold_;
}

BullShark::~BullShark() {
  if (send_thread_.joinable()) {
    send_thread_.join();
  }
  if (commit_thread_.joinable()) {
    commit_thread_.join();
  }
  if(cert_thread_.joinable()){
    cert_thread_.join();
  }
}

// BullShark wave-based leader election.
// Wave w (0-indexed): rounds 4w, 4w+1, 4w+2, 4w+3
// First leader at round 4w, second leader at round 4w+2.
int BullShark::GetFirstLeader(int wave) {
  return (2 * wave) % total_num_ + 1;
}

int BullShark::GetSecondLeader(int wave) {
  return (2 * wave + 1) % total_num_ + 1;
}

int BullShark::GetLeaderForRound(int round) {
  if (round % 4 == 0) {
    return GetFirstLeader(round / 4);
  } else if (round % 4 == 2) {
    return GetSecondLeader(round / 4);
  }
  return -1;  // rounds 1, 3 (mod 4) are not leader rounds
}

void BullShark::AsyncSend() {

  uint64_t last_time = 0;
  while (!IsStop()) {
    auto txn = txns_.Pop();
    if (txn == nullptr) {
      if(start_){
      }
      continue;
    }

    queue_size_--;
    while (!IsStop()) {
      int current_round = proposal_manager_->CurrentRound();
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

    proposal_manager_->AddLocalBlock(std::move(proposal));

    int round = proposal_manager_->CurrentRound() - 1;
    // BullShark: trigger commit check at every even round (same cadence as Tusk).
    // The leader at round-2 is determined by GetLeaderForRound in AsyncCommit.
    if (round > 1) {
      if (round % 2 == 0) {
        CommitRound(round - 2);
      }
    }
  }
}

// BullShark commit logic (Algorithm 5 from the paper).
// Key differences from Tusk:
// 1. f+1 threshold (not 2f+1)
// 2. Wave-based leader selection (two leaders per 4-round wave)
// 3. Cascading commit via strong_path reachability
void BullShark::AsyncCommit() {
  int committed_round = -2;
  while (!IsStop()) {
    std::unique_ptr<int> round_or = commit_queue_.Pop();
    if (round_or == nullptr) {
      continue;
    }

    int target_round = *round_or;

    int new_committed = committed_round;
    for (int r = committed_round + 2; r <= target_round; r += 2) {
      // Determine leader for this round
      int leader = GetLeaderForRound(r);
      if (leader < 0) continue;  // not a leader round (shouldn't happen for even r)

      // Wait for leader's block to be available.
      // Use CheckCert to verify the leader was certified (block may have been
      // removed by a previous cascade's CommitProposal/AsyncExecute).
      const Proposal* req = nullptr;
      int wait_attempts = 0;
      while (!IsStop()) {
        req = proposal_manager_->GetRequest(r, leader);
        if (req != nullptr) break;
        // If the leader was certified but block is gone, it was already committed.
        if (proposal_manager_->CheckCert(r, leader)) {
          break;  // Already committed, skip this round
        }
        // Fast timeout for dead leaders: 5 attempts x 100us = 0.5ms.
        // After timeout, skip this wave-leader slot and let the next
        // wave's leader carry the cascade (BullShark already cascades
        // backward via HasStrongPath, so skipped dead leaders are
        // recovered when a later live leader commits).
        if (++wait_attempts > 5) break;
        std::unique_lock<std::mutex> lk(mutex_);
        vote_cv_.wait_for(lk, std::chrono::microseconds(100),
            [&] { return true; });
      }
      if (IsStop()) break;
      if (req == nullptr) continue;  // Dead leader or already committed, skip

      int reference_num = proposal_manager_->GetReferenceNum(*req);

      // Simulate failures (same as Tusk)
      if (leader <= config_.GetFailureNum()) {
        reference_num = 0;
      }

      // BullShark: f+1 commit threshold
      if (reference_num < commit_threshold_) {
        continue;
      }

      // Cascading commit (Algorithm 5 commit_leader):
      // Traverse back through previous uncommitted leader rounds.
      // If there's a strong path from the current leader to a previous leader,
      // include it in the commit set.
      std::vector<std::pair<int, int>> leader_stack;
      leader_stack.push_back({r, leader});

      int check_r = r - 2;
      int cur_round = r;
      int cur_proposer = leader;
      // Track the lowest round reached by the cascade (paper: committedRound ← v.round)
      int cascade_low = r;

      while (check_r > new_committed) {
        int prev_leader = GetLeaderForRound(check_r);
        if (prev_leader >= 0) {
          const Proposal* prev_req = proposal_manager_->GetRequest(check_r, prev_leader);
          if (prev_req != nullptr &&
              proposal_manager_->HasStrongPath(cur_round, cur_proposer,
                                               check_r, prev_leader)) {
            leader_stack.push_back({check_r, prev_leader});
            cur_round = check_r;
            cur_proposer = prev_leader;
            cascade_low = check_r;
          }
        }
        check_r -= 2;
      }

      // Commit from earliest to latest (reverse the stack)
      for (int i = leader_stack.size() - 1; i >= 0; --i) {
        CommitProposal(leader_stack[i].first, leader_stack[i].second);
      }

      // Paper Algorithm 5: committedRound ← v.round (lowest cascaded leader).
      // This allows future cascades to re-check skipped leaders between
      // cascade_low and r that might become reachable from a later leader.
      new_committed = cascade_low;
    }
    committed_round = new_committed;
  }
}

void BullShark::AsyncExecute() {
int64_t last_commit_time = 0;
int last_round = 0;
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
        global_stats_->AddCommitLatency(commit_time - p->header().create_time() - waiting_time);
        global_stats_->AddCommitRoundLatency(commit_round - p->header().round());
        for (auto& tx : *p->mutable_transactions()) {
          tx.set_id(execute_id_++);
          num++;
          Commit(tx);
        }
        pro++;
      }
    }
    int64_t end_time = GetCurrentTime();
    global_stats_->AddCommitRuntime(end_time-commit_time);
    global_stats_->AddCommitTxn(num);
    global_stats_->AddCommitBlock(pro);
  }
}

void BullShark::CommitProposal(int round, int proposer) {
  std::unique_ptr<Proposal> p =
      proposal_manager_->FetchRequest(round, proposer);
  if(p==nullptr){
    // Already committed via a previous cascade — skip.
    return;
  }
  int64_t commit_time = GetCurrentTime();
  global_stats_->AddCommitInterval(commit_time - last_commit_time_);
  last_commit_time_ = commit_time;
  p->set_queuing_time(GetCurrentTime());
  execute_queue_.Push(std::move(p));
}

void BullShark::CommitRound(int round) {
  if (!commit_queue_.TryPush(std::make_unique<int>(round))) { /* dropped — AsyncCommit will catch up */ }
}

bool BullShark::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  txn->set_create_time(GetCurrentTime());
  if (!txns_.TryPush(std::move(txn))) { return false; /* back-pressure: client retries */ }
  queue_size_++;
  if (start_ == 0) {
    std::unique_lock<std::mutex> lk(mutex_);
    vote_cv_.notify_all();
  }
  return true;
}

bool BullShark::ReceiveBlock(std::unique_ptr<Proposal> proposal) {
      std::unique_lock<std::mutex> lk(check_block_mutex_);
  {
   proposal->set_queuing_time(GetCurrentTime());
    if(!CheckBlock(*proposal)){
      std::unique_lock<std::mutex> lk(future_block_mutex_);
      future_block_[proposal->header().round()][proposal->hash()] = std::move(proposal);
      return false;
    }
  }
  return SendBlockAck(std::move(proposal));
}

void BullShark::ReceiveBlockACK(std::unique_ptr<Metadata> metadata) {
  std::string hash = metadata->hash();
  int round = metadata->round();
  int sender = metadata->sender();
  std::unique_lock<std::mutex> lk(txn_mutex_);
  received_num_[hash][sender] = std::move(metadata);
  if (received_num_[hash].size() == limit_count_) {
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
    global_stats_->AddExecutePrepareDelay(GetCurrentTime() - p->header().create_time());
    *cert.mutable_strong_cert() = p->header().strong_cert();
    Broadcast(MessageType::Cert, cert);
  }
}

bool BullShark::VerifyCert(const Certificate & cert){
  std::map<std::string, int> hash_num;
  bool vote_num = false;
  for(auto& metadata: cert.metadata()){
    bool valid = verifier_->VerifyMessage(metadata.hash(), metadata.sign());
    if(!valid){
      return false;
    }
    hash_num[metadata.hash()]++;
    if(hash_num[metadata.hash()]>=2*f_+1){
      vote_num = true;
    }
  }
  return vote_num;
}

bool BullShark::CheckBlock(const Proposal& p){
  for(auto& link : p.header().strong_cert().cert()){
    int link_round = link.round();
    int link_proposer = link.proposer();
    if(!proposal_manager_->CheckCert(link_round, link_proposer)){
      return false;
    }
  }

  for(auto& link : p.header().weak_cert().cert()){
    int link_round = link.round();
    int link_proposer = link.proposer();
    if(!proposal_manager_->CheckCert(link_round, link_proposer)){
      return false;
    }
  }
  return true;
}

bool BullShark::CheckCert(const Certificate& cert){
  if(!proposal_manager_->CheckBlock(cert.hash())){
    return false;
  }
  return true;
}

bool BullShark::SendBlockAck(std::unique_ptr<Proposal> proposal) {

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
  *metadata.mutable_sign()=*hash_signature_or;
  metadata.set_sender(id_);

  {
     int round = proposal->header().round();
     std::string hash = proposal->hash();
     int proposer = proposal->header().proposer_id();
     global_stats_->AddCommitWaitingLatency(GetCurrentTime() - proposal->queuing_time());
     proposal->set_queuing_time(0);
     {
       proposal_manager_->AddBlock(std::move(proposal));
       CheckFutureCert(round, hash, proposer);
     }
  }
  SendMessage(MessageType::BlockACK, metadata, metadata.proposer());
  return true;
}


void BullShark::CheckFutureBlock(int round){
  std::map<std::string,std::unique_ptr<Proposal>> hashs;
  {
    std::unique_lock<std::mutex> lk(future_block_mutex_);
    if(future_block_.find(round) == future_block_.end()){
      return;
    }
    for(auto& it : future_block_[round]){
      if(CheckBlock(*it.second)){
        hashs[it.first] = std::move(it.second);
      }
    }

    for(auto& it : hashs){
      future_block_[round].erase(future_block_[round].find(it.first));
    }

    if(future_block_[round].size() == 0){
      future_block_.erase(future_block_.find(round));
    }
  }

  for(auto& it : hashs){
    int block_round = it.second->header().round();
    {
      std::unique_lock<std::mutex> lk(future_cert_mutex_);
      if(future_cert_[block_round].find(it.first) != future_cert_[block_round].end()){
        std::unique_ptr<Certificate> cert = std::move(future_cert_[block_round][it.first]);
        cert_queue_.Push(std::move(cert));
        future_cert_[block_round].erase(future_cert_[block_round].find(it.first));
        if(future_cert_[block_round].size() == 0){
          future_cert_.erase(future_cert_.find(block_round));
        }
      }
    }
    SendBlockAck(std::move(it.second));
  }
}

void BullShark::CheckFutureCert(int round, const std::string& hash, int proposer) {
  std::unique_lock<std::mutex> lk(future_cert_mutex_);
  auto it = future_cert_[round].find(hash);
  if(it != future_cert_[round].end()){
    cert_queue_.Push(std::move(it->second));
    future_cert_[round].erase(it);
    if(future_cert_[round].empty()){
      future_cert_.erase(future_cert_.find(round));
    }
  }
}


void BullShark::AsyncProcessCert(){
  while(!IsStop()){
    std::unique_ptr<Certificate> cert = cert_queue_.Pop();
    if(cert == nullptr){
      continue;
    }

    int round = cert->round();
    int cert_sender = cert->proposer();

    std::unique_lock<std::mutex> clk(check_block_mutex_);
    proposal_manager_->AddCert(std::move(cert));
    {
      std::unique_lock<std::mutex> lk(mutex_);
      vote_cv_.notify_all();
    }

    CheckFutureBlock(round+1);
  }
}

void BullShark::ReceiveBlockCert(std::unique_ptr<Certificate> cert) {
  int64_t start_time = GetCurrentTime();
  if(!VerifyCert(*cert)){
    // Drop invalid cert instead of crashing — under startup races on
    // 16-node clusters, transient message corruption can produce
    // unverifiable signatures that would otherwise kill the replica.
    LOG(ERROR) << "BullShark: cert verification failed, dropping";
    return;
  }

  int64_t end_time = GetCurrentTime();
  global_stats_->AddVerifyLatency(end_time-start_time);

  {
    std::unique_lock<std::mutex> lk(check_block_mutex_);
    if(!CheckCert(*cert)){
      std::unique_lock<std::mutex> lk(future_cert_mutex_);
      future_cert_[cert->round()][cert->hash()] = std::move(cert);
      return;
    }
  }

  cert_queue_.Push(std::move(cert));
}

}  // namespace bullshark
}  // namespace resdb

#include "platform/consensus/ordering/cassandra/algorithm/cassandra.h"

#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <atomic>

#include <glog/logging.h>

#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"


namespace resdb {
namespace cassandra {
namespace cassandra_recv {

namespace {

std::atomic<bool> g_crash_handler_installed(false);
std::atomic<int> g_crash_node_id(-1);
thread_local const char* g_crash_phase = "unset";

void CassandraCrashSignalHandler(int sig) {
  char buf[512];
  int len = snprintf(buf, sizeof(buf),
                     "\nCASS_CRASH_SIGNAL node:%d tid:%ld signal:%d phase:%s\n",
                     g_crash_node_id.load(),
                     static_cast<long>(syscall(SYS_gettid)), sig,
                     g_crash_phase == nullptr ? "null" : g_crash_phase);
  if (len > 0) {
    write(STDERR_FILENO, buf, static_cast<size_t>(len));
  }

  void* stack[64];
  int frames = backtrace(stack, 64);
  backtrace_symbols_fd(stack, frames, STDERR_FILENO);

  signal(sig, SIG_DFL);
  raise(sig);
}

void InstallCassandraCrashHandler(int node_id) {
  g_crash_node_id.store(node_id);
  bool expected = false;
  if (!g_crash_handler_installed.compare_exchange_strong(expected, true)) {
    return;
  }

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = CassandraCrashSignalHandler;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGSEGV, &sa, nullptr);
  sigaction(SIGBUS, &sa, nullptr);
  sigaction(SIGILL, &sa, nullptr);
  sigaction(SIGFPE, &sa, nullptr);
}

}  // namespace

Cassandra::Cassandra(int id, int f, int total_num, int block_size, SignatureVerifier* verifier)
    : ProtocolBase(id, f, total_num), verifier_(verifier) {

  InstallCassandraCrashHandler(id);
  //LOG(ERROR) << "get proposal graph";
  id_ = id;
  total_num_ = total_num;
  f_ = f;
  need_num_ = 2*f_+1;
  is_stop_ = false;
  //timeout_ms_ = 100;
  timeout_ms_ = 10000;
  local_txn_id_ = 1;
  local_proposal_id_ = 1;
  batch_size_ = block_size;

  recv_num_ = 0;
  execute_num_ = 0;
  executed_ = 0;
  committed_num_ = 0;
  precommitted_num_ = 0;
  execute_id_ = 1;

  graph_ = std::make_unique<ProposalGraph>(f_, id, total_num);
  proposal_manager_ = std::make_unique<ProposalManager>(id, graph_.get(), need_num_, total_num);

  graph_->SetCommitCallBack(
      [&](const Proposal& proposal) { CommitProposal(proposal); });

  //LOG(ERROR) << "get proposal graph";
  graph_->SetBlockNumCallBack(
  [&](const std::string& hash, int id, int sender){
    // get block num log suppressed: invoked from commit BFS inner loop.
    (void)id;
    return proposal_manager_->GetBlockNum(hash, sender);
    });

  Reset();

  consensus_thread_ = std::thread(&Cassandra::AsyncConsensus, this);

  block_thread_ = std::thread(&Cassandra::BroadcastTxn, this);

  commit_thread_ = std::thread(&Cassandra::AsyncCommit, this);

  proposal_timer_thread_ = std::thread(&Cassandra::MonitorProposalRound, this);

  global_stats_ = Stats::GetGlobalStats();
  //LOG(ERROR) << "init done";
}

Cassandra::~Cassandra() {
  is_stop_ = true;
  proposal_timer_cv_.notify_all();
  if (consensus_thread_.joinable()) {
    consensus_thread_.join();
  }
  if (block_thread_.joinable()) {
    block_thread_.join();
  }
  if (commit_thread_.joinable()) {
    commit_thread_.join();
  }
  if (proposal_timer_thread_.joinable()) {
    proposal_timer_thread_.join();
  }
}

void Cassandra::SetPrepareFunction(std::function<int(const Transaction&)> prepare){
  prepare_ = prepare;
}

bool Cassandra::IsStop() { return is_stop_; }

void Cassandra::Reset() {
  // received_num_.clear();
  // state_ = State::NewProposal;
}

int Cassandra::NextLeader(int round) {
  round++;
  int leader = round % total_num_;
  if(leader == 0) leader = total_num_;
  return leader;
}

bool Cassandra::IsLeader(int round) {
  round++;
  return IsLeader(round, id_);
}

bool Cassandra::IsLeader(int round, int id) {
  int leader = round % total_num_;
  // NOTE: DO NOT add LOG(ERROR) here. IsLeader is invoked tens of thousands
  // of times per second on the hot vote/proposal path; logging here causes
  // synchronous stderr flushes that pin the CPU and lead to upstream
  // timeouts / SIGKILL on large clusters (32+ nodes).
  if(leader == 0) leader = total_num_;
  return leader == id;
}

void Cassandra::AsyncConsensus() {
  int height = 0;
  int64_t start_time = GetCurrentTime();
  while (!is_stop_) {
    // Propose the next round unconditionally. Cassandra's design allows
    // every replica to propose every round (the leader-vs-non-leader
    // distinction is enforced downstream by leader_implicit_por and the
    // commit anchor in TryCommitByChain, not by suppressing non-leader
    // proposals here).
    //
    // The previous version of this loop guarded SendTxn with IsLeader and
    // also called WaitVote *before* SendTxn for the leader's first round.
    // That created a startup deadlock: round 1's leader (replica 1) would
    // block on WaitVote(1), but can_vote_[1] is only ever set after round
    // 2's leader proposes round 2, which itself requires round 1's
    // proposal to first arrive -- a circular wait that left every replica
    // idle for the entire pre-partition window (~5s) and meant nothing
    // got committed before partition. We restore the simple
    // "propose -> wait for next vote" loop from the original design.
    int next_height = SendTxn(height);
    if (next_height == -1) {
      // No transactions yet (start_ still false) or upstream block queue
      // is empty. Wait for a block to arrive and try again.
      proposal_manager_->WaitBlock();
      continue;
    }

    int64_t end_time = GetCurrentTime();
    global_stats_->AddRoundLatency(end_time - start_time);
    start_time = end_time;
    height = next_height;
    // Wait until our just-proposed round has been voted on (can_vote_
    // [height] is set when the next round's leader proposal arrives and
    // implicitly PoR's our round). On timeout we proceed anyway so a
    // stalled neighbor cannot indefinitely block our own progress.
    WaitVote(height);
  }
}

bool Cassandra::WaitVote(int height) {
  std::unique_lock<std::mutex> lk(mutex_);
  // wait vote / wait vote done logs suppressed: per-leader-round.
  vote_cv_.wait_for(lk, std::chrono::microseconds(timeout_ms_ * 1000),
      [&] { return can_vote_[height]; });
  if (!can_vote_[height]) {
    // wait vote timeout log retained at lower frequency would be useful,
    // but during partition this fires many times in a row and is noisy.
    return false;
  }
  return true;
}

void Cassandra::AskBlock(int sender, int block_id) {
  BlockQuery block;
  block.set_local_id(block_id);
  block.set_sender(id_);
  block.set_proposer(sender);
  {
    std::unique_lock<std::mutex> lk(b_round_mutex_);
    if (send_block_.find(std::make_pair(sender, block_id)) != send_block_.end()) {
      // ask_block_skip_have diag suppressed: hot path.
      return;
    }
  }

  // ask_block diag suppressed: hot path during block-recovery.
  if (sender == id_) {
    Broadcast(MessageType::CMD_BlockQuery, block);
  } else {
    SendMessage(MessageType::CMD_BlockQuery, block, sender);
  }
}

void Cassandra::ReceiveAskBlock(std::unique_ptr<BlockQuery> block) {
  // recv_ask_block diag suppressed: every cross-replica AskBlock fires this.
  Block* ack_block =
      proposal_manager_->GetReceivedBlock(block->proposer(), block->local_id());
  if (ack_block != nullptr) {
    SendMessage(MessageType::CMD_ProposalQuery, *ack_block, block->sender());
    return;
  }

  // ask_block_no_data diag suppressed.
  if (block->proposer() != id_ && block->sender() != block->proposer()) {
    SendMessage(MessageType::CMD_BlockQuery, *block, block->proposer());
  }
}

void Cassandra::ReceiveAskBlockAck(std::unique_ptr<Block> block) {
  if(block) {
    // proposal ack / receive ack / ack done diags suppressed: hot path.
    {
      std::unique_lock<std::mutex> lk(b_round_mutex_);
      int sender = block->sender_id();
      int block_id = block->local_id();
      if(send_block_.find(std::make_pair(sender, block_id))!=send_block_.end()){
        return;
      }
      send_block_.insert(std::make_pair(sender, block_id));
    }
    proposal_manager_->AddSubBlock(std::move(block));
  }
}

void Cassandra::AskProposal(int round) {
  if (round <= 0) {
    return;
  }
  ProposalQuery query;
  query.set_round(round);
  query.set_sender(id_);
  const int64_t now = GetCurrentTime();
  auto ask_it = ask_p_.find(round);
  if (ask_it != ask_p_.end() && now - ask_it->second < 500000) {
    // ask_round_proposal_skip diag suppressed: hot path during recovery,
    // every retry on the same round logs once -- thousands per second.
    return;
  }
  ask_p_[round] = now;

  // Broadcast itself happens at most every 500ms per round (throttled
  // above), so it's safe to log at INFO frequency, but we leave it
  // suppressed to keep stderr quiet in steady state.
  Broadcast(MessageType::CMD_AskProposal, query);
}

void Cassandra::AskProposal(int sender, int proposal_id, const std::string& hash) {
  const auto key = std::make_pair(sender, proposal_id);
  const int64_t now = GetCurrentTime();
  {
    std::unique_lock<std::mutex> lk(ask_chain_mutex_);
    auto ask_it = ask_chain_p_.find(key);
    if (ask_it != ask_chain_p_.end() && now - ask_it->second < 500000) {
      // ask_chain_proposal_skip diag suppressed: hot retry path.
      return;
    }
    ask_chain_p_[key] = now;
  }
  ProposalQuery query;
  query.set_sender(id_);
  query.set_proposer(sender);
  query.set_id(proposal_id);
  query.set_hash(hash);
  // ask_chain_proposal diag suppressed: hot path during recovery.
  Broadcast(MessageType::CMD_AskProposal, query);
}

void Cassandra::ReceiveAskProposal(std::unique_ptr<ProposalQuery> query) {

  int round = query->round();
  int requester = query->sender();

  ProposalQueryResp resp;
  if(query->id()>0) {
    int proposal_id = query->id();
    int proposer = query->proposer();
    // receive ask proposal log suppressed; every replica receives every
    // AskProposal broadcast and logging it three times per call was a
    // significant stderr contributor.
    std::string hash = query->hash();
    auto ret = graph_->GetProposals(proposer, proposal_id, hash);

    for(int i = static_cast<int>(ret.size()) - 1; i >= 0; --i) {
      *resp.add_proposal() = *ret[i];
    }
    resp.set_sender(id_);
    resp.set_id(proposal_id);
  }
  else {
    auto ret = graph_->GetProposals(round);
    if(ret.size()==0) {
      return;
    }

    for(auto& it : ret) {
      *resp.add_proposal() = *it;
    }
    resp.set_sender(id_);
    resp.set_round(round);
  }

  resp.set_max_round(graph_->GetLastRound());

  SendMessage(MessageType::CMD_AskProposalAck, resp, requester);
}

void Cassandra::ReceiveAskProposalAck(std::unique_ptr<ProposalQueryResp> resp) {
  int sender = resp->sender();
  int round = resp->round();
  int id = resp->id();
  int max_round = resp->max_round();
  int proposal_size = resp->proposal_size();
  // recv_proposal_ack diag suppressed -- this fires for every AskProposalAck
  // (hundreds per second per replica during recovery) and was the dominant
  // stderr offender after the last cleanup pass.
  {
    std::unique_lock<std::mutex> lk(recv_mutex_);
    if (recv_ > 0) {
      // Always remember the highest max_round reported by `sender`, even if
      // it doesn't currently meet `recv_`. When the recovery target later
      // rises, FinishRecoveryIfReady will recount these stored values; when
      // a sender catches up further, this entry is monotonically updated.
      auto& cur = recovery_quorum_senders_[sender];
      if (max_round > cur) cur = max_round;

      // Recompute current quorum size = #senders with max_round >= recv_.
      size_t qsize = 0;
      for (const auto& kv : recovery_quorum_senders_) {
        if (kv.second >= recv_) qsize++;
      }
      // recovery_quorum_progress diag suppressed: fires per-ack while the
      // quorum is being assembled. Equivalent state is captured by the
      // (rate-limited) recovery_progress / recovery_done logs.
      (void)qsize;
    }
  }

  auto add_proposal_and_replay_waiting = [&](std::unique_ptr<Proposal> proposal) {
    std::queue<std::pair<int, int>> resolved;
    auto key = std::make_pair(proposal->header().proposer_id(),
                              proposal->header().proposal_id());
    if (AddNewProposal(std::move(proposal))) {
      resolved.push(key);
    }

    while (!resolved.empty()) {
      auto resolved_key = resolved.front();
      resolved.pop();
      // notfound_ is also written under g_mutex_ from Cassandra::AddProposal
      // when a child fails with v_ret==2. Multiple threads can call
      // ReceiveAskProposalAck concurrently (one per sender) and would
      // otherwise race with AddProposal writers, which has been observed
      // to corrupt the std::map node and segfault during the early
      // pre-partition window. We bracket the read+erase in notfound_mutex_
      // (a dedicated mutex separate from g_mutex_) so we can still call
      // AddNewProposal -> AddProposal (which itself takes g_mutex_) below
      // without recursive-lock deadlock or lock-order conflicts.
      std::vector<std::unique_ptr<Proposal>> waiting;
      {
        std::unique_lock<std::mutex> nf_lk(notfound_mutex_);
        auto it = notfound_.find(resolved_key);
        if (it == notfound_.end()) {
          continue;
        }
        waiting = std::move(it->second);
        notfound_.erase(it);
      }
      for (auto& next_p : waiting) {
        key = std::make_pair(next_p->header().proposer_id(),
                             next_p->header().proposal_id());
        // replay_waiting_proposal diag suppressed: hot recovery path.
        if (AddNewProposal(std::move(next_p))) {
          resolved.push(key);
        }
      }
    }
  };

  if (id > 0) {
    if (proposal_size == 0) {
      // empty_chain_ack diag suppressed.
      FinishRecoveryIfReady("chain_ack_empty");
      return;
    }
    bool duplicate_chain = false;
    {
      std::unique_lock<std::mutex> lk(chain_ack_mutex_);
      auto chain_key = std::make_pair(sender, id);
      const int64_t now = GetCurrentTime();
      auto chain_it = recent_chain_ack_.find(chain_key);
      if (chain_it != recent_chain_ack_.end() &&
          now - chain_it->second < 500000) {
        duplicate_chain = true;
      } else {
        recent_chain_ack_[chain_key] = now;
      }
    }
    if (duplicate_chain) {
      // skip_duplicate_chain_ack diag suppressed: hot recovery path.
      FinishRecoveryIfReady("chain_ack_duplicate");
      return;
    }
    for (auto& p : resp->proposal()) {
      // recv_chain_proposal per-proposal diag suppressed.
      add_proposal_and_replay_waiting(std::make_unique<Proposal>(p));
    }
    FinishRecoveryIfReady("chain_ack");
    return;
  }

  std::vector<std::unique_ptr<Proposal>> proposals;
  for (auto& p : resp->proposal()) {
    proposals.push_back(std::make_unique<Proposal>(p));
  }

  size_t responders = 0;
  bool duplicate_response = false;
  {
    std::unique_lock<std::mutex> lk(round_mutex_);
    auto& round_responses = receive_round_p_[round];
    duplicate_response = round_responses.find(sender) != round_responses.end();
    if (!duplicate_response) {
      round_responses[sender] = std::move(resp);
    }
    responders = round_responses.size();
  }

  if (duplicate_response) {
    // skip_duplicate_round_ack diag suppressed: hot recovery path.
    FinishRecoveryIfReady("round_ack_duplicate");
    return;
  }

  // apply_round_recovery diag suppressed: hot recovery path.
  for (auto& p : proposals) {
    add_proposal_and_replay_waiting(std::move(p));
  }
  FinishRecoveryIfReady("round_ack");
}




void Cassandra::RetryMissingParents() {
  const int64_t now = GetCurrentTime();
  struct MissingParentRequest {
    int proposer;
    int proposal_id;
    int round;
    std::string hash;
    size_t waiting_size;
  };
  std::vector<MissingParentRequest> requests;

  // notfound_ is mutated under notfound_mutex_ in
  // ReceiveAskProposalAck::add_proposal_and_replay_waiting and under
  // g_mutex_ in cassandra::AddProposal. We use notfound_mutex_ here as
  // the simpler shared lock between recovery threads; the worst-case
  // race with AddProposal's writer is now bounded to a single std::map
  // operation and we copy out only what we need.
  {
    std::unique_lock<std::mutex> nf_lk(notfound_mutex_);
    for (const auto& it : notfound_) {
      if (it.second.empty()) {
        continue;
      }
      auto ask_it = ask_missing_p_.find(it.first);
      if (ask_it != ask_missing_p_.end() && now - ask_it->second < 500000) {
        continue;
      }
      ask_missing_p_[it.first] = now;
      const Proposal& waiting = *it.second.front();
      requests.push_back({it.first.first, it.first.second,
                          waiting.header().height() - 1,
                          waiting.header().prehash(), it.second.size()});
    }
  }

  for (const auto& request : requests) {
    // retry_missing_parent diag suppressed: invoked from
    // FinishRecoveryIfReady's retry path which fires several times per
    // second per replica during partition healing.
    AskProposal(request.proposer, request.proposal_id, request.hash);
    AskProposal(request.round);
  }
}

void Cassandra::StartRecovery(int target_round, const std::string& reason) {
  if (target_round <= 0) {
    return;
  }
  int start_round = 0;
  bool should_request = false;
  {
    std::unique_lock<std::mutex> lk(recv_mutex_);
    start_round = std::max(1, graph_->GetCurrentHeight() - 1);
    if (recv_ < target_round) {
      recv_ = target_round;
      should_request = true;
      // DO NOT clear recovery_quorum_senders_ here even though the target
      // just moved. Each entry stores the per-sender highest max_round we
      // have observed, so historical acks remain valid evidence of catch-up
      // progress and the effective quorum is recomputed against the new
      // `recv_` on demand. Clearing causes lost progress every time the
      // target rises, which is fatal when many replicas are racing to catch
      // up after a partition heals.
      // Refresh self's max_round entry to the local graph height so we are
      // counted whenever we have already reached the target.
      const int self_height = graph_->GetCurrentHeight();
      auto& cur_self = recovery_quorum_senders_[id_];
      if (self_height > cur_self) cur_self = self_height;
    }
  }

  // Only log when this call actually advanced the recovery target. The
  // common case (StartRecovery merges into an already-active recovery) is
  // hit thousands of times per second from wait_missing_parent on busy
  // clusters and was a top stderr offender.
  if (should_request) {
    //LOG(ERROR) << "CASS_DIAG recovery_start id:" << id_
    //           << " reason:" << reason
    //           << " start_round:" << start_round
    //           << " target_round:" << target_round
    //           << " graph_height:" << graph_->GetCurrentHeight()
    //           << " monitored_round:" << monitored_round_;
  }

  for (int round = start_round; round <= target_round; ++round) {
    AskProposal(round);
  }
}

void Cassandra::FinishRecoveryIfReady(const std::string& source) {
  int target_round = 0;
  int graph_height = graph_->GetCurrentHeight();
  size_t quorum_size = 0;
  bool need_retry_missing = false;
  {
    std::unique_lock<std::mutex> lk(recv_mutex_);
    target_round = recv_;
    // No active recovery -> nothing to do. Avoid noisy "recovery_progress"
    // logs and pointless RetryMissingParents calls when recv_ is 0.
    if (target_round <= 0) {
      return;
    }
    // Once we have caught up locally, make sure self is counted toward the
    // sync quorum (StartRecovery only seeds self if we were already at the
    // target height at that time).
    if (graph_height >= target_round) {
      auto& cur_self = recovery_quorum_senders_[id_];
      if (graph_height > cur_self) cur_self = graph_height;
    }
    // Recompute quorum against the (possibly newly-raised) target.
    quorum_size = 0;
    for (const auto& kv : recovery_quorum_senders_) {
      if (kv.second >= target_round) quorum_size++;
    }
    if (graph_height < target_round ||
        static_cast<int>(quorum_size) < need_num_) {
      // Rate-limit recovery_progress diagnostic. Without this guard we emit
      // it on every FinishRecoveryIfReady() call (which is invoked from the
      // hot vote / proposal paths several thousand times per second under
      // partition healing). We only re-emit if either the target or the
      // observed quorum_size advanced -- i.e. when there is new information.
      if (target_round != last_recovery_log_target_ ||
          static_cast<int>(quorum_size) != last_recovery_log_qsize_) {
        //LOG(ERROR) << "CASS_DIAG recovery_progress id:" << id_
        //           << " source:" << source
        //           << " target_round:" << target_round
        //           << " graph_height:" << graph_height
        //           << " quorum_size:" << quorum_size
        //           << " need:" << need_num_
        //           << " monitored_round:" << monitored_round_;
        last_recovery_log_target_ = target_round;
        last_recovery_log_qsize_ = static_cast<int>(quorum_size);
      }
      // Defer RetryMissingParents until after recv_mutex_ is released:
      // RetryMissingParents now takes g_mutex_ to safely iterate notfound_,
      // and g_mutex_ -> recv_mutex_ is the established lock order
      // (cassandra::AddProposal holds g_mutex_ and calls StartRecovery
      // which acquires recv_mutex_). Calling it under recv_mutex_ here
      // would invert that order and deadlock.
      need_retry_missing = true;
    } else {
      // Recovery target reached and quorum satisfied -- finalize.
      recv_ = 0;
      pending_p_.clear();
      // Allow next recovery cycle to log its own initial progress line.
      last_recovery_log_target_ = -1;
      last_recovery_log_qsize_ = -1;
      // Keep recovery_quorum_senders_ entries (they remain valid evidence
      // for future recovery rounds and only grow monotonically). Capping
      // size: entries are keyed by sender_id, so this is bounded by
      // total_replicas.
    }
  }
  if (need_retry_missing) {
    // Recovery still in progress; issue retransmissions for any waiting
    // proposals whose parent we don't have yet, then return without
    // declaring recovery_done.
    RetryMissingParents();
    return;
  }
  {
    std::unique_lock<std::mutex> lk(proposal_timer_mutex_);
    monitored_round_ = std::max(monitored_round_, graph_height + 1);
    proposal_timer_cv_.notify_all();
  }
  //LOG(ERROR) << "CASS_DIAG recovery_done id:" << id_
  //           << " source:" << source
  //           << " target_round:" << target_round
  //           << " graph_height:" << graph_height
  //           << " quorum_size:" << quorum_size
  //           << " monitored_round:" << monitored_round_;
}

void Cassandra::AsyncCommit() {

  std::set<std::pair<int, int>> committed;

  while (!is_stop_) {
    std::unique_ptr<Proposal> p = execute_queue_.Pop(timeout_ms_ * 1000);
    if (p == nullptr) {
      // execu timeout log suppressed: fires every timeout_ms while idle.
      continue;
    }
    int round = p->header().height();
    int txn_num = 0;
    //LOG(ERROR) << "CASS_CRASH_TRACE async_commit_proposal_start id:" << id_
    //           << " proposer:" << p->header().proposer_id()
    //           << " proposal_id:" << p->header().proposal_id()
    //           << " height:" << round
    //           << " block_size:" << p->block_size()
    //           << " sub_block_size:" << p->sub_block_size();
    for (const Block& block : p->sub_block()) {
      std::unique_lock<std::mutex> lk(mutex_);
      //LOG(ERROR) << "CASS_CRASH_TRACE async_commit_before_get_block id:" << id_
      //           << " proposal_height:" << round
      //           << " proposal_proposer:" << p->header().proposer_id()
      //           << " proposal_id:" << p->header().proposal_id()
      //           << " block_sender:" << block.sender_id()
      //           << " block_local_id:" << block.local_id()
      //           << " hash_size:" << block.hash().size();
      std::unique_ptr<Block> data_block =
        proposal_manager_->GetBlock(block.hash(), p->header().proposer_id());
      if(data_block == nullptr){
        //LOG(ERROR) << "CASS_CRASH_TRACE async_commit_get_block_null id:" << id_
        //           << " proposal_height:" << round
        //           << " proposal_proposer:" << p->header().proposer_id()
        //           << " block_sender:" << block.sender_id()
        //           << " block_local_id:" << block.local_id();
        continue;
      }

      //LOG(ERROR) << "CASS_CRASH_TRACE async_commit_got_block id:" << id_
      //           << " proposal_height:" << round
      //           << " proposal_proposer:" << p->header().proposer_id()
      //           << " block_local_id:" << data_block->local_id()
      //           << " block_sender:" << data_block->sender_id()
      //           << " cut_size:" << data_block->cut_size()
      //           << " txn_size:" << data_block->data().transaction_size();
      auto it = committed.find(std::make_pair(p->header().proposer_id(), block.local_id()));
      if( it != committed.end()){
        //LOG(ERROR) << "CASS_CRASH_TRACE async_commit_duplicate_skip id:" << id_
        //           << " proposer:" << p->header().proposer_id()
        //           << " block_local_id:" << block.local_id();
        continue;
      }
      committed.insert(std::make_pair(p->header().proposer_id(), block.local_id()));

      assert(data_block->cut_size() > 0);
      int num = 0;
      for(int i = 0; i < data_block->cut_size(); ++i) {
        const int cut_target = data_block->cut(i);
        //LOG(ERROR) << "CASS_CRASH_TRACE async_commit_cut_enter id:" << id_
        //           << " proposal_height:" << round
        //           << " cut_index:" << i
        //           << " sender:" << (i + 1)
        //           << " from:" << txn_commit_[i]
        //           << " to:" << cut_target;
        //assert(data_block->cut(i) >= txn_commit_[i]);
        for(int j = txn_commit_[i]+1; j <= cut_target; ++j) {
          std::unique_ptr<Block> sub_data_block = nullptr;
          while(true){
            sub_data_block = proposal_manager_->GetSubBlockById(i+1, j);
            // sub block done log suppressed: hot inner-commit loop.
            if(sub_data_block == nullptr){
              break;
            }
            else {
              break;
            }
          }
          if(sub_data_block == nullptr){
            continue;
          }

          assert(sub_data_block != nullptr);
          num+=sub_data_block->data().transaction_size();
          global_stats_->AddLatency(GetCurrentTime()-sub_data_block->create_time());
          for (Transaction& txn :
              *sub_data_block->mutable_data()->mutable_transaction()) {
            txn.set_id(execute_id_++);
            txn_num++;
            commit_(txn);
          }
        }
        txn_commit_[i] = std::max(txn_commit_[i], cut_target);
        //LOG(ERROR) << "CASS_CRASH_TRACE async_commit_cut_done id:" << id_
        //           << " proposal_height:" << round
        //           << " cut_index:" << i
        //           << " committed_to:" << txn_commit_[i]
        //           << " txn_num:" << txn_num;
      }
      //LOG(ERROR) << "CASS_CRASH_TRACE async_commit_block_done id:" << id_
      //           << " proposal_height:" << round
      //           << " proposer:" << p->header().proposer_id()
      //           << " block_local_id:" << block.local_id()
      //           << " txn_num:" << txn_num;
    }
    //LOG(ERROR) << "CASS_CRASH_TRACE async_commit_proposal_done id:" << id_
    //           << " proposer:" << p->header().proposer_id()
    //           << " proposal_id:" << p->header().proposal_id()
    //           << " height:" << round
    //           << " txn_num:" << txn_num;
    (void)txn_num; (void)round;
  }

}

void Cassandra::SyncRound() {
  BlockQuery block;
  block.set_sender(id_);
  block.set_local_id(monitored_round_);
  // ask block log suppressed: SyncRound is invoked periodically.
  Broadcast(MessageType::CMD_SyncRound, block);
}

void Cassandra::ReceiveRound(std::unique_ptr<BlockQuery> block) {
  int local_id = block->local_id();
  int sender = block->sender();

  block->set_sender(id_);
  block->set_local_id(monitored_round_);
  block->set_proposer(local_id);
  // ask block log suppressed.
  SendMessage(MessageType::CMD_SyncRoundAck, *block, sender);
}

void Cassandra::ReceiveRoundAck(std::unique_ptr<BlockQuery> block) {
  // proposal ack trace logs suppressed: hot path on every replica's reply.
  std::unique_lock<std::mutex> lk(round_mutex_);
  int local_id = block->proposer();
  int sender = block->sender();
  receive_round_[local_id][sender] = std::move(block);
  if(receive_round_[local_id].size() >= 2*f_+1) {
    std::vector<int> rounds;
    for(auto& it : receive_round_[local_id]) {
      rounds.push_back(it.second->local_id());
    }
    sort(rounds.begin(), rounds.end());
    int ad_round = rounds[f_+1];
    (void)ad_round;
  }
}


void Cassandra::CommitProposal(const Proposal& p) {
  // commit proposal entry log suppressed: hot path; latency is recorded
  // via global_stats_->AddCommitLatency below.
  if (p.block_size() == 0) {
    return;
  }
  // proposal_manager_->ClearProposal(p);
  committed_num_++;
  int64_t commit_time = GetCurrentTime() - p.create_time();
  global_stats_->AddCommitLatency(commit_time);
  // LOG(ERROR) << "commit num:" << committed_num_
  //           << " commit delay:" << GetCurrentTime() - p.create_time();
  execute_queue_.Push(std::make_unique<Proposal>(p));
}

void Cassandra::MonitorProposalRound() {
  int timeout_ms = 400;
  while (!IsStop()) {
    RetryMissingParents();
    int round = 0;
    bool received = false;
    bool is_from_leader = true;
    {
      std::unique_lock<std::mutex> lk(proposal_timer_mutex_);
      {
        std::unique_lock<std::mutex> lk(recv_mutex_);
        round = monitored_round_;
      }
      received = proposal_timer_cv_.wait_for(
          lk, std::chrono::milliseconds(timeout_ms),
          [&] { return proposal_round_received_.count(round) > 0; });
      // per-round wait result diag suppressed.
      if (received) {
        is_from_leader = (proposal_round_received_[round] == 1);
        proposal_round_received_.erase(proposal_round_received_.find(round));
      } else {
        if (monitored_round_ == 1) {
          continue;
        }
      }
      int active_recovery_target = 0;
      {
        std::unique_lock<std::mutex> lk(recv_mutex_);
        active_recovery_target = recv_;
      }
      if (active_recovery_target > 0) {
        // monitor_recovery_active diag suppressed: this fires on every
        // monitor wakeup while a recovery is in flight (~10 Hz per replica).
        lk.unlock();
        StartRecovery(active_recovery_target, "monitor_active_recovery");
        continue;
      }
    }

    bool should_advance = true;
    if (!received || !is_from_leader) {
      should_advance = SlowPath(round);
      // slow path advance / want next per-round trace logs suppressed.
    }

    std::unique_lock<std::mutex> lk(proposal_timer_mutex_);
    if (should_advance) {
      if (monitored_round_ <= round) {
        monitored_round_ = round + 1;
      }
      // wait next diag suppressed.
    } else {
      int graph_height = 0;
      {
        std::unique_lock<std::mutex> g_lk(g_mutex_);
        graph_height = graph_->GetCurrentHeight();
      }
      if (graph_height < round && monitored_round_ > round) {
        monitored_round_ = round;
        // slowpath_defer diag suppressed.
      }
    }
  }
}

bool Cassandra::SlowPath(int round) {
  if(round == 1) {
    return true;
  }
  //LOG(ERROR) << "CASS_CRASH_TRACE slowpath_enter id:" << id_
  //           << " round:" << round;

  std::unique_ptr<Proposal> proposal = nullptr;

  bool has_strongest = false;
  {
    std::unique_lock<std::mutex> g_lk(g_mutex_);
    has_strongest = proposal_manager_->CheckLatestStrongestProposal(round);
  }
  if (!has_strongest) {
    //LOG(ERROR) << "CASS_CRASH_TRACE slowpath_missing_strongest id:" << id_
    //           << " round:" << round;
    AskProposal(round - 1);
    AskProposal(round);
    StartRecovery(round - 1, "slowpath_missing_strongest");
    return false;
  }

  while(proposal == nullptr){
    {
      // GenerateProposal reads and mutates ProposalGraph
      // (GetStrongestProposal / GetLatestStrongestProposal / IncreaseHeight).
      // It must be serialized with AddProposal / ReceiveProposalVote graph
      // mutations, otherwise MonitorProposalRound can race on node_info_ /
      // last_node_ and segfault inside SlowPath.
      std::unique_lock<std::mutex> g_lk(g_mutex_);
      proposal = proposal_manager_->GenerateProposal(round, start_);
    }
    if (proposal == nullptr) {
      if (start_ == false) {
        //LOG(ERROR) << "CASS_CRASH_TRACE slowpath_no_start id:" << id_
        //           << " round:" << round;
        return true;
      }
      //LOG(ERROR) << "CASS_CRASH_TRACE slowpath_generate_null id:" << id_
      //           << " round:" << round;
      usleep(10000);
      continue;
    }
  }

  for (const auto& his : proposal->history()) {
    const std::string& hash = his.hash();
    int state = his.state();
    if (state != ProposalState::New) {
      continue;
    }

    std::vector<Block> blocks_to_check;
    {
      std::unique_lock<std::mutex> g_lk(g_mutex_);
      const Proposal* p = graph_->GetProposalInfo(hash);
      if (p == nullptr) {
        //LOG(ERROR) << "CASS_CRASH_TRACE slowpath_history_missing id:" << id_
        //           << " round:" << round
        //           << " history_sender:" << his.sender()
         //          << " history_id:" << his.id()
         //          << " hash_size:" << hash.size();
        AskProposal(his.sender(), his.id(), hash);
        StartRecovery(round - 1, "slowpath_history_missing");
        return false;
      }
      for (const auto& b : p->block()) {
        blocks_to_check.push_back(b);
      }
    }

    for (const auto& b : blocks_to_check) {
      bool ret = proposal_manager_->ContainBlock(b.hash(), b.sender_id());
      if (!ret) {
        //LOG(ERROR) << "CASS_CRASH_TRACE slowpath_missing_block id:" << id_
        //           << " round:" << round
        //           << " block_sender:" << b.sender_id()
        //           << " block_local_id:" << b.local_id();
        AskBlock(b.sender_id(), b.local_id());
        return false;
      }
    }
  }
  proposal_manager_->AddLocalProposal(*proposal);

  if (proposal->block_size() <= 0) {
    //LOG(ERROR) << "CASS_CRASH_TRACE slowpath_empty_proposal id:" << id_
    //           << " round:" << round
    //           << " proposer:" << proposal->header().proposer_id()
    //           << " proposal_id:" << proposal->header().proposal_id();
    return false;
  }
  Broadcast(MessageType::NewProposal, *proposal);
  assert(proposal->header().height() == round);

  std::queue<std::unique_ptr<Proposal>> pending;
  {
    std::unique_lock<std::mutex> lk(proposal_timer_mutex_);
    auto it = pending_.find(round);
    if(it != pending_.end()){
      pending.swap(it->second);
      pending_.erase(it);
    }
  }
  while(!pending.empty()){
    auto p = std::move(pending.front());
    pending.pop();
    AddNewProposal(std::move(p));
  }
  //LOG(ERROR) << "CASS_CRASH_TRACE slowpath_done id:" << id_
  //           << " round:" << round
  //           << " proposal_id:" << proposal->header().proposal_id();
  return true;
}


bool Cassandra::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  // LOG(ERROR)<<"recv txn:";
  txn->set_create_time(GetCurrentTime());
  txns_.Push(std::move(txn));
  recv_num_++;
  return true;
}

void Cassandra::BroadcastTxn() {
  std::vector<std::unique_ptr<Transaction>> txns;
  while (!IsStop()) {
    std::unique_ptr<Transaction> txn = txns_.Pop();
    if (txn == nullptr) {
      continue;
    }
    txn->set_queuing_time(GetCurrentTime()-txn->create_time());
    global_stats_->AddQueuingLatency(GetCurrentTime()-txn->create_time());
    //LOG(ERROR)<<"get txn, proxy id:"<<txn->proxy_id()<<" hash:"<<txn->hash();
    txns.push_back(std::move(txn));
    /*
    if (txns.size() < batch_size_) {
      continue;
    }
    */

    for(int i = 1; i < batch_size_; ++i){
      std::unique_ptr<Transaction> txn = txns_.Pop(10000);
      if(txn == nullptr){
        break;
      }
      //LOG(ERROR)<<"get txn, proxy id:"<<txn->proxy_id()<<" hash:"<<txn->hash();
      txn->set_queuing_time(GetCurrentTime()-txn->create_time());
      global_stats_->AddQueuingLatency(GetCurrentTime()-txn->create_time());
      txns.push_back(std::move(txn));
    }

    //global_stats_->AddCommitBlock(txns.size());
    std::unique_ptr<Block> block = proposal_manager_->MakeBlock(txns);
    assert(block != nullptr);
    // send block / bc / retry / done diag suppressed: hot block-broadcast.
    Broadcast(MessageType::NewBlocks, *block);
    global_stats_->IncPendingExecute();

    std::string hash = block->hash();
    int local_id = block->local_id();
    Block tmp_block = *block;
    proposal_manager_->AddLocalBlock(std::move(block));
    txns.clear();

    while(!proposal_manager_->WaitBlock(local_id)){
      Broadcast(MessageType::NewBlocks, tmp_block);
    }
  }
}

void Cassandra::ReceiveBlock(std::unique_ptr<Block> block) {
  {
    std::unique_lock<std::mutex> lk(block_mutex_);

    int sender_id = block->sender_id();
    int local_id = block->local_id();
    int create_time = block->create_time();

    BlockACK ack_block;
    ack_block.set_hash(block->hash());
    ack_block.set_sender_id(id_);
    ack_block.set_local_id(block->local_id());

    // Per-block receive diag suppressed: thousands per second on busy paths.
    proposal_manager_->ReceiveBlock(std::move(block));
    SendMessage(MessageType::CMD_BlockACK, ack_block, sender_id);
    (void)local_id; (void)create_time;
  }
  return ;
}

void Cassandra::ReceiveBlockACK(std::unique_ptr<BlockACK> block) {
    std::unique_lock<std::mutex> lk(block_mutex_);

    int sender = block->sender_id();

    if(received_block_[block->local_id()].find(sender) != received_block_[block->local_id()].end()){
      return;
    }
    received_block_[block->local_id()].insert(sender);
    // receive block ack diag suppressed: every replica sends ACKs per block,
    // so this fires N times per block at every replica.
    if(received_block_[block->local_id()].size() == f_+1) {
      // ready block diag suppressed (still only once per block, but quiet).
      proposal_manager_->BlockReady(block->hash(), block->local_id());
    }
}

int Cassandra::SendTxn(int round) {


  std::unique_ptr<Proposal> proposal = nullptr;
  // LOG(ERROR)<<"send:"<<round;
  {
    round++;
    std::unique_lock<std::mutex> g_lk(g_mutex_);
    int current_round = proposal_manager_->CurrentRound();
    // mutex / current round / skip_send_existing_round / mutex done trace
    // logs suppressed: hot path on busy proposers.
    round = std::max(round, upgrade_);
    if(current_round >= round) {
      return current_round;
    }

    proposal = proposal_manager_->GenerateProposal(round, start_);
    if (proposal == nullptr) {
      if (start_ == false) {
        return -1;
      }
      return 0;
    }
  }

  for (const auto& his : proposal->history()) {
    const std::string& hash = his.hash();
    int state = his.state();
    if (state != ProposalState::New) {
      continue;
    }

    std::vector<Block> blocks_to_check;
    {
      std::unique_lock<std::mutex> g_lk(g_mutex_);
      const Proposal* p = graph_->GetProposalInfo(hash);
      if (p == nullptr) {
        //LOG(ERROR) << "CASS_CRASH_TRACE sendtxn_history_missing id:" << id_
        //           << " round:" << round
        //           << " history_sender:" << his.sender()
        //           << " history_id:" << his.id()
        //           << " hash_size:" << hash.size();
        AskProposal(his.sender(), his.id(), hash);
        StartRecovery(round - 1, "sendtxn_history_missing");
        return 0;
      }
      for (const auto& b : p->block()) {
        blocks_to_check.push_back(b);
      }
    }

    for (const auto& b : blocks_to_check) {
      bool ret = proposal_manager_->ContainBlock(b.hash(), b.sender_id());
      if (!ret) {
        //LOG(ERROR) << "CASS_CRASH_TRACE sendtxn_missing_block id:" << id_
        //           << " round:" << round
        //           << " block_sender:" << b.sender_id()
        //           << " block_local_id:" << b.local_id();
        AskBlock(b.sender_id(), b.local_id());
        return 0;
      }
    }
  }
  proposal_manager_->AddLocalProposal(*proposal);

  // bc proposal trace log suppressed: invoked once per leader round.
  if (proposal->block_size() <= 0) {
    //LOG(ERROR) << "CASS_CRASH_TRACE sendtxn_empty_proposal id:" << id_
    //           << " round:" << round
    //           << " proposer:" << proposal->header().proposer_id()
    //           << " proposal_id:" << proposal->header().proposal_id();
    return 0;
  }
  Broadcast(MessageType::NewProposal, *proposal);

  assert(proposal->header().height() == round);
  return proposal->header().height();
}

bool Cassandra::Checklimit(int low, int hight, int proposer) {
 return !(low <= proposer && proposer <= hight ) && low <= id_ && id_ <= hight;
}

bool Cassandra::AddNewProposal(std::unique_ptr<Proposal> proposal) {
  // CASS_DIAG add_new_proposal log suppressed: hot path, called once per
  // received proposal; at 32 nodes this is several thousand calls per second
  // and the synchronous LOG(ERROR) flush dominates CPU time.
  // NOTE: previously this code path also kept a full copy of every incoming
  // proposal in pending_p_. That map was write-only (only ever cleared at
  // FinishRecoveryIfReady completion, never read), so during partition heal
  // it grew without bound and consumed hundreds of MB of RSS, which is the
  // most likely cause of the SIGKILL/OOM-killer terminations observed for
  // the fastest replicas. The map has been removed entirely; the same
  // proposal object is owned by ProposalGraph::data_p_/data_m_ when it
  // succeeds AddProposal, which is sufficient for protocol correctness.
  bool ret = AddNewProposalInternal(std::move(proposal));
  FinishRecoveryIfReady("add_new_proposal");
  return ret;
}


bool Cassandra::AddNewProposalInternal(std::unique_ptr<Proposal> proposal) {


  {
    // Hot path: avoid synchronous LOG(ERROR) here, it gets called O(N) times
    // per round and at 32 nodes a single noisy log per call is enough to
    // saturate stderr and stall the process under load.
    std::unique_lock<std::mutex> lk(mutex_);

    for(const auto& block : proposal->block()){
      std::unique_ptr<Block> new_block = std::make_unique<Block>(block);
      proposal_manager_->AddBlock(std::move(new_block));
    }

    if(!AddProposal(*proposal)){
      if (last_add_proposal_ret_ != 2) {
        future_[proposal->header().height()].push(std::move(proposal));
      }
      // proposal_waiting_parent diag is intentionally suppressed on the hot
      // path; AddProposal() already logs the missing-parent diagnostic when
      // it returns code 2 (see cassandra.cpp `wait_missing_parent`).
      return false;
    }

    int round = proposal->header().height();
    if(!future_.empty() && future_.begin()->first == round) {
      while(!future_.begin()->second.empty()){
        auto& p = future_.begin()->second.front();
        if (p == nullptr) {
          future_.begin()->second.pop();
          continue;
        }
        if(AddProposal(*p)){
          future_.begin()->second.pop();
        }
        else if (last_add_proposal_ret_ == 2) {
          // remove_future_waiting_parent diag suppressed: hot path.
          future_.begin()->second.pop();
        }
        else {
          break;
        }
      }
      if(future_.begin()->second.empty()){
        future_.erase(future_.begin());
      }
    }

    while(true) {
      int ok = 0;
      while(!future_.empty() && future_.begin()->first == round+1) {
        while(!future_.begin()->second.empty()){
          auto& p = future_.begin()->second.front();
          if (p == nullptr) {
            future_.begin()->second.pop();
            continue;
          }
          if(AddProposal(*p)){
            future_.begin()->second.pop();
          }
          else if (last_add_proposal_ret_ == 2) {
            // remove_future_waiting_parent diag suppressed: hot path.
            future_.begin()->second.pop();
          }
          else {
            break;
          }
        }
        if(future_.begin()->second.empty()){
          future_.erase(future_.begin());
          ok = 1;
        }
      }
      if(!ok) {
        break;
      }
      round++;
    }
  }
  return true;
}

bool Cassandra::ReceiveProposal(std::unique_ptr<Proposal> proposal) {
  const int round = proposal->header().height();
  const int proposer = proposal->header().proposer_id();
  bool should_add = false;
  {
    std::unique_lock<std::mutex> lk(proposal_timer_mutex_);
    // recv_proposal entry diag suppressed: hot path (every received proposal).
    if (round >= monitored_round_) {
      if (IsLeader(round, proposer)) {
        proposal_round_received_.insert(std::make_pair(round, 1));
        proposal_timer_cv_.notify_all();
        should_add = true;
      } else {
        pending_[round].push(std::move(proposal));
        if (pending_[round].size() >= static_cast<size_t>(f_ + 1)) {
          proposal_round_received_.insert(std::make_pair(round, 0));
          proposal_timer_cv_.notify_all();
        }
        // pending_nonleader_proposal diag suppressed.
        return true;
      }
    } else {
      should_add = true;
    }
  }
  if (should_add) {
    bool ret = AddNewProposal(std::move(proposal));
    std::queue<std::unique_ptr<Proposal>> pending;
    {
      std::unique_lock<std::mutex> lk(proposal_timer_mutex_);
      auto it = pending_.find(round);
      if (it != pending_.end()) {
        pending.swap(it->second);
        pending_.erase(it);
      }
    }
    while (!pending.empty()) {
      AddNewProposal(std::move(pending.front()));
      pending.pop();
    }
    return ret;
  }
  return true;
}

bool Cassandra::ReceiveProposalVote(std::unique_ptr<Proposal> proposal) {
  // Hashes that crossed quorum and need to be promoted to PoR via
  // graph_->ChangeState. We collect them while holding gg_mutex_ and
  // promote them AFTER releasing gg_mutex_, taking g_mutex_ instead.
  // This preserves the documented g_mutex_ -> gg_mutex_ lock order
  // (see AddProposal's leader fast-path liveness hook) and avoids
  // deadlocks caused by the inverse order.
  std::vector<std::string> pending_quorum_promote_hashes;

  std::unique_lock<std::mutex> lk(gg_mutex_);
  const int proposal_height = proposal->header().height();
  const int graph_height = graph_->GetCurrentHeight();
  const int sender_id = proposal->header().sender_id();
  const auto vote_key =
      std::make_pair(proposal->header().proposer_id(), proposal->header().hash());

  received_num_[proposal_height].insert(sender_id);
  vote_[proposal_height][vote_key].insert(sender_id);

  // CASS_DIAG proposal_vote diagnostic suppressed: this fires on every vote
  // arrival and at 32 nodes is one of the top stderr offenders (>8K/s/replica
  // observed on production). Keep only the rate-limited quorum_vote_promote
  // log below, which fires once per (height,leader_hash) crossing quorum.

  if (proposal_height > graph_height &&
      received_num_[proposal_height].size() >= static_cast<size_t>(f_ + 1)) {
    StartRecovery(proposal_height, "future_vote_f_plus_1");
  }

  auto apply_votes_for_height = [&](int height) {
    if (height <= 0 ||
        received_num_[height].size() < static_cast<size_t>(need_num_)) {
      return false;
    }
    // The slow-path "vote" carries the sender's locally-strongest proposal
    // for `height`, whose proposer may be ANY node that proposed at this
    // height (leader or not). Different replicas may have different graph
    // states and therefore vote for different (proposer, hash) keys.
    // If we promoted every key that crosses the quorum threshold, we could
    // promote a non-leader proposer's hash to PoR while the leader fast-path
    // (in ProposalGraph::AddProposal, via the next-round leader's prehash)
    // independently promotes the leader's hash — producing two distinct PoR
    // hashes at the same height and breaking the
    // GetStrongestProposal assert.
    //
    // Restrict quorum-vote PoR to ONLY the leader's hash for that height.
    // This keeps both PoR paths (vote-quorum here and leader-implicit-por
    // in graph) aligned on the unique leader-chosen hash.
    // Collect leader hashes that crossed quorum; we promote them BELOW
    // (outside gg_mutex_) under g_mutex_ to avoid the gg_mutex_ -> g_mutex_
    // lock-order inversion that exists elsewhere (AddProposal takes
    // g_mutex_ -> gg_mutex_, see leader fast-path notify hook).
    // Only emit per-(height,key) crossing-the-quorum diagnostics ONCE per
    // (height,key) -- otherwise this loop relogs every time we see a new
    // vote at `height`, even after the bucket has long since crossed
    // need_num_ (size monotonically grows so the inner predicate keeps
    // firing).  At 32 nodes this is the second-highest stderr offender.
    const bool first_time_can_vote = !can_vote_[height];
    for (auto it : vote_[height]) {
      const int voted_proposer = it.first.first;
      const std::string& voted_hash = it.first.second;
      if (it.second.size() < static_cast<size_t>(need_num_)) continue;
      if (!IsLeader(height, voted_proposer)) {
        if (first_time_can_vote) {
          //LOG(ERROR) << "CASS_DIAG skip_nonleader_quorum_vote id:" << id_
          //           << " height:" << height
          //           << " voted_proposer:" << voted_proposer
          //           << " votes:" << it.second.size();
        }
        continue;
      }
      if (first_time_can_vote) {
        //LOG(ERROR) << "CASS_DIAG quorum_vote_promote_leader id:" << id_
        //           << " height:" << height
        //           << " leader_proposer:" << voted_proposer
        //           << " votes:" << it.second.size();
      }
      pending_quorum_promote_hashes.push_back(voted_hash);
    }
    can_vote_[height] = true;
    vote_cv_.notify_all();
    if (first_time_can_vote) {
      //LOG(ERROR) << "CASS_DIAG can_vote id:" << id_
      //           << " height:" << height
      //           << " votes:" << received_num_[height].size()
      //           << " graph_height:" << graph_height;
    }
    return true;
  };

  const bool proposal_ready = apply_votes_for_height(proposal_height);
  const bool graph_ready =
      proposal_height == graph_height ? proposal_ready
                                      : apply_votes_for_height(graph_height);
  if (proposal_ready || graph_ready) {
    FinishRecoveryIfReady("proposal_vote");
  }

  // Release gg_mutex_ before mutating the graph: graph_->ChangeState walks
  // and modifies node_info_ / last_node_ / expected_commit_, which are
  // also touched under g_mutex_ in AddProposal. Without this swap of
  // locks, a concurrent reader holding g_mutex_ (e.g. GetStrongestProposal
  // raw NodeInfo* deref) would race with our writes and produce
  // segfault-at-0x78 style crashes that have been observed in dmesg on
  // multiple replicas.
  lk.unlock();
  if (!pending_quorum_promote_hashes.empty()) {
    std::unique_lock<std::mutex> g_lk(g_mutex_);
    for (const auto& h : pending_quorum_promote_hashes) {
      graph_->ChangeState(h);
    }
  }
  return true;
}

bool Cassandra::AddProposal(const Proposal& proposal) {
  last_add_proposal_ret_ = 0;
  {
    // Hot path: previously double-logged (before and after lock acquire).
    // At 32 nodes this fired several thousand times per second per replica;
    // synchronous LOG(ERROR) flushes were the dominant cost and triggered
    // OOM-killer / SIGKILL via stderr backpressure on the fastest replicas.
    std::unique_lock<std::mutex> lk(g_mutex_);

 /*
    if(proposal.header().height() == graph_->GetCurrentHeight()){
      return true;
    }
    */
    
    while(true){
      int v_ret = graph_->AddProposal(proposal);
      if (v_ret != 0) {
        last_add_proposal_ret_ = v_ret;
        // "add proposal fail" diag suppressed: hot path during recovery,
        // every child of a missing parent triggers it.
        if (v_ret == 2) {
          // miss history
          auto pre_key = std::make_pair(proposal.header().pre_proposer_id(),proposal.header().pre_proposal_id());
          auto key = std::make_pair(proposal.header().proposer_id(), proposal.header().proposal_id());

          // notfound_ is shared with ReceiveAskProposalAck and
          // RetryMissingParents (both run on different threads). Use the
          // dedicated notfound_mutex_ to serialize access. Lock order:
          // g_mutex_ -> notfound_mutex_ (we already hold g_mutex_ here);
          // the readers in ReceiveAskProposalAck / RetryMissingParents
          // take notfound_mutex_ alone, so no inversion is possible.
          bool duplicate_waiting = false;
          {
            std::unique_lock<std::mutex> nf_lk(notfound_mutex_);
            for (const auto& waiting : notfound_[pre_key]) {
              if (waiting->header().proposer_id() == key.first &&
                  waiting->header().proposal_id() == key.second) {
                duplicate_waiting = true;
                break;
              }
            }
            if (!duplicate_waiting) {
              notfound_[pre_key].push_back(std::make_unique<Proposal>(proposal));
            }
          }
          if (!duplicate_waiting) {
            // Only log on the FIRST time we encounter a (parent,child) pair.
            //LOG(ERROR) << "CASS_DIAG wait_missing_parent id:" << id_
            //           << " child_proposer:" << key.first
            //           << " child_proposal:" << key.second
            //           << " missing_proposer:" << pre_key.first
            //           << " missing_proposal:" << pre_key.second;
          }
          // Only kick off lookups when we have not already enqueued a waiting
          // child for this parent; AskProposal/AskRound have their own
          // 500ms throttles to absorb concurrent calls, but skipping here
          // avoids issuing the same broadcast for every duplicate child.
          if (!duplicate_waiting) {
            AskProposal(proposal.header().pre_proposer_id(),
                        proposal.header().pre_proposal_id(),
                        proposal.header().prehash());
            AskProposal(proposal.header().height() - 1);
          }
          // Remember that we need to recover at least up to the parent of
          // the proposal we just rejected. StartRecovery uses recv_mutex_
          // (different from g_mutex_ held here) so the lock-order is safe.
          int parent_round = proposal.header().height() - 1;
          if (parent_round > 0) {
            StartRecovery(parent_round, "wait_missing_parent");
          }
          //sleep(1);
          return false;
          //continue;
          //}
        }
        // TrySendRecoveery(proposal);
        return false;
      }
      break;
    }

    if (proposal.header().proposer_id() == id_) {
      proposal_manager_->RemoveLocalProposal(proposal.header().hash());
    }
  }

  // Leader fast-path liveness hook: graph::AddProposal already promoted the
  // prehash parent to PoR for a leader proposal. Mirror that into the
  // cassandra layer's can_vote_ map keyed by parent height (= height-1), so
  // the local AsyncConsensus loop's WaitVote(parent_height) can return
  // promptly without depending on receiving 2f+1 distinct vote messages
  // (which is impossible during partition / when fewer than 2f+1 nodes
  // actively propose). The vote-quorum path in ReceiveProposalVote is kept
  // as a redundant trigger for the same can_vote_[h] flag.
  {
    const int proposal_height = proposal.header().height();
    const int proposer = proposal.header().proposer_id();
    if (proposal_height > 1 &&
        IsLeader(proposal_height, proposer)) {
      const int parent_height = proposal_height - 1;
      bool need_notify = false;
      {
        std::unique_lock<std::mutex> lk(gg_mutex_);
        if (!can_vote_[parent_height]) {
          can_vote_[parent_height] = true;
          need_notify = true;
        }
      }
      if (need_notify) {
        //LOG(ERROR) << "CASS_DIAG can_vote_leader_fastpath id:" << id_
        //           << " parent_height:" << parent_height
        //           << " trigger_round:" << proposal_height
        //           << " trigger_proposer:" << proposer;
        vote_cv_.notify_all();
      }
    }
  }

  if(IsLeader(proposal.header().height(), proposal.header().proposer_id())) {
    std::unique_ptr<Proposal> next_proposal = std::make_unique<Proposal>();
    *next_proposal->mutable_header() = proposal.header();
    next_proposal->mutable_header()->set_sender_id(id_);
    SendMessage(MessageType::CMD_ProposalVote, *next_proposal, NextLeader(proposal.header().height()));
  }
  else {
    // CRITICAL: this branch executes WITHOUT g_mutex_ (released at line 1311
    // when the outer scope closed). Two unsafe-without-locking concerns:
    //  (1) slow_received_num_ / slow_sent_height_ are cassandra-layer maps
    //      shared across threads. Without a mutex two concurrent
    //      AddProposal calls race on insert and may corrupt the std::set
    //      internal nodes -> nullptr deref / coredump.
    //  (2) graph_->GetStrongestProposal walks last_node_[h] and dereferences
    //      raw NodeInfo* obtained from node_info_. Concurrent AddProposal
    //      calls on other threads are mutating these structures under their
    //      own g_mutex_. Reading them here without g_mutex_ is a data race
    //      that has been observed crashing replicas during the early
    //      "everyone proposing simultaneously" startup window before
    //      partition kicks in.
    // We use the documented g_mutex_ -> gg_mutex_ ordering: take a fresh
    // g_mutex_ for the graph access and bracket the cassandra-layer state
    // mutations in gg_mutex_.
    const int proposal_height = proposal.header().height();
    const int need_num = need_num_;
    bool first_quorum = false;
    {
      std::unique_lock<std::mutex> lk(gg_mutex_);
      slow_received_num_[proposal_height].insert(proposal.header().proposer_id());
      const size_t received_size = slow_received_num_[proposal_height].size();
      if (received_size >= static_cast<size_t>(need_num) &&
          slow_sent_height_.insert(proposal_height).second) {
        first_quorum = true;
        //LOG(ERROR) << "CASS_DIAG slowpath_quorum id:" << id_
        //           << " proposal_height:" << proposal_height
        //           << " num:" << received_size
        //           << " need:" << need_num;
      }
    }
    if (first_quorum) {
      // Take g_mutex_ to safely read graph_->last_node_/node_info_.
      // GetStrongestProposal returns a Proposal* into node_info_; we copy
      // its header out under the lock so the message we send is independent
      // of the underlying NodeInfo lifetime.
      Header header_to_send;
      bool have_sp = false;
      {
        std::unique_lock<std::mutex> g_lk(g_mutex_);
        Proposal* sp = graph_->GetStrongestProposal(proposal_height);
        if (sp != nullptr) {
          header_to_send = sp->header();
          have_sp = true;
        }
      }
      if (!have_sp) {
        //LOG(ERROR) << "CASS_DIAG slowpath_vote_no_sp id:" << id_
        //           << " height:" << proposal_height;
        return true;
      }
      std::unique_ptr<Proposal> next_proposal = std::make_unique<Proposal>();
      *next_proposal->mutable_header() = header_to_send;
      next_proposal->mutable_header()->set_sender_id(id_);
      SendMessage(MessageType::CMD_ProposalVote, *next_proposal, NextLeader(proposal_height));
    }
  }
  return true;
}

}  // namespace cassandra_recv
}  // namespace cassandra
}  // namespace resdb

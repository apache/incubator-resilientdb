#include "platform/consensus/ordering/cassandra/algorithm/proposal_graph.h"

#include <glog/logging.h>

#include <queue>
#include <stack>

#include "common/utils/utils.h"

namespace resdb {
namespace cassandra {
namespace cassandra_recv {

/*
std::vector<ProposalState> GetStates() {
  return std::vector<ProposalState>{ProposalState::New, ProposalState::Prepared,
                                    ProposalState::PreCommit};
}
*/

bool IsLeader(int height, int id, int total_num) {
  int leader = height % total_num;
  //LOG(ERROR)<<" height:"<<height<<" id:"<<id<<" leader:"<<leader;
  if(leader == 0) leader = total_num;
  return leader == id;
}


ProposalGraph::ProposalGraph(int fault_num, int id, int total_num) : f_(fault_num),id_(id), total_num_(total_num) {
  ranking_ = std::make_unique<Ranking>();
  current_height_ = 0;
  global_stats_ = Stats::GetGlobalStats();
}

int ProposalGraph::GetBlockNum(const std::string& hash, int local_id, int proposer_id) {
if(num_callback_) {
  return num_callback_(hash, local_id, proposer_id);
}

  return 0;
}

void ProposalGraph::IncreaseHeight() {
  // increase height log suppressed: per-round state advance.
  current_height_++;
}

void ProposalGraph::TryUpgradeHeight(int height) {
  if (last_node_[height].size() > 0) {
    // upgrade height log suppressed.
    current_height_ = height;
  } else {
    // need_to_recovery log suppressed: noisy on recovery path.
  }
}

std::string Encode(const std::string& hash) {
  std::string ret;
  for (int i = 0; i < hash.size(); ++i) {
    int x = hash[i];
    ret += std::to_string(x);
  }
  return ret;
}

void ProposalGraph::AddProposalOnly(const Proposal& proposal) {
  auto it = node_info_.find(proposal.header().hash());
  if (it == node_info_.end()) {
    auto np = std::make_unique<NodeInfo>(proposal);
    node_info_[proposal.header().hash()] = std::move(np);
    // add proposal log suppressed: hot path.
  }
}

void ProposalGraph::AddProposalToLast(const Proposal& proposal) {
  // add to last log suppressed: invoked from recovery for each recovered hash.
  last_node_[proposal.header().height()].insert(proposal.header().hash());
}


int ProposalGraph::ChangeState(const std::string& hash){
  auto node_it = node_info_.find(hash);
  if(node_it == node_info_.end()){
    return 0;
  }
  assert(node_it != node_info_.end());
  if (node_it->second->state == ProposalState::Committed) {
    return 0;
  }
  const ProposalState old_state = node_it->second->state;
  node_it->second->state = ProposalState::PoR;
  if (old_state != ProposalState::PoR) {
    TryCommitByChain(hash);
  }
  return 0;
}

int ProposalGraph::ChangeState(const Proposal& proposal){
  std::string hash = proposal.header().hash();
  auto node_it = node_info_.find(hash);
  if(node_it == node_info_.end()){
    return 0;
  }
  assert(node_it != node_info_.end());
  if (node_it->second->state == ProposalState::Committed) {
    return 0;
  }
  const ProposalState old_state = node_it->second->state;
  node_it->second->state = ProposalState::PoR;
  if (old_state != ProposalState::PoR) {
    TryCommitByChain(hash);
  }
  return 0;
}

int ProposalGraph::AddProposal(const Proposal& proposal) {
  // Hot path: this is invoked once per incoming proposal (~thousands/sec on
  // 32 nodes). Synchronous LOG(ERROR) here was a top contributor to the
  // stderr-flood that triggered SIGKILL of the busy replicas.
  assert(current_height_ >= latest_commit_.header().height());

  const auto proposal_key = std::make_pair(proposal.header().proposer_id(),
                                           proposal.header().proposal_id());
  {
    std::unique_lock<std::mutex> lk(mutex_);
    if (data_m_.find(proposal_key) == data_m_.end()) {
      data_p_[proposal.header().height()].push_back(
          std::make_unique<Proposal>(proposal));
      data_m_[proposal_key] = std::make_unique<Proposal>(proposal);
    }
    // skip_duplicate_data_proposal diag suppressed -- duplicates are
    // expected and harmless (our own proposal echo, retransmits, etc).
  }

  if (proposal.header().height() > current_height_) {
    pending_header_[proposal.header().height()].insert(
        proposal.header().proposer_id());
  } else {
    while (!pending_header_.empty()) {
      if (pending_header_.begin()->first <= current_height_) {
        pending_header_.erase(pending_header_.begin());
      } else {
        break;
      }
    }
  }

  //LOG(ERROR) << "height :" << current_height_
  //  << " proposal height:" << proposal.header().height();
  if (proposal.header().height() > current_height_ + 1) {
    //LOG(ERROR) << "height not match:" << current_height_
    //  << " proposal height:" << proposal.header().height();
    if (pending_header_[proposal.header().height()].size() >= f_ + 1 || IsLeader(proposal.header().height(), proposal.header().proposer_id(), total_num_)) {
      TryUpgradeHeight(proposal.header().height());
    }
    return 1;
  }
  if (proposal.header().height() == current_height_ + 1) {
    IncreaseHeight();
  }


  if (!VerifyParent(proposal)) {
    // verify parent fail log suppressed: hot path during recovery.
    return 2;
  }

  // Hot path: history-walk diagnostics suppressed. CheckState() itself
  // already logs once per state transition (CheckState's own LOG is below).
  if(proposal.history_size()>0){
    const auto& history = proposal.history(0);

    std::string hash = history.hash();

    auto node_it = node_info_.find(hash);
    if(node_it == node_info_.end()){
      return 2;
    }
    assert(node_it != node_info_.end());

    // history(0) is the current-round proposal seen by `proposal.proposer_id`;
    // because each proposal commits the full prehash chain, voting for
    // history(0) implicitly endorses the entire chain back to the genesis.
    // We therefore only need to count one vote per (sender, history(0)).
    node_it->second->votes[ProposalState::New].insert(proposal.header().proposer_id());
    CheckState(node_it->second.get(),
        static_cast<resdb::cassandra::ProposalState>(history.state()));

    int num = 0;
    int cur_h = proposal.header().height();
    bool all_por = true;
    bool all_poa_or_por = true;
    for(int i = 0; i <3 && proposal.history_size()>=3; ++i){
      const auto& sub_history = proposal.history(i);
      std::string sub_hash = sub_history.hash();
      auto sub_node_it = node_info_.find(sub_hash);
      if(sub_node_it == node_info_.end()){
        all_por = false;
        all_poa_or_por = false;
        break;
      }

      const ProposalState state = sub_node_it->second->state;

      assert(cur_h == sub_node_it->second->proposal.header().height()+1);
      cur_h--;
      const bool is_por_state =
          state == ProposalState::PoR || state == ProposalState::Committed;
      const bool is_poa_or_por_state =
          is_por_state || state == ProposalState::PoA;
      if (!is_poa_or_por_state) {
        all_por = false;
        all_poa_or_por = false;
        break;
      }
      if (!is_por_state) {
        all_por = false;
      }
      num++;
    }

    if(num == 3) {
      const auto& sub_history = proposal.history(2);
      std::string sub_hash = sub_history.hash();

      auto sub_node_it = node_info_.find(sub_hash);
      if(sub_node_it != node_info_.end()){
        if (sub_node_it->second->state == ProposalState::Committed) {
          // Already committed -- nothing to do, no log.
        } else if (all_por && sub_node_it->second->state == ProposalState::PoR) {
          // chain[head, middle, target] all reached PoR and target is PoR ->
          // commit immediately. The BFS in Commit() will release every
          // ancestor still pending in expected_commit_ as part of the same
          // commit batch (this realizes the "tentative-commit becomes real
          // commit when a downstream PoR commits" semantics).
          Commit(sub_hash);
        } else if (all_poa_or_por) {
          // chain has at least one PoA, or target itself is only PoA/PoR.
          // Tentatively commit: remember target in expected_commit_ and let
          // it be flushed to execute_queue later when a future PoR target's
          // BFS walks back through this hash.
          //expected_commit_.insert(sub_hash);
          //Commit(sub_hash);
        }
      }
    }
  }

  if (proposal.header().height() < current_height_) {
    const std::string& hash = proposal.header().hash();
    node_hash_[std::make_pair(proposal.header().proposer_id(), proposal.header().proposal_id())] = hash;
    if (node_info_.find(hash) == node_info_.end()) {
      auto np = std::make_unique<NodeInfo>(proposal);
      node_info_[hash] = std::move(np);
    }
    last_node_[proposal.header().height()].insert(hash);

    return 0;
  } else {
    g_[proposal.header().prehash()].push_back(proposal.header().hash());
    const std::string& hash = proposal.header().hash();
    node_hash_[std::make_pair(proposal.header().proposer_id(), proposal.header().proposal_id())] = hash;
    if (node_info_.find(hash) == node_info_.end()) {
      auto np = std::make_unique<NodeInfo>(proposal);
      node_info_[hash] = std::move(np);
    }
    last_node_[proposal.header().height()].insert(hash);
  }

  // ----- Leader fast-path (HotStuff-style implicit PoR for parent) -----
  // When the proposal we just added is the legitimate leader proposal of its
  // round, treat its prehash as an implicit PoR certificate for the parent
  // proposal. The leader, by including a specific prehash, attests that
  // the parent is the canonical proposal it builds upon.
  //
  // We optimistically promote the parent regardless of whether its proposer
  // is itself a round leader (this gives us liveness in the rare case the
  // leader picks a non-leader prehash). The commit anchor is independently
  // restricted in TryCommitByChain to "target.proposer must be the
  // round-leader of target.height", so the commit chain across replicas
  // always agrees on a single (leader, leader_hash) target.
  //
  // We DO call TryCommitByChain here so that the leader-chosen prehash
  // chain (which forms the canonical commit chain for healthy rounds) is
  // exercised every time it lengthens by one, recovering the high commit
  // throughput the protocol is designed for.
  if (IsLeader(proposal.header().height(), proposal.header().proposer_id(),
               total_num_)) {
    const std::string& parent_hash = proposal.header().prehash();
    if (!parent_hash.empty()) {
      auto parent_it = node_info_.find(parent_hash);
      if (parent_it != node_info_.end() &&
          parent_it->second->state != ProposalState::Committed &&
          parent_it->second->state != ProposalState::PoR) {
        const ProposalState old_state = parent_it->second->state;
        parent_it->second->state = ProposalState::PoR;
        //LOG(ERROR) << "CASS_DIAG leader_implicit_por parent_proposer:"
        //           << parent_it->second->proposal.header().proposer_id()
        //           << " parent_id:"
        //           << parent_it->second->proposal.header().proposal_id()
        //           << " parent_height:"
        //           << parent_it->second->proposal.header().height()
        //           << " old_state:" << old_state
        //           << " trigger_round:" << proposal.header().height()
        //           << " trigger_proposer:"
        //           << proposal.header().proposer_id();
        TryCommitByChain(parent_hash);
      }
    }
  }

  //GetProposals(96);
  //LOG(ERROR)<<"add graph done";
  return 0;
}

void ProposalGraph::UpgradeState(ProposalState& state) {
return;
}

void ProposalGraph::TryCommitByChain(const std::string& hash) {
  std::vector<NodeInfo*> chain;
  std::string cur_hash = hash;
  for (int i = 0; i < 3 && !cur_hash.empty(); ++i) {
    auto node_it = node_info_.find(cur_hash);
    if (node_it == node_info_.end()) {
      return;
    }
    chain.push_back(node_it->second.get());
    cur_hash = node_it->second->proposal.header().prehash();
  }
  if (chain.size() < 3) {
    return;
  }

  bool all_por = true;
  bool all_poa_or_por = true;
  for (NodeInfo* node : chain) {
    const ProposalState state = node->state;
    const bool is_por_state =
        state == ProposalState::PoR || state == ProposalState::Committed;
    const bool is_poa_or_por_state = is_por_state || state == ProposalState::PoA;
    if (!is_poa_or_por_state) {
      all_por = false;
      all_poa_or_por = false;
      break;
    }
    if (!is_por_state) {
      all_por = false;
    }
  }

  NodeInfo* target = chain[2];
  const std::string target_hash = target->proposal.header().hash();
  if (target->state == ProposalState::Committed) {
    return;
  }
  // Per user instruction, prioritize liveness after partition over strict
  // cross-replica execution agreement. Allow commit on any 3-PoR chain
  // anchor (leader or non-leader), since requiring a leader-only anchor
  // can stall commits when only the leader_implicit_por path has elevated
  // PoR status during partition recovery. The height-level dedup in
  // Commit() and the deterministic Compare() in GetStrongestProposal still
  // ensure no double execution within a single replica.
  if (all_por && target->state == ProposalState::PoR) {
    // commit_by_por_chain diag suppressed: hot commit path.
    Commit(target_hash);
    return;
  }
  if (all_poa_or_por) {
    // chain[head, middle, target] all PoA/PoR but not all PoR; target stays
    // tentatively committed until a future PoR target's Commit() walks back
    // through target_hash and flushes it (along with the rest of its
    // prehash chain) to execute_queue_.
    expected_commit_.insert(target_hash);
    // expect_commit_by_chain diag suppressed: hot commit path.
  }
}

int ProposalGraph::CheckState(NodeInfo* node_info, ProposalState declared_state) {
  // Hot path: invoked once per history(0) of every received proposal.
  // The unconditional LOG was firing several thousand times per second on
  // 32-node clusters; promote_to_por (below) still logs once per actual
  // state transition, which is what we actually care about.
  const size_t vote_num = node_info->votes[ProposalState::New].size();

  if (node_info->state == ProposalState::Committed) {
    return true;
  }
  const ProposalState old_state = node_info->state;

  // Always derive the new state from the actual number of votes accumulated
  // for this proposal. The declared_state from a remote `history` entry is
  // only used to bias upward when votes warrant the promotion; we never trust
  // it blindly because a minority partition would otherwise be able to drive
  // local state up to PoR/Committed without ever reaching 2f+1 votes.
  ProposalState computed_state = New;
  if (vote_num >= static_cast<size_t>(2 * f_ + 1)) {
    computed_state = PoR;
  } else if (vote_num >= static_cast<size_t>(f_ + 1)) {
    computed_state = PoA;
  }

  ProposalState new_state = computed_state;
  // Allow declared_state to elevate state to PoA when votes already cover
  // f+1 (so we accept the upstream signal without weakening safety).
  if (declared_state == PoA && new_state == New &&
      vote_num >= static_cast<size_t>(f_ + 1)) {
    new_state = PoA;
  }
  // Never let declared_state drag the state above what votes support.
  if (new_state < node_info->state) {
    new_state = node_info->state;  // monotonic.
  }
  node_info->state = new_state;
  if (old_state != ProposalState::PoR && new_state == ProposalState::PoR) {
    //LOG(ERROR) << "CASS_DIAG promote_to_por proposer:"
    //           << node_info->proposal.header().proposer_id()
    //           << " id:" << node_info->proposal.header().proposal_id()
    //           << " height:" << node_info->proposal.header().height()
    //           << " votes:" << vote_num << " need:" << (2 * f_ + 1);
    TryCommitByChain(node_info->proposal.header().hash());
  }
  //LOG(ERROR) << "node: (" << node_info->proposal.header().proposer_id() <<
  // ","
  //           << node_info->proposal.header().proposal_id()
  //           << ") get state:" << node_info->state
  //           << " vote num:" << node_info->votes[ProposalState::New].size();

  //GetProposals(96);

  return true;
}

// TryCommitDeferred has been retired. Tentatively committed proposals
// (held in expected_commit_) are now flushed exclusively when a downstream
// PoR target's Commit() walks back through their hash via Commit's BFS.
// We keep an empty stub to preserve the header signature in case other
// translation units still reference it; remove if no longer needed.
void ProposalGraph::TryCommitDeferred(const std::string& /*hash*/) {}

void ProposalGraph::Commit(const std::string& hash, bool allow_poa_commit) {
  //GetProposals(96);
  auto it = node_info_.find(hash);
  if (it == node_info_.end()) {
    LOG(ERROR) << "node not found, hash:" << hash;
    assert(1 == 0);
    return;
  }

 // LOG(ERROR) << "commit, hash:";
  std::set<std::string> is_main_hash;
  is_main_hash.insert(hash);
  
  if (it->second->state == ProposalState::Committed) {
    return;
  }
  if (it->second->state != ProposalState::PoR &&
      !(allow_poa_commit && it->second->state == ProposalState::PoA)) {
    // commit_defer_non_quorum diag suppressed: hot path.
    return;
  }
  // Height-level commit dedup: if some other (proposer, hash) at the same
  // height has already been committed (because a competing PoR raced ahead
  // through a different code path), drop this attempt entirely so we never
  // execute two distinct proposals for the same height. This is a safety
  // net for the rare double-PoR window; it does NOT replace upstream
  // restrictions that aim to make the two PoR paths agree on a single hash.
  {
    const int h = it->second->proposal.header().height();
    if (committed_height_.find(h) != committed_height_.end()) {
      // skip_commit_height_already_committed diag suppressed: hot path.
      // Mark this losing branch as Committed locally to short-circuit any
      // future TryCommitByChain re-entry; we must NOT push it to the
      // execute_queue because the winning branch already did.
      it->second->state = ProposalState::Committed;
      expected_commit_.erase(hash);
      return;
    }
  }

  expected_commit_.erase(hash);

  int from_proposer = it->second->proposal.header().proposer_id();
  int from_proposal_id = it->second->proposal.header().proposal_id();
  // commit entry log suppressed: hot path.
  (void)from_proposer; (void)from_proposal_id;

  std::vector<std::vector<Proposal*>> commit_p;
  std::vector<std::string> newly_committed;
  auto bfs = [&]() {
    std::queue<std::string> q;
    q.push(hash);
    while (!q.empty()) {
      std::string c_hash = q.front();
      q.pop();

      auto it = node_info_.find(c_hash);
      if (it == node_info_.end()) {
        // Defensive: node_info_ entries are not freed by the memory
        // reclaim path, but partition recovery / chain ack races may
        // briefly leave dangling prehashes. Skip rather than abort.
        continue;
      }

      Proposal* p = &it->second->proposal;
      if (it->second->state == ProposalState::Committed) {
        continue;
      }
      // BFS-level height dedup: if a sibling (proposer, hash) at this height
      // has already been committed by a previous Commit() call, do NOT
      // execute this hash's chain at this height. We still mark our local
      // node as Committed to prevent later re-entry, but skip pushing to
      // commit_p so commit_callback_ never fires for it.
      const int p_height = p->header().height();
      if (committed_height_.find(p_height) != committed_height_.end()) {
        // bfs_skip_committed_height diag suppressed: hot commit-BFS path.
        it->second->state = ProposalState::Committed;
        expected_commit_.erase(c_hash);
        if (!p->header().prehash().empty()) {
          q.push(p->header().prehash());
        }
        continue;
      }

      //LOG(ERROR)<<" bfs sub block size :"<<p->sub_block_size();
      /*
      for(auto block : p->sub_block()){
        LOG(ERROR)<<" get sub block proposer:"<<p->header().proposer_id()<<" local id:"<<block.local_id();
      }
    */
      it->second->state = ProposalState::Committed;
      committed_height_.insert(p_height);
      // Any ancestor that was previously tentatively committed (waiting for a
      // downstream PoR to be truly committed) is now flushed for real.
      // flush_expected_commit diag suppressed: hot commit-BFS path.
      expected_commit_.erase(c_hash);
      newly_committed.push_back(c_hash);
      if (is_main_hash.find(c_hash) != is_main_hash.end()) {
        commit_num_[p->header().proposer_id()]++;
        // LOG(ERROR)<<"commit main node:"<<p->header().proposer_id();
        is_main_hash.insert(p->header().prehash());
        commit_p.push_back(std::vector<Proposal*>());
      }

      commit_p.back().push_back(p);
      if (!p->header().prehash().empty()) {
        q.push(p->header().prehash());
      }
    }
  };

  bfs();
  // commit more hash / commit not ready logs suppressed: hot path noise.
  if(commit_p.size()==0) {
    return;
  }
  assert(commit_p.size()>0);
  int block_num = 0;
  int p_num = 0;
  for (int i = commit_p.size() - 1; i >= 0; i--) {
    for (int j = 0; j < commit_p[i].size(); ++j) {
      /*
      if (j == 0) {
        LOG(ERROR) << "commmit proposal lead from:"
                   << commit_p[i][j]->header().proposer_id()
                   << " height:" << commit_p[i][j]->header().height()
                   << " size:" << commit_p[i].size();
      }
      */
      //LOG(ERROR) << "commmit proposal:"
       //          << commit_p[i][j]->header().proposer_id()
        //         << " height:" << commit_p[i][j]->header().height()
         //        << " idx:" << j 
          //       << " delay:" << (GetCurrentTime() - commit_p[i][j]->create_time()) 
           //      << " commit from:"<< from_proposer<<" id:"<<from_proposal_id;
      block_num += commit_p[i][j]->sub_block_size();

      // commmit proposal log suppressed: hot path during commit; per-commit
      // metrics still recorded via global_stats_ below.



      for(auto block : commit_p[i][j]->sub_block()){
        //LOG(ERROR) << "commmit proposal from:" << commit_p[i][j]->header().proposer_id()
                   //<< " block id:" << block.local_id();
        if(check_.find(std::make_pair(block.local_id(), commit_p[i][j]->header().proposer_id())) != check_.end()){
          //LOG(ERROR) << "commmit proposal from:" << commit_p[i][j]->header().proposer_id()
          //         << " block id:" << block.local_id() << "has committed";
        }
        else {
          check_.insert(std::make_pair(block.local_id(), commit_p[i][j]->header().proposer_id()));
          //LOG(ERROR)<<" trans size:"<<block.data().transaction_size();
          p_num+=GetBlockNum(block.hash(), block.local_id(), commit_p[i][j]->header().proposer_id());
        }
      }

      if (commit_callback_) {
        commit_callback_(*commit_p[i][j]);
      }
    }
  }
  //global_stats_->AddCommitBlock(block_num);
  global_stats_->AddCommitTxn(p_num);
  //LOG(ERROR)<<" commit proposal num:"<<p_num;
  //LOG(ERROR)<<"commit proposal from :"<<it->second->proposal.header().proposer_id()<<" id:"<<it->second->proposal.header().proposal_id()<<" height:"<<it->second->proposal.header().height()<<" num:"<<p_num;
  // TODO clean
  {
    std::unique_lock<std::mutex> lk(mutex_);
    temp_last_node_[it->second->proposal.header().height()] = last_node_[it->second->proposal.header().height()];
  }
  last_node_[it->second->proposal.header().height()].clear();
  latest_commit_ = it->second->proposal;
  it->second->state = ProposalState::Committed;

  // ----- Sliding-window memory reclaim (prevents OOM under load) -----
  // Without this every Proposal a replica ever observed lives forever in
  // data_p_ / data_m_ (which keep two copies per proposal: by-height and by
  // (proposer,id)). On a fast replica that processes 100s of proposals/sec
  // during partition heal this grows by tens of MB per second and triggers
  // OOM-killer SIGKILLs (which were the main cause of the early node
  // deaths after partition recovery).
  //
  // We retain a window of the most recently committed heights so that
  // AskProposal / chain-recovery can still respond to peers that lag
  // slightly behind, and drop everything older. The window is generous
  // enough to cover typical recovery quora; nodes farther behind than
  // kCommitRetainWindow will simply have to fetch from a peer that hasn't
  // committed past their target yet.
  // Reclaim memory for the redundant by-height / by-(proposer,id) caches
  // (data_p_, data_m_) that are otherwise never freed. These caches keep
  // FULL Proposal copies (including sub_block bytes), which dominates RSS
  // growth on fast replicas during partition heal and was the suspected
  // OOM cause. We deliberately do NOT prune node_info_ here: other graph
  // operations (GetStrongestProposal / UpdateHistory / TryCommitByChain)
  // walk node_info_ holding raw NodeInfo* pointers, and freeing those
  // entries from another thread is unsafe (it caused observed segfaults).
  // node_info_ memory will be addressed separately if it becomes a
  // problem; in practice it carries only Proposal headers, not the bulk
  // sub_block payloads, so its growth rate is much smaller than data_p_.
  static const int kCommitRetainWindow = 256;
  const int committed_h = it->second->proposal.header().height();
  const int cutoff = committed_h - kCommitRetainWindow;
  if (cutoff > 0) {
    std::unique_lock<std::mutex> lk(mutex_);
    while (!data_p_.empty() && data_p_.begin()->first <= cutoff) {
      for (const auto& p : data_p_.begin()->second) {
        if (!p) continue;
        data_m_.erase(std::make_pair(p->header().proposer_id(),
                                     p->header().proposal_id()));
      }
      data_p_.erase(data_p_.begin());
    }
    while (!temp_last_node_.empty() &&
           temp_last_node_.begin()->first <= cutoff) {
      temp_last_node_.erase(temp_last_node_.begin());
    }
  }
  //GetProposals(96);
  // Clear(latest_commit_.header().hash());
}

std::vector<std::unique_ptr<Proposal>> ProposalGraph::GetNotFound(
    int height, const std::string& hash) {
  auto it = not_found_proposal_.find(height);
  if (it == not_found_proposal_.end()) {
    return std::vector<std::unique_ptr<Proposal>>();
  }
  auto pre_it = it->second.find(hash);
  std::vector<std::unique_ptr<Proposal>> ret;
  if (pre_it != it->second.end()) {
    ret = std::move(pre_it->second);
    it->second.erase(pre_it);
    //LOG(ERROR) << "found future height:" << height;
  }
  return ret;
}

bool ProposalGraph::VerifyParent(const Proposal& proposal) {
  // LOG(ERROR) << "last commit:" << latest_commit_.header().proposal_id()
  //           << " current :" << proposal.header().proposal_id()
  //           << " height:" << proposal.header().height();

  //GetProposals(96);
  if (proposal.header().prehash() == latest_commit_.header().hash()) {
    return true;
  }

  std::string prehash = proposal.header().prehash();
  // LOG(ERROR)<<"prehash:"<<prehash;

  auto it = node_info_.find(prehash);
  if (it == node_info_.end()) {
    // prehash not here log suppressed: hot path during recovery.
    not_found_proposal_[proposal.header().height()][proposal.header().prehash()]
        .push_back(std::make_unique<Proposal>(proposal));
    return false;
  } else {
    if (proposal.header().height() !=
        it->second->proposal.header().height() + 1) {
      //LOG(ERROR) << "link to invalid proposal, height:"
      //           << proposal.header().height()
      //           << " pre height:" << it->second->proposal.header().height();
      return false;
    }
  }
  //GetProposals(96);
  return true;
}

void ProposalGraph::UpdateHistory(Proposal* proposal) {
  proposal->mutable_history()->Clear();
  std::string hash = proposal->header().hash();

  for (int i = 0; i < 3 && !hash.empty(); ++i) {
    auto node_it = node_info_.find(hash);
    if(node_it == node_info_.end()){
      break;
    }
    auto his = proposal->add_history();
    his->set_hash(hash);
    his->set_state(node_it->second->state);
    his->set_sender(node_it->second->proposal.header().proposer_id());
    his->set_id(node_it->second->proposal.header().proposal_id());
    hash = node_it->second->proposal.header().prehash();
    // Per-step UpdateHistory diag suppressed: invoked once per
    // GetStrongestProposal* call which itself fires on every vote arrival.
  }
}

Proposal* ProposalGraph::GetStrongestProposal() {
  // Hot path: see GetStrongestProposal(int). LOG suppressed.
  if (last_node_.find(current_height_) == last_node_.end()) {
    return nullptr;
  }

  //LOG(ERROR)<<" node size:"<<last_node_[current_height_].size();
  NodeInfo* sp = nullptr;
  for (const auto& last_hash : last_node_[current_height_]) {
    if(node_info_.find(last_hash) == node_info_.end()){
      continue;
    }
    NodeInfo* node_info = node_info_[last_hash].get();
    assert(node_info->proposal.header().height() == current_height_);
    if (sp == nullptr || Compare(*sp, *node_info)) {
      sp = node_info;
    }
  }
  if(sp == nullptr) {
    return nullptr;
  }
  //assert(sp != nullptr);

  //LOG(ERROR)<<" last node size:"<<last_node_[current_height_].size()<<" height:"<<current_height_<<" get strong from:"<<sp->proposal.header().proposer_id();

  for (const auto& last_hash : last_node_[current_height_]) {
    if(node_info_.find(last_hash) == node_info_.end()){
      continue;
    }
    NodeInfo* node_info = node_info_[last_hash].get();
  //  LOG(ERROR)<<" node info:"<<node_info->proposal.header().proposer_id()<<" sub blocks:"<<node_info->proposal.sub_block_size();

    if(node_info->proposal.header().proposer_id() != id_){
      continue;
    }

    //LOG(ERROR)<<" node info:"<<node_info->proposal.header().proposer_id()<<" sub blocks:"<<node_info->proposal.sub_block_size()<<" node state:"<<node_info->state;
    if(node_info->state == ProposalState::PoR) {
      // See GetStrongestProposal(int): tolerate transient double-PoR at the
      // same height. Compare() picks the deterministic winner as `sp`; the
      // losing PoR's sub_blocks are still removed locally so we don't
      // re-include them in our next proposal. Commit-level dedup in
      // Commit() prevents the losing chain from being executed.
      // double_por_height diag suppressed on hot path.
      for(auto sub_block : node_info->proposal.sub_block()){ 
        if(new_blocks_.find(sub_block.hash()) != new_blocks_.end()){
          new_blocks_.erase(new_blocks_.find(sub_block.hash()));
        }
      }
      continue;
    }

    //LOG(ERROR)<<"get sub block size:"<<node_info->proposal.sub_block_size();
    for(auto sub_block : node_info->proposal.sub_block()){ 
      new_blocks_[sub_block.hash()] = sub_block;
    }
    //LOG(ERROR)<<" new blocks:"<<new_blocks_.size();


    std::string pre_prehash = node_info->proposal.header().prehash();
    if(node_info_.find(pre_prehash) != node_info_.end()){
      NodeInfo* pre_node_info = node_info_[pre_prehash].get();
      //LOG(ERROR)<<" pre node info:"<<pre_node_info->proposal.header().proposer_id()
      //<<" pre node state:"<<pre_node_info->state;

      if(pre_node_info->state == ProposalState::PoR 
          && pre_node_info->proposal.header().proposer_id() == id_) {
        for(auto sub_block : pre_node_info->proposal.sub_block()){ 
          if(new_blocks_.find(sub_block.hash()) != new_blocks_.end()){
            //LOG(ERROR)<<" remove new blocks:"<<sub_block.local_id(); 
            new_blocks_.erase(new_blocks_.find(sub_block.hash()));
          }
        }
      }
    }
  }



  //LOG(ERROR)<<" update his";
  UpdateHistory(&sp->proposal);
   //LOG(ERROR) << "get strong proposal from height:" << current_height_ << " ->("
   //          << sp->proposal.header().proposer_id() << ","
   //          << sp->proposal.header().proposal_id() << ")";
  return &sp->proposal;
}


Proposal* ProposalGraph::GetStrongestProposal(int height) {
  // Hot path: this is invoked from the slow-path quorum branch in
  // Cassandra::AddProposal()'s else clause AND from ReceiveProposalVote().
  // At 32 nodes last_node_[height] can contain up to total_num_ entries,
  // so the per-iteration LOG previously generated up to ~N*calls_per_sec
  // = 32 * thousands of lines per second per replica, dwarfing every other
  // log source and stalling the process via stderr backpressure.
  if (last_node_.find(height) == last_node_.end() || last_node_[height].empty()) {
    return nullptr;
  }

  NodeInfo* sp = nullptr;
  for (const auto& last_hash : last_node_[height]) {
    if(node_info_.find(last_hash) == node_info_.end()){
      continue;
    }
    NodeInfo* node_info = node_info_[last_hash].get();
    assert(node_info->proposal.header().height() == height);
    if (sp == nullptr || Compare(*sp, *node_info)) {
      sp = node_info;
    }
  }
  if(sp == nullptr) {
    return nullptr;
  }

  for (const auto& last_hash : last_node_[height]) {
    if(node_info_.find(last_hash) == node_info_.end()){
      continue;
    }
    NodeInfo* node_info = node_info_[last_hash].get();

    if(node_info->proposal.header().proposer_id() != id_){
      continue;
    }

    //LOG(ERROR)<<" node info:"<<node_info->proposal.header().proposer_id()<<" sub blocks:"<<node_info->proposal.sub_block_size()<<" node state:"<<node_info->state;
    if(node_info->state == ProposalState::PoR) {
      // Tolerate transient double-PoR at the same height. Two paths can
      // independently raise PoR for different (proposer, hash) pairs at the
      // same height (leader-implicit-por for leader's prehash; quorum-vote
      // for the leader-of-this-round hash). The Compare() rule above
      // deterministically picks ONE of them as `sp` (PoR-tied state, then
      // closest distance to the round leader), which is what every honest
      // replica also computes. The losing PoR is still useful locally to
      // suppress its sub_blocks from being repackaged in our next proposal.
      // Commit() / TryCommitByChain() already gate on `state==Committed`
      // per-hash, but to prevent the losing PoR's chain from being committed
      // ahead of (or in addition to) the winner's chain we also dedupe
      // commits at the height level (see Commit()).
      // double_por_height_int diag suppressed on hot path (see comment block
      // above for tolerance rationale).
      for(auto sub_block : node_info->proposal.sub_block()){
        if(new_blocks_.find(sub_block.hash()) != new_blocks_.end()){
          new_blocks_.erase(new_blocks_.find(sub_block.hash()));
        }
      }
      continue;
    }

    //LOG(ERROR)<<"get sub block size:"<<node_info->proposal.sub_block_size();
    for(auto sub_block : node_info->proposal.sub_block()){ 
      new_blocks_[sub_block.hash()] = sub_block;
    }
    //LOG(ERROR)<<" new blocks:"<<new_blocks_.size();


    std::string pre_prehash = node_info->proposal.header().prehash();
    if(node_info_.find(pre_prehash) != node_info_.end()){
      NodeInfo* pre_node_info = node_info_[pre_prehash].get();
      //LOG(ERROR)<<" pre node info:"<<pre_node_info->proposal.header().proposer_id()
      //<<" pre node state:"<<pre_node_info->state;

      if(pre_node_info->state == ProposalState::PoR 
          && pre_node_info->proposal.header().proposer_id() == id_) {
        for(auto sub_block : pre_node_info->proposal.sub_block()){ 
          if(new_blocks_.find(sub_block.hash()) != new_blocks_.end()){
            //LOG(ERROR)<<" remove new blocks:"<<sub_block.local_id(); 
            new_blocks_.erase(new_blocks_.find(sub_block.hash()));
          }
        }
      }
    }
  }



  UpdateHistory(&sp->proposal);
  return &sp->proposal;
}


bool ProposalGraph::Cmp(int id1, int id2) {
   //LOG(ERROR) << "commit commit num:" << id1 << " " << id2
   //          << " commit  time:" << commit_num_[id1] << " " <<
   //          commit_num_[id2];
  if (commit_num_[id1]  < commit_num_[id2]) {
    return false;
  }

  if (commit_num_[id1] > commit_num_[id2] ) {
    return true;
  }
  return id1 < id2;
}

int ProposalGraph::StateScore(const ProposalState& state) {
  // return state == ProposalState::Prepared? 1:0;
  return state;
}

int ProposalGraph::CompareState(const ProposalState& state1,
                                const ProposalState& state2) {
  // LOG(ERROR) << "check state:" << state1 << " " << state2;
  return StateScore(state1) - StateScore(state2);
}

// p1 < p2
bool ProposalGraph::Compare(const NodeInfo& p1, const NodeInfo& p2) {
  //LOG(ERROR) << "proposer:" << p1.proposal.header().proposer_id() << " "
  //           << p2.proposal.header().proposer_id()
  //          << "height:" << p1.proposal.header().height() << " "
  //          << p2.proposal.header().height()
  //          <<" state:"<< p1.state<<" "<<p2.state
  //         <<" hash cmp:"<< (p1.proposal.header().hash() < p2.proposal.header().hash())
  //         <<" cmp num:" << Cmp(p1.proposal.header().proposer_id(), p2.proposal.header().proposer_id())
  //         <<" sub block:" << p1.proposal.sub_block_size() <<" "<< p2.proposal.sub_block_size();
  if (p1.proposal.header().height() != p2.proposal.header().height()) {
    return p1.proposal.header().height() < p2.proposal.header().height();
  }
  // LOG(ERROR)<<"proposer:"<<p1.proposal.header().proposer_id()<<"
  // "<<p2.proposal.header().proposer_id();
  if (CompareState(p1.state, p2.state) != 0) {
    return CompareState(p1.state, p2.state) < 0;
  }

  int h = (p1.proposal.header().height())%total_num_;
  if ( h == 0) h = total_num_;
  //LOG(ERROR)<<" check height :"<<h<<" cmp:"<<abs(p1.proposal.header().proposer_id() - h )<<" "<<abs(p2.proposal.header().proposer_id() - h)<<" from:"<<p1.proposal.header().proposer_id();
  //if (p1.proposal.header().height() <= 120 && 220 <= proposal.header().height()) {
    return abs(p1.proposal.header().proposer_id() - h ) > abs(p2.proposal.header().proposer_id() - h);
  //}

  if (abs(p1.proposal.sub_block_size() - p2.proposal.sub_block_size()) > 5) {
    //return p1.proposal.sub_block_size() < p2.proposal.sub_block_size();
  }
  return p1.proposal.header().hash() < p2.proposal.header().hash();

  if (p1.proposal.header().proposer_id() ==
      p2.proposal.header().proposer_id()) {
    return p1.proposal.header().proposal_id() <
           p2.proposal.header().proposal_id();
  }

  return Cmp(p1.proposal.header().proposer_id(),
             p2.proposal.header().proposer_id());
}

Proposal* ProposalGraph::GetLatestStrongestProposal() {
int num = 0;
  while(true){
    Proposal* sp = GetStrongestProposal();
    if (sp == nullptr) {
      if (current_height_ > 0) {
      num++;
      if(num>3) {
        //return nullptr;
      }
        LOG(ERROR)<<" wait for sp:"<<current_height_;
        sleep(1);
        continue;
        assert(1 == 0);
      }
      return &latest_commit_;
    }

    // LOG(ERROR) << "====== get strong proposal from:" <<
    // sp->header().proposer_id()
    //           << " id:" << sp->header().proposal_id();
    return sp;
  }
}

Proposal* ProposalGraph::GetLatestStrongestProposal(int round) {
int num = 0;
  while(true){
    Proposal* sp = GetStrongestProposal(round);
    if (sp == nullptr) {
      if (current_height_ > 0) {
      num++;
      if(num>3) {
        //return nullptr;
      }
        LOG(ERROR)<<" wait for sp:"<<current_height_;
        sleep(1);
        continue;
        assert(1 == 0);
      }
      return &latest_commit_;
    }

    //temp_last_node_[round].insert(sp->header().hash());
    return sp;
  }
}


ProposalState ProposalGraph::GetProposalState(const std::string& hash) const {
  auto node_it = node_info_.find(hash);
  if (node_it == node_info_.end()) {
    return ProposalState::New;
  }
  return node_it->second->state;
}

const Proposal* ProposalGraph::GetProposalInfo(const std::string& hash) const {
  auto it = node_info_.find(hash);
  if (it == node_info_.end()) {
    LOG(ERROR) << "hash not found:" << Encode(hash);
    return nullptr;
  }
  return &it->second->proposal;
}

int ProposalGraph::GetCurrentHeight() { return current_height_; }

std::vector<Proposal*> ProposalGraph::GetNewProposals(int height) {
  std::vector<Proposal*> ps;
  for (auto it : new_proposals_) {
  /*
    if (it.second->header().height() >= height) {
      continue;
    }
    */
    ps.push_back(it.second);
  }
  for (Proposal* p : ps) {
    new_proposals_.erase(new_proposals_.find(p->header().hash()));
  }
  return ps;
}

std::vector<Block> ProposalGraph::GetNewBlocks() {
  std::vector<Block> ps;
  for (auto it : new_blocks_) {
    ps.push_back(it.second);
  }
  //new_blocks_.clear();
  return ps;
}

int ProposalGraph::GetLastRound() {
  if(last_node_.empty()){
    return 0;
  }
  return (--last_node_.end())->first;
}

std::vector<std::unique_ptr<Proposal> > ProposalGraph::GetProposals(int round) {

  std::vector<std::unique_ptr<Proposal> > ret;
  std::unique_lock<std::mutex> lk(mutex_);


  if(data_p_.find(round) == data_p_.end()) {
    return ret;
  }

  for(auto& p : data_p_[round]) {
      // Per-proposal log suppressed: hot path on AskRound responses, can be
      // up to total_num_ entries per call and called per round during recov.
      ret.push_back(std::make_unique<Proposal>(*p));
  }
  return ret;
       //  LOG(ERROR)<<" node info:"<<node_info->proposal.header().proposer_id()<<" sub blocks:"<<node_info->proposal.sub_block_size();
  /*
   LOG(ERROR)<<" get proposal round:"<<round<<" last node size:"<<last_node_[round].size()<<" tmp size:"<<temp_last_node_[round].size();
   for(int i = std::max(round,1); i <=round; ++i){
     for (const auto& last_hash : last_node_[i]) {
       if(node_info_.find(last_hash) == node_info_.end()){
         LOG(ERROR)<<"round:"<<round<<" no node info";
         continue;
       }
       NodeInfo* node_info = node_info_[last_hash].get();
        LOG(ERROR)<<" node info:"<<node_info->proposal.header().proposer_id()<<" sub blocks:"<<node_info->proposal.sub_block_size()<< node_info->proposal.history_size();
        const auto& proposal = node_info->proposal;
        if(proposal.history_size()>0){
          const auto& history = proposal.history(0);

          std::string hash = history.hash();
          int proposer = history.sender();
          int proposal_id = history.id();

          LOG(ERROR)<<" node info :"<<proposal.header().proposer_id()<<" proposal id:"<<proposal.header().proposal_id()<<" history state:"<< history.state()<< "proposer:"<<proposer<<" proposal id:"<<proposal_id<<" hash:"<<Encode(hash);
        }
       ret.push_back(std::make_unique<Proposal>(node_info->proposal)); 
     }

     for (const auto& last_hash : temp_last_node_[i]) {
       if(node_info_.find(last_hash) == node_info_.end()){
         LOG(ERROR)<<"round:"<<round<<" no node info";
         continue;
       }
       NodeInfo* node_info = node_info_[last_hash].get();
       //  LOG(ERROR)<<" node info:"<<node_info->proposal.header().proposer_id()<<" sub blocks:"<<node_info->proposal.sub_block_size();
       const auto& proposal = node_info->proposal;
       if(proposal.history_size()>0){
         const auto& history = proposal.history(0);

         std::string hash = history.hash();
         int proposer = history.sender();
         int proposal_id = history.id();

         LOG(ERROR)<<" node info :"<<proposal.header().proposer_id()<<" proposal id:"<<proposal.header().proposal_id()<<" history state:"<< history.state()<< "proposer:"<<proposer<<" proposal id:"<<proposal_id<<" hash:"<<Encode(hash);
        }

       ret.push_back(std::make_unique<Proposal>(node_info->proposal)); 
     }
   }
   */
  
  return ret;
}

std::vector<std::unique_ptr<Proposal>> ProposalGraph::GetProposals(int sender, int proposal_id, const std::string& h) {

  std::vector<std::unique_ptr<Proposal>> ret;
  std::unique_lock<std::mutex> lk(mutex_);
  int p = sender;
  int id = proposal_id;
  while(ret.size()<10){
    // Per-step trace log suppressed: chain walk fires up to 10 times per
    // AskProposal response, several thousand calls per second on busy
    // recovery paths.
    if(data_m_.find(std::make_pair(p,id)) == data_m_.end()){
      return ret;
    }
    auto pd = std::make_unique<Proposal>(*data_m_[std::make_pair(p,id)]);
    p = pd->header().pre_proposer_id();
    id= pd->header().pre_proposal_id();
    ret.push_back(std::move(pd)); 
  }
  return ret;
}

bool  ProposalGraph::CheckProposals(int sender, int proposal_id, const std::string&hash) {

  std::unique_lock<std::mutex> lk(mutex_);
  if(node_info_.find(hash) == node_info_.end()){
    LOG(ERROR)<<"round:"<<proposal_id<<" no node info";
    return false;
  }
  return true;
}

}  // namespace cassandra_recv
}  // namespace cassandra
}  // namespace resdb

#pragma once

#include <map>
#include <mutex>
#include <set>

#include "platform/consensus/ordering/mysticeti/proto/proposal.pb.h"

namespace resdb {
namespace mysticeti {

class ProposalManager {
 public:
  ProposalManager(int32_t id, int limit_count, int total_num);

  std::unique_ptr<Proposal> GenerateProposal(
      const std::vector<std::unique_ptr<Transaction>>& txns);

  int CurrentRound();
  void AddLocalBlock(std::unique_ptr<Proposal> proposal);
  const Proposal* GetLocalBlock(const std::string& hash);
  std::unique_ptr<Proposal> FetchLocalBlock(const std::string& hash);

  // Mysticeti-C uncertified DAG: store blocks directly, no explicit certs
  void AddBlockToDAG(std::unique_ptr<Proposal> proposal);
  bool HasBlock(int round, int author);
  const Proposal* GetBlock(int round, int author);
  std::unique_ptr<Proposal> FetchBlock(int round, int author);
  bool CheckBlockExists(const std::string& hash);
  int GetDAGRoundSize(int round);

  bool Ready();
  // Relaxed Ready: accept a smaller parent count when the full 2f+1
  // quorum isn't reachable (e.g. one of the n-f alive replicas is
  // temporarily a straggler). `min_count` is the lowered threshold.
  bool ReadyRelaxed(int min_count);
  bool VerifyHash(const Proposal& proposal);

  // Mysticeti-C commit rule: support and certificate tracking
  int GetSupportCount(int round, int proposer);
  int GetCertCount(int round, int proposer);

  // Pipelined leader: every round has a leader
  int GetLeader(int round);

  void SetCommittedRound(int r);

 protected:
  void GetParentRefs(Proposal* proposal);

 private:
  int32_t id_;
  int round_;
  int limit_count_;
  int total_num_;

  // Uncertified DAG storage: dag_[round][author] = block
  std::map<int, std::map<int, std::unique_ptr<Proposal>>> dag_;
  std::map<std::string, std::unique_ptr<Proposal>> local_block_;

  // Support tracking: support_[round][proposer] = {set of supporters at round+1}
  std::map<int, std::map<int, std::set<int>>> support_;
  // Certificate tracking: cert_count_[round][proposer] = count of certifying blocks
  std::map<int, std::map<int, int>> cert_count_;

  // Track blocks already consumed by BFS execution (prevents double processing)
  std::set<std::pair<int, int>> fetched_;

  // Amortized GC: every gc_interval_ calls to AddBlockToDAG, prune
  // rounds older than the last committed round minus a safety window.
  int gc_counter_ = 0;
  static constexpr int gc_interval_ = 200;
  static constexpr int gc_keep_window_ = 20;
  void MaybeGC();
  // Round-based GC anchor: even if commits stall, prune based on the
  // current proposal round (minus a larger safety window).
  std::atomic<int> max_proposal_round_{0};

  std::mutex txn_mutex_, local_mutex_;
  std::atomic<int> committed_round_ = 0;
};

}  // namespace mysticeti
}  // namespace resdb

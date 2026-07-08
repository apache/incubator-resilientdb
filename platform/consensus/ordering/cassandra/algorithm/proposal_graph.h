#pragma once

#include <map>
#include <set>

#include "platform/consensus/ordering/cassandra/algorithm/proposal_state.h"
#include "platform/consensus/ordering/cassandra/algorithm/ranking.h"
#include "platform/consensus/ordering/cassandra/proto/proposal.pb.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace cassandra {
namespace cassandra_recv {

class ProposalGraph {
 public:
  ProposalGraph(int fault_num, int id,int total_num);
  inline void SetCommitCallBack(std::function<void(const Proposal&)> func) {
    commit_callback_ = func;
  }

  inline void SetBlockNumCallBack(
    std::function<int(const std::string& hash, int id, int sender)> func) {
    num_callback_ = func;
  }

  int AddProposal(const Proposal& proposal);
  void AddProposalOnly(const Proposal& proposal);

  std::vector<std::unique_ptr<Proposal> > GetProposals(int round);
  std::vector<std::unique_ptr<Proposal> > GetProposals(int sender, int proposal_id, const std::string& hash);

  Proposal* GetLatestStrongestProposal();
  Proposal* GetLatestStrongestProposal(int height);
  Proposal* GetStrongestProposal(int round);
  const Proposal* GetProposalInfo(const std::string& hash) const;

  int GetCurrentHeight();

  void Clear(const std::string& hash);
  void IncreaseHeight();
  ProposalState GetProposalState(const std::string& hash) const;
bool  CheckProposals(int sender, int proposal_id, const std::string&hash);

  int ChangeState(const Proposal& proposal);
  int ChangeState(const std::string& hash);
  void TryUpgradeHeight(int height);

  std::vector<std::unique_ptr<Proposal>> GetNotFound(int height,
                                                     const std::string& hash);

  std::vector<Proposal*> GetNewProposals(int height);
  std::vector<Block> GetNewBlocks();
  int GetLastRound();
  void AddProposalToLast(const Proposal& proposal);

 private:
  struct NodeInfo {
    Proposal proposal;
    ProposalState state;
    int score;
    int is_main;
    // std::set<int> received_num[5];
    std::map<int, std::set<int>> votes;

    NodeInfo(const Proposal& proposal)
        : proposal(proposal), state(ProposalState::New), score(0), is_main(0) {}
  };

  bool VerifyParent(const Proposal& proposal);

  bool Compare(const NodeInfo& p1, const NodeInfo& p2);
  bool Cmp(int id1, int id2);
  int StateScore(const ProposalState& state);
  int CompareState(const ProposalState& state1, const ProposalState& state2);

  Proposal* GetStrongestProposal();

  void UpdateHistory(Proposal* proposal);
  int CheckState(NodeInfo* node_info, ProposalState state);
  void UpgradeState(ProposalState& state);
  void TryCommitByChain(const std::string& hash);
  void TryCommitDeferred(const std::string& hash);

  void Commit(const std::string& hash, bool allow_poa_commit = false);
  int GetBlockNum(const std::string& hash, int local_id, int proposer_id);

 private:
  Proposal latest_commit_;
  std::map<std::string, std::vector<std::string>> g_;
  std::map<std::string, std::unique_ptr<NodeInfo>> node_info_;
  std::map<std::string, std::vector<VoteMessage>> not_found_;
  std::unique_ptr<Ranking> ranking_;
  std::map<int, int> commit_num_;
  std::map<int, std::set<std::string>> last_node_, temp_last_node_;
  std::set<std::string> expected_commit_;
  std::map<std::string, std::set<std::string>> deferred_by_anchor_;
  // Height-level commit dedup. Two PoR hashes can transiently exist at the
  // same height (leader-implicit-por path vs quorum-vote path). We allow
  // both to live in the graph, but only ONE of them is allowed to commit /
  // execute. The first hash to call Commit() at a given height "wins" and
  // is recorded here; later commit attempts at the same height are
  // suppressed. Since GetStrongestProposal() / Compare() use a
  // deterministic tie-break on the same data, every honest replica sees
  // the same "winner" provided the chosen (proposer, hash) reaches Commit.
  std::set<int> committed_height_;

  int current_height_;
  uint32_t f_;
  std::function<void(const Proposal&)> commit_callback_;
  std::function<int(const std::string&id, int, int&)> num_callback_;
  std::map<int, std::set<int>> pending_header_;
  std::map<int, std::map<std::string, std::vector<std::unique_ptr<Proposal>>>>
      not_found_proposal_;
  std::map<std::string, Proposal*> new_proposals_;
  Stats* global_stats_;
  int id_;
  std::map<std::string, Block> new_blocks_;
  int total_num_;
  std::set<std::pair<int,int> > check_;
  std::map<std::pair<int,int>, std::string> node_hash_;
  std::map<int, std::vector<std::unique_ptr<Proposal> > > data_p_;
  std::map<std::pair<int,int>, std::unique_ptr<Proposal> > data_m_;
  std::mutex mutex_;
};

}  // namespace cassandra_recv
}  // namespace cassandra
}  // namespace resdb

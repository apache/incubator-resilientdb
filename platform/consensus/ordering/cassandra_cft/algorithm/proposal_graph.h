#pragma once

#include <map>

#include "platform/consensus/ordering/cassandra_cft/algorithm/proposal_state.h"
#include "platform/consensus/ordering/cassandra_cft/algorithm/ranking.h"
#include "platform/consensus/ordering/cassandra_cft/proto/proposal.pb.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace cassandra_cft {

class ProposalGraph {
 public:
  ProposalGraph(int fault_num);
  inline void SetCommitCallBack(std::function<void(const Proposal&)> func) {
    commit_callback_ = func;
  }

  int AddProposal(const Proposal& proposal);
  void AddProposalOnly(const Proposal& proposal);

  Proposal* GetLatestStrongestProposal();
  const Proposal* GetProposalInfo(const std::string& hash) const;

  int GetCurrentHeight();

  void Clear(const std::string& hash);
  void IncreaseHeight();
  ProposalState GetProposalState(const std::string& hash) const;

  std::vector<std::unique_ptr<Proposal>> GetNotFound(int height,
                                                     const std::string& hash);

  std::vector<Proposal*> GetNewProposals(int height);

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
  void TryUpgradeHeight(int height);

  void Commit(const std::string& hash);

 private:
  Proposal latest_commit_;
  std::map<std::string, std::vector<std::string>> g_;
  std::map<std::string, std::unique_ptr<NodeInfo>> node_info_;
  std::map<std::string, std::vector<VoteMessage>> not_found_;
  std::unique_ptr<Ranking> ranking_;
  std::map<int, int> commit_num_;
  std::map<int, std::set<std::string>> last_node_;
  int current_height_;
  uint32_t f_;
  std::function<void(const Proposal&)> commit_callback_;
  std::map<int, std::set<int>> pending_header_;
  std::map<int, std::map<std::string, std::vector<std::unique_ptr<Proposal>>>>
      not_found_proposal_;
  std::map<std::string, Proposal*> new_proposals_;
  Stats* global_stats_;
};

}  // namespace cassandra_cft
}  // namespace resdb

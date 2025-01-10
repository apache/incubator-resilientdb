#pragma once

#include <condition_variable>
#include <list>

#include "platform/consensus/ordering/cassandra_cft/algorithm/proposal_graph.h"
#include "platform/consensus/ordering/cassandra_cft/proto/proposal.pb.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace cassandra_cft {

struct ProposalInfo {
  std::unique_ptr<Proposal> proposal;
  std::set<int> vote;
  ProposalInfo(std::unique_ptr<Proposal> proposal) : proposal(std::move(proposal)){}
};

class ProposalManager {
 public:
  ProposalManager(int32_t id, int f, int total_num, ProposalGraph* graph);

  std::unique_ptr<Proposal> MakeBlock(
      std::vector<std::unique_ptr<Transaction>>& txns);

  void GeneateGraph(Proposal * proposal);
  NodeInfo GetNodeInfo(const Proposal& proposal);
  std::unique_ptr<Proposal> FetchProposal(const NodeInfo& info);
  bool Ready() ;
  void AddProposal(std::unique_ptr<Proposal> proposal);
  void AddVote(const Proposal& proposal);
  void GenerateVoteInfo(Proposal * proposal);
int CurrentRound();
bool MayReady(int round);

  void CommitProposal(std::unique_ptr<Proposal> proposal);
  void SetCommitCallBack(std::function<void(std::unique_ptr<Proposal>)> func) {
    commit_func_ = func;
  }

  bool Ready(int round);
  void CheckVote(int round);

 private:
  int32_t id_;
  int f_;
  int total_num_;
  ProposalGraph* graph_;
  int64_t local_proposal_id_ = 1, local_block_id_ = 1;
  int round_;

  std::map<int, NodeInfo > proposals_;
  std::map<int, std::map<int, std::unique_ptr<ProposalInfo>> > proposals_rounds_;
  std::map<int, int> vote_record_;
  std::function<void(std::unique_ptr<Proposal>)> commit_func_;
  std::map<int, int> vote_proposer_;

  std::mutex mutex_;

  Stats* global_stats_;
};

}  // namespace cassandra_cft
}  // namespace resdb

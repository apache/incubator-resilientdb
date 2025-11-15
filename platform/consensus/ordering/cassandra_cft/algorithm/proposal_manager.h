#pragma once

#include <condition_variable>
#include <list>

#include "platform/consensus/ordering/cassandra_cft/algorithm/proposal_graph.h"
#include "platform/consensus/ordering/cassandra_cft/proto/proposal.pb.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace cassandra {
namespace cassandra_recv {

class ProposalManager {
 public:
  ProposalManager(int32_t id, ProposalGraph* graph);

  int VerifyProposal(const Proposal& proposal);

  void AddLocalBlock(std::unique_ptr<Block> block);
  void AddBlock(std::unique_ptr<Block> block);
  std::unique_ptr<Block> MakeBlock(
      std::vector<std::unique_ptr<Transaction>>& txn);

  std::unique_ptr<Proposal> GenerateProposal(int round, bool need_empty);
  int CurrentRound();

  void ClearProposal(const Proposal& p);
  std::unique_ptr<Block> GetBlock(const std::string& hash, int sender);
  Block* GetBlockSnap(const std::string& hash, int sender);

  bool ContainBlock(const std::string& hash, int sender);
  bool ContainBlock(const Block& block);
  bool WaitBlock();
  void BlockReady(const std::string& hash, int local_id);
  const Block* QueryBlock(const std::string& hash);
  std::unique_ptr<ProposalQueryResp> QueryProposal(const std::string& hash);
  bool VerifyProposal(const Proposal* proposal);
  void AddTmpProposal(std::unique_ptr<Proposal> proposal);
  void ReleaseTmpProposal(const Proposal& proposal);
  int VerifyProposalHistory(const Proposal* p);
  void AddLocalProposal(const Proposal& proposal);
  void RemoveLocalProposal(const std::string& hash);

  int VerifyProposal(const ProposalQueryResp& resp);

 private:
  void ObtainHistoryProposal(const Proposal* p,
                             std::set<std::pair<int, int>>& v,
                             std::vector<const Proposal*>& resp,
                             int current_height);
  Proposal* GetLocalProposal(const std::string& hash);

 private:
  int32_t id_;
  ProposalGraph* graph_;
  int64_t local_proposal_id_ = 1, local_block_id_ = 1;

  std::map<std::string, std::unique_ptr<Block>> pending_blocks_[512];
  std::list<std::unique_ptr<Block>> blocks_;
  std::mutex mutex_, p_mutex_, q_mutex_;
  std::condition_variable notify_;
  std::map<int, std::unique_ptr<Block>> blocks_candidates_;
  std::map<std::string, std::unique_ptr<Proposal>> tmp_proposal_;

  std::mutex t_mutex_;
  std::map<std::string, std::unique_ptr<Proposal>> local_proposal_;
  Stats* global_stats_;
};

}  // namespace cassandra_recv
}  // namespace cassandra
}  // namespace resdb

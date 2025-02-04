#pragma once

#include <condition_variable>
#include <list>

#include "platform/consensus/ordering/cassandra/algorithm/fast_path/proposal_graph.h"
#include "platform/consensus/ordering/cassandra/proto/proposal.pb.h"

namespace resdb {
namespace cassandra {
namespace cassandra_fp {

class ProposalManager {
 public:
  ProposalManager(int32_t id, int total_num, ProposalGraph* graph);

  bool VerifyProposal(const Proposal& proposal);

  void AddLocalBlock(std::unique_ptr<Block> block);
  void AddBlock(std::unique_ptr<Block> block);
  std::unique_ptr<Block> MakeBlock(
      std::vector<std::unique_ptr<Transaction>>& txn);

  std::unique_ptr<Proposal> GenerateProposal(int round, bool need_empty);
  int CurrentRound();

  void ClearProposal(const Proposal& p);
  std::unique_ptr<Block> GetBlock(const std::string& hash, int sender);
  bool WaitBlock();

  int GetNextLeader(int height);
  void Confirm(int height, int leader);

 private:
  int32_t id_;
  int total_num_;
  ProposalGraph* graph_;
  int64_t local_proposal_id_ = 1, local_block_id_ = 1;

  std::map<std::string, std::unique_ptr<Block>> pending_blocks_[512];
  std::list<std::unique_ptr<Block>> blocks_;
  std::mutex mutex_, p_mutex_;
  std::condition_variable notify_;
};

}  // namespace cassandra_fp
}  // namespace cassandra
}  // namespace resdb

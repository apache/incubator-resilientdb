#pragma once

#include "platform/consensus/ordering/cassandra/algorithm/basic/miner.h"
#include "platform/consensus/ordering/cassandra/algorithm/basic/proposal_graph.h"
#include "platform/consensus/ordering/cassandra/proto/proposal.pb.h"

namespace resdb {
namespace cassandra {
namespace basic {

class ProposalManager {
 public:
  ProposalManager(int32_t id, ProposalGraph* graph);

  std::unique_ptr<Proposal> GenerateProposal(
      const std::vector<Transaction*>& txns);

  bool VerifyProposal(const Proposal& proposal);

 private:
  int32_t id_;
  ProposalGraph* graph_;
  int64_t local_proposal_id_ = 1;

  std::unique_ptr<Miner> miner_;
};

}  // namespace basic
}  // namespace cassandra
}  // namespace resdb

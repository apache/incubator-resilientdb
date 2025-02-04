#pragma once

#include "platform/consensus/ordering/cassandra/algorithm/miner.h"
#include "platform/consensus/ordering/cassandra/algorithm/proposal_graph.h"
#include "platform/consensus/ordering/cassandra/proto/proposal.pb.h"

namespace resdb {
namespace cassandra {

class ProposalManagerMem {
 public:
  ProposalManagerMem(int32_t id, ProposalGraph* graph);

  std::unique_ptr<Proposal> GenerateProposal(
      const std::vector<Transaction*>& txns);

  bool VerifyProposal(const Proposal& proposal);

 private:
  int32_t id_;
  ProposalGraph* graph_;
  int64_t local_proposal_id_ = 1;

  std::unique_ptr<Miner> miner_;
};

}  // namespace cassandra
}  // namespace resdb

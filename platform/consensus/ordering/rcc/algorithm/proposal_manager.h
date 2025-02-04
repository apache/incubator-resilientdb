#pragma once

#include "platform/consensus/ordering/rcc/proto/proposal.pb.h"

namespace resdb {
namespace rcc {

class ProposalManager {
 public:
  ProposalManager(int32_t id);

  std::unique_ptr<Proposal> GenerateProposal(
      const std::vector<std::unique_ptr<Transaction>>& txns);

  int64_t CurrentSeq();

 private:
  int32_t id_;
  int64_t seq_ = 1;
};

}  // namespace rcc
}  // namespace resdb

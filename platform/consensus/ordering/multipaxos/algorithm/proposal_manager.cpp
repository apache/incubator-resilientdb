#include "platform/consensus/ordering/multipaxos/algorithm/proposal_manager.h"

#include <glog/logging.h>
#include "common/utils/utils.h"
#include "common/crypto/signature_verifier.h"

namespace resdb {
namespace multipaxos {

ProposalManager::ProposalManager(int32_t id, int limit_count, SignatureVerifier* verifier)
    : id_(id) {
    round_ = 1;
    seq_ = 1;
}

std::unique_ptr<Proposal> ProposalManager::GenerateProposal(
    const std::vector<std::unique_ptr<Transaction>>& txns) {
  std::unique_ptr<Proposal> proposal = std::make_unique<Proposal>();
  {
    for(const auto& txn: txns){
      *proposal->add_transactions() = *txn;
    }
    proposal->mutable_header()->set_proposal_id(seq_++);
    proposal->mutable_header()->set_proposer(id_);
    proposal->mutable_header()->set_view(round_++);
    proposal->set_sender(id_);
  }
  return proposal;
}

int ProposalManager::CurrentView(){
  return round_;
}
}  // namespace tusk
}  // namespace resdb

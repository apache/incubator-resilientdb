#include "platform/consensus/ordering/rcc/protocol/proposal_manager.h"

#include <glog/logging.h>

#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"

namespace resdb {
namespace rcc {

ProposalManager::ProposalManager(int32_t id) : id_(id) { seq_ = 1; }

std::unique_ptr<Proposal> ProposalManager::GenerateProposal(
    const std::vector<std::unique_ptr<Transaction>>& txns) {
  std::unique_ptr<Proposal> proposal = std::make_unique<Proposal>();
  std::string data;

  for (const auto& txn : txns) {
    *proposal->add_transactions() = *txn;
    std::string hash;
    txn->SerializeToString(&hash);
    data += hash;
  }

  proposal->mutable_header()->set_proposer_id(id_);
  proposal->mutable_header()->set_sender(id_);
  proposal->mutable_header()->set_seq(seq_++);
  proposal->mutable_header()->set_create_time(GetCurrentTime());

  data += std::to_string(proposal->header().proposer_id()) +
          std::to_string(proposal->header().seq());

  std::string hash = SignatureVerifier::CalculateHash(data);
  proposal->mutable_header()->set_hash(hash);
  return proposal;
}

}  // namespace rcc
}  // namespace resdb

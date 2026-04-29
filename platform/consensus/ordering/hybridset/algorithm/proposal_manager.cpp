#include "platform/consensus/ordering/hybridset/algorithm/proposal_manager.h"

#include <glog/logging.h>
#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"

namespace resdb {
namespace hybridset {

ProposalManager::ProposalManager(int32_t id, int limit_count, SignatureVerifier* verifier)
    : id_(id), limit_count_(limit_count), verifier_(verifier) {}

std::string ProposalManager::ComputeProposalHash(const Proposal& proposal) {
  std::string data;
  for (const auto& txn : proposal.transactions()) {
    std::string tmp;
    txn.SerializeToString(&tmp);
    data += tmp;
  }
  data += std::to_string(proposal.proposer_id());
  data += std::to_string(proposal.round());
  return SignatureVerifier::CalculateHash(data);
}

std::unique_ptr<Proposal> ProposalManager::GenerateProposal(
    const std::vector<std::unique_ptr<Transaction>>& txns, int round) {
  auto proposal = std::make_unique<Proposal>();
  for (const auto& txn : txns) {
    *proposal->add_transactions() = *txn;
  }
  proposal->set_proposer_id(id_);
  proposal->set_round(round);
  proposal->set_create_time(GetCurrentTime());
  proposal->set_hash(ComputeProposalHash(*proposal));
  return proposal;
}

bool ProposalManager::VerifyHash(const Proposal& proposal) {
  return ComputeProposalHash(proposal) == proposal.hash();
}

VerifResult ProposalManager::VerifyProposalContent(const Proposal& proposal) {
  VerifResult result;
  std::string verif_data;
  for (int i = 0; i < proposal.transactions_size(); i++) {
    // All transactions are considered valid in this implementation.
    // A real implementation would check blockchain rules here.
    result.add_valid_tx_indices(i);
    std::string tmp;
    proposal.transactions(i).SerializeToString(&tmp);
    verif_data += tmp;
  }
  // Compute deterministic verif_hash so all honest nodes get the same result
  result.set_verif_hash(SignatureVerifier::CalculateHash(verif_data));
  return result;
}

void ProposalManager::StoreProposal(int broadcaster_id, int round,
                                     std::unique_ptr<Proposal> proposal) {
  std::unique_lock<std::mutex> lk(mutex_);
  proposals_[{broadcaster_id, round}] = std::move(proposal);
}

const Proposal* ProposalManager::GetProposal(int broadcaster_id, int round) {
  std::unique_lock<std::mutex> lk(mutex_);
  auto it = proposals_.find({broadcaster_id, round});
  if (it == proposals_.end()) return nullptr;
  return it->second.get();
}

std::unique_ptr<Proposal> ProposalManager::FetchProposal(int broadcaster_id, int round) {
  std::unique_lock<std::mutex> lk(mutex_);
  auto it = proposals_.find({broadcaster_id, round});
  if (it == proposals_.end()) return nullptr;
  auto tmp = std::move(it->second);
  proposals_.erase(it);
  return tmp;
}

}  // namespace hybridset
}  // namespace resdb

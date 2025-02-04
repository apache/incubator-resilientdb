#include "platform/consensus/ordering/cassandra/algorithm/basic/proposal_manager.h"

#include <glog/logging.h>

#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"

namespace resdb {
namespace cassandra {
namespace basic {

ProposalManager::ProposalManager(int32_t id, ProposalGraph* graph)
    : id_(id), graph_(graph) {
  local_proposal_id_ = 1;
  // miner_ = std::make_unique<Miner>();
}

std::unique_ptr<Proposal> ProposalManager::GenerateProposal(
    const std::vector<Transaction*>& txns) {
  std::unique_ptr<Proposal> proposal = std::make_unique<Proposal>();

  std::string data;
  for (int i = 0; i < txns.size(); ++i) {
    data.append(txns[i]->data());
    *proposal->add_transactions() = *txns[i];
  }

  LOG(ERROR) << "generate proposal:";
  Proposal* last = graph_->GetLatestStrongestProposal();
  graph_->IncreaseHeight();
  if (last != nullptr) {
    proposal->mutable_header()->set_prehash(last->header().hash());
    proposal->mutable_header()->set_height(last->header().height() + 1);
    *proposal->mutable_history() = last->history();
  }

  proposal->mutable_header()->set_proposer_id(id_);
  proposal->mutable_header()->set_proposal_id(local_proposal_id_++);
  proposal->set_create_time(GetCurrentTime());
  data += std::to_string(proposal->header().proposal_id()) +
          std::to_string(proposal->header().proposer_id()) +
          std::to_string(proposal->header().height());

  std::string hash = SignatureVerifier::CalculateHash(data);
  proposal->mutable_header()->set_hash(hash);
  LOG(ERROR) << "mine";
  // auto nonce_it = miner_->Mine(proposal->header());
  // assert(nonce_it.ok());
  // proposal->set_nonce(*nonce_it);

  return proposal;
}

bool ProposalManager::VerifyProposal(const Proposal& proposal) {
  return true;
  return miner_->Verify(proposal.header(), proposal.nonce());
}

}  // namespace basic
}  // namespace cassandra
}  // namespace resdb

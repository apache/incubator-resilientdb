#include "platform/consensus/ordering/damysus/algorithm/proposal_manager.h"

#include <glog/logging.h>
#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"

namespace resdb {
namespace damysus {

ProposalManager::ProposalManager(int32_t id, int limit_count, SignatureVerifier* verifier)
    : id_(id), limit_count_(limit_count), verifier_(verifier) {
  view_ = 0;
}

int ProposalManager::CurrentView() { return view_; }

void ProposalManager::SetView(int view) { view_ = view; }

void ProposalManager::AdvanceView() { view_++; }

std::unique_ptr<Proposal> ProposalManager::GenerateProposal(
    const std::vector<std::unique_ptr<Transaction>>& txns,
    const Accumulator& acc,
    const Commitment& block_cert) {
  auto proposal = std::make_unique<Proposal>();

  for (const auto& txn : txns) {
    *proposal->add_transactions() = *txn;
  }

  proposal->mutable_header()->set_proposer_id(id_);
  proposal->mutable_header()->set_view(view_);
  proposal->mutable_header()->set_create_time(GetCurrentTime());
  proposal->mutable_header()->set_parent_hash(acc.block_hash());
  *proposal->mutable_header()->mutable_accumulator() = acc;
  *proposal->mutable_header()->mutable_block_cert() = block_cert;
  proposal->set_sender(id_);

  // Compute hash from transactions + core header fields only.
  // Excludes TEE certificates (block_cert, accumulator) which are set after hash.
  proposal->set_hash(ComputeBlockHash(*proposal));

  return proposal;
}

void ProposalManager::AddBlock(std::unique_ptr<Proposal> proposal) {
  std::unique_lock<std::mutex> lk(mutex_);
  blocks_[proposal->hash()] = std::move(proposal);
}

const Proposal* ProposalManager::GetBlock(const std::string& hash) {
  std::unique_lock<std::mutex> lk(mutex_);
  auto it = blocks_.find(hash);
  if (it == blocks_.end()) return nullptr;
  return it->second.get();
}

std::unique_ptr<Proposal> ProposalManager::FetchBlock(const std::string& hash) {
  std::unique_lock<std::mutex> lk(mutex_);
  auto it = blocks_.find(hash);
  if (it == blocks_.end()) return nullptr;
  auto tmp = std::move(it->second);
  blocks_.erase(it);
  return tmp;
}

std::string ProposalManager::ComputeBlockHash(const Proposal& proposal) {
  // Hash is computed from transactions + core header fields (proposer, view, parent_hash).
  // TEE certificates (block_cert, accumulator) are excluded because they are
  // attached after hash computation (TEEprepare needs the hash as input).
  std::string data;
  for (const auto& txn : proposal.transactions()) {
    std::string tmp;
    txn.SerializeToString(&tmp);
    data += tmp;
  }
  // Include core identity fields but not TEE metadata
  data += std::to_string(proposal.header().proposer_id());
  data += std::to_string(proposal.header().view());
  data += proposal.header().parent_hash();

  return SignatureVerifier::CalculateHash(data);
}

bool ProposalManager::VerifyHash(const Proposal& proposal) {
  return ComputeBlockHash(proposal) == proposal.hash();
}

bool ProposalManager::VerifyProposal(const Proposal& proposal) {
  if (!VerifyHash(proposal)) {
    LOG(ERROR) << "Proposal hash verification failed";
    return false;
  }
  // TODO: verify accumulator signature and block_cert TEE signature
  return true;
}

}  // namespace damysus
}  // namespace resdb

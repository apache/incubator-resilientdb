#include "platform/consensus/ordering/achilles/algorithm/proposal_manager.h"

#include <glog/logging.h>
#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"

namespace resdb {
namespace achilles {

ProposalManager::ProposalManager(int32_t id, int limit_count, SignatureVerifier* verifier)
    : id_(id), limit_count_(limit_count), verifier_(verifier) {
  view_ = 0;
}

int ProposalManager::CurrentView() { return view_; }
void ProposalManager::SetView(int view) { view_ = view; }
void ProposalManager::AdvanceView() { view_++; }

std::string ProposalManager::ComputeBlockHash(const Proposal& proposal) {
  std::string data;
  for (const auto& txn : proposal.transactions()) {
    std::string tmp;
    txn.SerializeToString(&tmp);
    data += tmp;
  }
  data += std::to_string(proposal.header().proposer_id());
  data += std::to_string(proposal.header().view());
  data += proposal.header().parent_hash();
  return SignatureVerifier::CalculateHash(data);
}

std::unique_ptr<Proposal> ProposalManager::GenerateProposal(
    const std::vector<std::unique_ptr<Transaction>>& txns,
    const std::string& parent_hash,
    const Accumulator& acc,
    const BlockCertificate& block_cert) {
  auto proposal = std::make_unique<Proposal>();

  for (const auto& txn : txns) {
    *proposal->add_transactions() = *txn;
  }

  proposal->mutable_header()->set_proposer_id(id_);
  proposal->mutable_header()->set_view(view_);
  proposal->mutable_header()->set_create_time(GetCurrentTime());
  proposal->mutable_header()->set_parent_hash(parent_hash);
  *proposal->mutable_header()->mutable_accumulator() = acc;
  *proposal->mutable_header()->mutable_block_cert() = block_cert;
  proposal->set_sender(id_);

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

bool ProposalManager::VerifyHash(const Proposal& proposal) {
  return ComputeBlockHash(proposal) == proposal.hash();
}

bool ProposalManager::VerifyProposal(const Proposal& proposal) {
  if (!VerifyHash(proposal)) {
    LOG(ERROR) << "Achilles: Proposal hash verification failed";
    return false;
  }
  return true;
}

void ProposalManager::MarkCommitted(const std::string& hash) {
  std::unique_lock<std::mutex> lk(mutex_);
  committed_.insert(hash);
}

bool ProposalManager::IsCommitted(const std::string& hash) {
  std::unique_lock<std::mutex> lk(mutex_);
  return committed_.count(hash) > 0;
}

std::vector<std::string> ProposalManager::GetUncommittedAncestors(const std::string& hash) {
  std::unique_lock<std::mutex> lk(mutex_);
  std::vector<std::string> chain;
  std::string current = hash;

  while (!current.empty() && committed_.count(current) == 0) {
    auto it = blocks_.find(current);
    if (it == blocks_.end()) break;
    chain.push_back(current);
    current = it->second->header().parent_hash();
  }

  // Reverse so ancestors come first (oldest to newest)
  std::reverse(chain.begin(), chain.end());
  return chain;
}

}  // namespace achilles
}  // namespace resdb

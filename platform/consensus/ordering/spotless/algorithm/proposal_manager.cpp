#include "platform/consensus/ordering/spotless/algorithm/proposal_manager.h"

#include <glog/logging.h>

#include "common/utils/utils.h"
#include <sstream>

namespace resdb {
namespace spotless {
namespace {

std::string BuildCertPayload(int32_t instance_id, int32_t view,
                             const std::string& hash) {
  return std::to_string(instance_id) + ":" + std::to_string(view) + ":" +
         hash;
}

std::string ShortHash(const std::string& value) {
  if (value.empty()) {
    return "empty";
  }
  static constexpr size_t kPreview = 12;
  if (value.size() <= kPreview) {
    return value;
  }
  return value.substr(0, kPreview);
}

}  // namespace

ProposalManager::ProposalManager(int32_t id, int32_t instance_id,
                                 int32_t quorum_size,
                                 SignatureVerifier* verifier)
    : id_(id),
      instance_id_(instance_id),
      quorum_size_(quorum_size),
      verifier_(verifier),
      current_view_(1) {
  assert(verifier_ != nullptr);
}

std::string ProposalManager::GetHash(const Proposal& proposal) const {
  std::string data;
  std::string header_data;
  proposal.header().SerializeToString(&header_data);
  data += header_data;
  for (const auto& txn : proposal.transactions()) {
    std::string txn_data;
    txn.SerializeToString(&txn_data);
    data += txn_data;
  }
  return SignatureVerifier::CalculateHash(data);
}

bool ProposalManager::VerifyHash(const Proposal& proposal) const {
  return GetHash(proposal) == proposal.hash();
}

bool ProposalManager::VerifyCertificate(const Certificate& cert) const {
  return true;
  if (cert.instance_id() != instance_id_) {
    return false;
  }
  if (cert.hash().empty()) {
    return cert.view() == 0;
  }
  if (cert.signatures_size() < quorum_size_ ||
      cert.signatures_size() != cert.signer_ids_size()) {
    return false;
  }

  std::set<int32_t> signers;
  const std::string payload =
      BuildCertPayload(cert.instance_id(), cert.view(), cert.hash());
  for (int i = 0; i < cert.signatures_size(); ++i) {
    if (!signers.insert(cert.signer_ids(i)).second) {
      return false;
    }
    if (!verifier_->VerifyMessage(payload, cert.signatures(i))) {
      return false;
    }
  }
  return true;
}

bool ProposalManager::SafeNode(const Proposal& proposal) const {
  const Certificate& cert = proposal.header().justify();
  if (locked_cert_.hash().empty()) {
    return true;
  }
  if (cert.view() > locked_cert_.view()) {
    return true;
  }
  return cert.view() == locked_cert_.view() &&
         cert.hash() == locked_cert_.hash();
}

bool ProposalManager::VerifyProposal(const Proposal& proposal) const {
  return true;
  if (proposal.header().instance_id() != instance_id_) {
    return false;
  }
  if (!VerifyHash(proposal)) {
    LOG(ERROR) << "spotless proposal hash mismatch, instance:"
               << proposal.header().instance_id()
               << " view:" << proposal.header().view();
    return false;
  }

  if (proposal.header().view() <= 1) {
    return true;
  }

  if (proposal.header().prehash() != proposal.header().justify().hash()) {
    LOG(ERROR) << "spotless justify/prehash mismatch, instance:"
               << proposal.header().instance_id()
               << " view:" << proposal.header().view();
    return false;
  }
  if (!VerifyCertificate(proposal.header().justify())) {
    LOG(ERROR) << "spotless invalid justify cert, instance:"
               << proposal.header().instance_id()
               << " view:" << proposal.header().view();
    return false;
  }
  if (!SafeNode(proposal)) {
    LOG(ERROR) << "spotless unsafe proposal, instance:"
               << proposal.header().instance_id()
               << " view:" << proposal.header().view();
    return false;
  }
  return true;
}

std::unique_ptr<Proposal> ProposalManager::GenerateProposal(
    int32_t view, const std::vector<std::unique_ptr<Transaction>>& txns) {
  auto proposal = std::make_unique<Proposal>();
  proposal->mutable_header()->set_instance_id(instance_id_);
  proposal->mutable_header()->set_view(view);
  proposal->mutable_header()->set_proposer_id(id_);
  proposal->mutable_header()->set_sender_id(id_);
  proposal->mutable_header()->set_create_time(GetCurrentTime());
  proposal->set_create_time(GetCurrentTime());

  {
    std::unique_lock<std::mutex> lk(mutex_);
    if (!highest_prepared_cert_.hash().empty()) {
      proposal->mutable_header()->set_prehash(highest_prepared_cert_.hash());
      *proposal->mutable_header()->mutable_justify() = highest_prepared_cert_;
    }
  }

  for (const auto& txn : txns) {
    *proposal->add_transactions() = *txn;
  }

  proposal->set_hash(GetHash(*proposal));
  return proposal;
}

void ProposalManager::AdvanceView(int32_t view) {
  std::unique_lock<std::mutex> lk(mutex_);
  int32_t old_view = current_view_;
  if (view > current_view_) {
    current_view_ = view;
    LOG(ERROR) << "[spotless_dbg] advance_view instance:" << instance_id_
               << " from:" << old_view << " to:" << current_view_
               << " target:" << view;
  } else {
    LOG(ERROR) << "[spotless_dbg] advance_view_skip instance:" << instance_id_
               << " current:" << current_view_
               << " target:" << view
               << " reason:target_not_greater";
  }
}

int32_t ProposalManager::CurrentView() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return current_view_;
}

void ProposalManager::RecordProposal(std::unique_ptr<Proposal> proposal) {
  if (proposal == nullptr) {
    return;
  }
  std::unique_lock<std::mutex> lk(mutex_);
  const std::string hash = proposal->hash();
  auto& stored = proposals_[hash];
  if (stored.proposal == nullptr) {
    stored.proposal = std::move(proposal);
  }
}

const Proposal* ProposalManager::GetProposal(const std::string& hash) const {
  std::unique_lock<std::mutex> lk(mutex_);
  auto it = proposals_.find(hash);
  if (it == proposals_.end() || it->second.proposal == nullptr) {
    return nullptr;
  }
  return it->second.proposal.get();
}

Certificate ProposalManager::GetHighestPreparedCert() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return highest_prepared_cert_;
}

Certificate ProposalManager::GetLockedCert() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return locked_cert_;
}

void ProposalManager::UpdateHighestPreparedLocked(const Certificate& cert) {
  if (cert.hash().empty()) {
    return;
  }
  if (highest_prepared_cert_.hash().empty() ||
      cert.view() > highest_prepared_cert_.view() ||
      (cert.view() == highest_prepared_cert_.view() &&
       cert.hash() != highest_prepared_cert_.hash())) {
    highest_prepared_cert_ = cert;
  }
  if (current_view_ <= cert.view()) {
    current_view_ = cert.view() + 1;
    LOG(ERROR)<<"update view:"<<current_view_;
  }
}

void ProposalManager::UpdateLockedLocked(const Certificate& cert) {
  if (cert.hash().empty()) {
    return;
  }
  if (locked_cert_.hash().empty() || cert.view() > locked_cert_.view() ||
      (cert.view() == locked_cert_.view() &&
       cert.hash() != locked_cert_.hash())) {
    locked_cert_ = cert;
  }
}

bool ProposalManager::UpdateHighestPrepared(const Certificate& cert) {
  if (!VerifyCertificate(cert)) {
    return false;
  }
  std::unique_lock<std::mutex> lk(mutex_);
  const Certificate before = highest_prepared_cert_;
  UpdateHighestPreparedLocked(cert);
  return before.view() != highest_prepared_cert_.view() ||
         before.hash() != highest_prepared_cert_.hash();
}

std::unique_ptr<Proposal> ProposalManager::MarkPrepared(const Certificate& cert) {
  if (!VerifyCertificate(cert)) {
    LOG(ERROR) << "[spotless_dbg] mark_prepared_reject instance:" << instance_id_
               << " cert_view:" << cert.view();
    return nullptr;
  }

  std::unique_lock<std::mutex> lk(mutex_);
  auto it = proposals_.find(cert.hash());
  if (it == proposals_.end() || it->second.proposal == nullptr) {
    LOG(ERROR) << "[spotless_dbg] mark_prepared_reject instance:" << instance_id_
               << " cert_view:" << cert.view();
    return nullptr;
  }

  StoredProposal& node = it->second;
  if (!node.prepared) {
    node.prepared = true;
    node.prepared_cert = cert;
  }
  LOG(ERROR) << "[spotless_dbg] mark_prepared instance:" << instance_id_
             << " cert_view:" << cert.view()
             << " proposal_view:" << node.proposal->header().view();
  UpdateHighestPreparedLocked(cert);

  const Proposal* child = node.proposal.get();
  const Proposal* parent = nullptr;
  if (!child->header().prehash().empty()) {
    auto parent_it = proposals_.find(child->header().prehash());
    if (parent_it != proposals_.end() && parent_it->second.proposal != nullptr) {
      parent = parent_it->second.proposal.get();
      UpdateLockedLocked(child->header().justify());
    }
  }

  if (parent == nullptr || parent->header().prehash().empty()) {
    LOG(ERROR) << "[spotless_dbg] mark_commit_wait instance:" << instance_id_
               << " child_view:" << child->header().view()
               << " reason:parent_or_grandparent_missing";
    return nullptr;
  }

  auto grandparent_it = proposals_.find(parent->header().prehash());
  if (grandparent_it == proposals_.end() ||
      grandparent_it->second.proposal == nullptr ||
      grandparent_it->second.committed) {
    LOG(ERROR) << "[spotless_dbg] mark_commit_wait instance:" << instance_id_
               << " child_view:" << child->header().view();
    return nullptr;
  }

  grandparent_it->second.committed = true;
  LOG(ERROR) << "[spotless_dbg] mark_commit_ready instance:" << instance_id_
             << " commit_view:" << grandparent_it->second.proposal->header().view();
  return std::make_unique<Proposal>(*grandparent_it->second.proposal);
}

}  // namespace spotless
}  // namespace resdb

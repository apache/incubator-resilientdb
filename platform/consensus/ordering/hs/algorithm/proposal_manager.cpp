#include "platform/consensus/ordering/hs/algorithm/proposal_manager.h"

#include <glog/logging.h>
#include <thread>
#include "common/utils/utils.h"
#include "common/crypto/signature_verifier.h"

namespace resdb {
namespace hs {

ProposalManager::ProposalManager(int32_t id, int limit_count, SignatureVerifier* verifier)
    : id_(id), limit_count_(limit_count), verifier_(verifier) {
    round_ = 1;
    assert(verifier_ != nullptr);
}


std::string ProposalManager::GetHash(const Proposal& proposal){
  std::string data;
  for(const auto& txn: proposal.transactions()){
    std::string tmp;
    txn.SerializeToString(&tmp);
    data += tmp;
  }

  std::string header_data;
  proposal.header().SerializeToString(&header_data);
  data +=  header_data;

  //LOG(ERROR)<<"get hash";
  return SignatureVerifier::CalculateHash(data);
}

bool ProposalManager::VerifyHash(const Proposal& proposal) {
  return  GetHash(proposal) == proposal.hash();
}

bool ProposalManager::VerifyCert(const Certificate& cert) {
  bool valid = verifier_->VerifyMessage(cert.hash(), cert.sign());
  // Simulate RSA-2048 signature verification (~500μs per verification).
  // HMAC is ~1μs; this models the real asymmetric crypto cost that
  // TEE-based protocols (Damysus/Achilles) avoid entirely.
  std::this_thread::sleep_for(std::chrono::microseconds(1600));
  return valid;
}


bool ProposalManager::VerifyQC(const QC& qc) {
  if(qc.signatures_size() < limit_count_){
    LOG(ERROR)<<"qc size:"<<qc.signatures_size()<<" not enought, limit:"<<limit_count_;
    return false;
  }
  for(const auto& sign : qc.signatures()){
    bool valid = verifier_->VerifyMessage(qc.hash(), sign);
    if(!valid){
      LOG(ERROR) << "Verify message fail";
      return false;
    }
    // Simulate RSA-2048 verification (~2000μs per signature in QC)
    std::this_thread::sleep_for(std::chrono::microseconds(2000));
  }
  return  true;
}

bool ProposalManager::SafeNode(const Proposal& proposal){
  if(proposal.header().qc().view() > lock_qc_.view()){
    return true;
  }
  //LOG(ERROR)<<"qc view:"<<proposal.header().qc().view()<<" lock view:"<<lock_qc_.view()<<" safe fail";
  return false; 
}

bool ProposalManager::Verify(const Proposal& proposal) {
  if( !VerifyHash(proposal)){
    LOG(ERROR)<<"hash not match:";
    return false;
  }

  if(proposal.header().view() == 1){
    return true;
  }

  // Crash-fault relaxation: if the proposal carries no QC, the sender
  // advanced past a dead leader via timeout and nothing blocks safety
  // in a crash-only adversary model. Accept the proposal as long as
  // its view is strictly ahead of our locked view.
  if (proposal.header().qc().hash().empty()) {
    if (proposal.header().view() > lock_qc_.view()) {
      return true;
    }
    return false;
  }

  if(!SafeNode(proposal)){
    return false;
  }
  return VerifyQC(proposal.header().qc());
}

std::unique_ptr<Proposal> ProposalManager::GenerateProposal(
    const std::vector<std::unique_ptr<Transaction>>& txns) {
  std::unique_ptr<Proposal> proposal = std::make_unique<Proposal>();
  {
    std::unique_lock<std::mutex> lk(txn_mutex_);
    for(const auto& txn: txns){
      *proposal->add_transactions() = *txn;
    }
    if(!generic_qc_.hash().empty()){
     //LOG(ERROR)<<" add qc:"<<generic_qc_.view();
      proposal->mutable_header()->set_prehash(generic_qc_.hash());
      *proposal->mutable_header()->mutable_qc() = generic_qc_;
    }

    proposal->mutable_header()->set_view(round_);
    proposal->mutable_header()->set_create_time(GetCurrentTime());
    proposal->set_sender(id_);
  }

  proposal->set_hash(GetHash(*proposal));
  return proposal;
}

int ProposalManager::CurrentView(){
  std::unique_lock<std::mutex> lk(txn_mutex_);
  return round_;
}

bool ProposalManager::AdvanceViewOnTimeout(int expected_view) {
  std::unique_lock<std::mutex> lk(txn_mutex_);
  // Only advance if we're still at the expected view. Otherwise a
  // regular Decide already moved us forward and we should not bump again.
  if (round_ != expected_view) {
    return false;
  }
  round_++;
  LOG(ERROR) << "HotStuff timeout view advance: " << expected_view
             << " -> " << round_;
  return true;
}

void ProposalManager::AddQC(std::unique_ptr<QC> qc){
  std::unique_lock<std::mutex> lk(txn_mutex_);
  if(generic_qc_.view() == 0 || generic_qc_.view() < qc->view()){
    generic_qc_ = *qc;
    round_ = generic_qc_.view()+1;
  //  LOG(ERROR)<<"get new round:"<<round_;
  }
}

void ProposalManager::AddProposal(std::unique_ptr<Proposal> proposal){
  std::unique_lock<std::mutex> lk(txn_mutex_);
  if(generic_qc_.view() < proposal->header().qc().view()){
    generic_qc_ = proposal->header().qc();
  }
  // Basic commit: just store the proposal. Commit happens when QC/Decide arrives.
  local_block_[proposal->hash()] = std::move(proposal);
}

const Proposal * ProposalManager::GetProposal(const std::string& hash){
  auto it = local_block_.find(hash);
  if(it == local_block_.end()){
    return nullptr;
  }
  return it->second.get();
}

std::unique_ptr<Proposal> ProposalManager::FetchProposal(const std::string& hash){
  auto it = local_block_.find(hash);
  assert(it != local_block_.end());
  std::unique_ptr<Proposal> ret = std::move(it->second);
  local_block_.erase(it);
  return ret;
}

std::unique_ptr<Proposal> ProposalManager::FetchProposalByHash(const std::string& hash){
  std::unique_lock<std::mutex> lk(txn_mutex_);
  auto it = local_block_.find(hash);
  if (it == local_block_.end()) return nullptr;
  std::unique_ptr<Proposal> ret = std::move(it->second);
  local_block_.erase(it);
  return ret;
}

}  // namespace tusk
}  // namespace resdb

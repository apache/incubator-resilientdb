#include "platform/consensus/ordering/slot_hs1/algorithm/proposal_manager.h"

#include <glog/logging.h>
#include "common/utils/utils.h"
#include "common/crypto/signature_verifier.h"

namespace resdb {
namespace slot_hs1 {

ProposalManager::ProposalManager(int32_t id, int limit_count, int slot_num, SignatureVerifier* verifier, int total_num, int fork_tail_num, int rollback_num)
    : id_(id), limit_count_(limit_count), slot_num_(slot_num), verifier_(verifier), total_num_(total_num), fork_tail_num_(fork_tail_num), rollback_num_(rollback_num) {
    round_ = 1;
    global_stats_ = Stats::GetGlobalStats();
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
  return verifier_->VerifyMessage(cert.hash(), cert.sign());
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
  }
  return  true;
}

bool ProposalManager::SafeNode(const Proposal& proposal){
  if(proposal.header().qc().view() > lock_qc_.view()){
    return true;
  } else if (proposal.header().qc().view() == lock_qc_.view() 
            && proposal.header().qc().slot() >= lock_qc_.slot()){
    return true;
  }
  LOG(ERROR)<<"qc view:"<<proposal.header().qc().view()<<" lock view:"<<lock_qc_.view()<<" safe fail";
   LOG(ERROR)<<"qc slot:"<<proposal.header().qc().slot()<<" lock slot:"<<lock_qc_.slot()<<" safe fail";
  return false; 
}

bool ProposalManager::Verify(const Proposal& proposal) {
  if(!VerifyHash(proposal)){
    LOG(ERROR)<<"hash not match:";
    return false;
  }

  if(proposal.header().view() == 1){
    return true;
  }

  if(!SafeNode(proposal)){
    return false;
  }
  return VerifyQC(proposal.header().qc());
}

std::unique_ptr<Proposal> ProposalManager::GenerateFakeProposal(
    const std::vector<std::unique_ptr<Transaction>>& txns, int slot, bool is_final) {
  std::unique_ptr<Proposal> proposal = std::make_unique<Proposal>();
  {
    std::unique_lock<std::mutex> lk(txn_mutex_);
    for(const auto& txn: txns){
      global_stats_->AddProposeLatency(GetCurrentTime() - txn->reception_time());
      *proposal->add_transactions() = *txn;
    }
    if(!fake_generic_qc_.hash().empty()){
     //LOG(ERROR)<<" add qc:"<<generic_qc_.view();
      proposal->mutable_header()->set_prehash(fake_generic_qc_.hash());
      *proposal->mutable_header()->mutable_qc() = fake_generic_qc_;
    }
    proposal->mutable_header()->set_view(round_);
    proposal->set_sender(id_);
  }
  proposal->set_createtime(GetCurrentTime());
  proposal->mutable_header()->set_slot(slot);
  proposal->mutable_header()->set_is_final(is_final);
  proposal->set_hash(GetHash(*proposal));
  return proposal;
}

std::unique_ptr<Proposal> ProposalManager::GenerateProposal(
    const std::vector<std::unique_ptr<Transaction>>& txns, int slot, bool is_final) {
  std::unique_ptr<Proposal> proposal = std::make_unique<Proposal>();
  {
    std::unique_lock<std::mutex> lk(txn_mutex_);
    for(const auto& txn: txns){
      global_stats_->AddProposeLatency(GetCurrentTime() - txn->reception_time());
      *proposal->add_transactions() = *txn;
    }
    if(!generic_qc_.hash().empty()){
    //  LOG(ERROR)<<" add qc view:"<<generic_qc_.view() << " slot:" << generic_qc_.slot();
      proposal->mutable_header()->set_prehash(generic_qc_.hash());
      *proposal->mutable_header()->mutable_qc() = generic_qc_;
    }

    proposal->mutable_header()->set_view(round_);
    proposal->set_sender(id_);
  }
  proposal->set_createtime(GetCurrentTime());
  proposal->mutable_header()->set_slot(slot);
  proposal->mutable_header()->set_is_final(is_final);
  proposal->set_hash(GetHash(*proposal));
  return proposal;
}

int ProposalManager::CurrentView(){
  return round_;
}

void ProposalManager::AddQC(std::unique_ptr<QC> qc){
  std::unique_lock<std::mutex> lk(txn_mutex_);
  if(generic_qc_.view() == 0 || generic_qc_.view() < qc->view() || (generic_qc_.view() == qc->view() && generic_qc_.slot() < qc->slot())){
    int leader = GetLeader(qc->view());
    if(!(leader < 3 * fork_tail_num_ && leader % 3 == 1 && qc->is_final())) {
      generic_qc_ = *qc;
    } 

    if (fork_tail_num_) {
      // IGNORE the QC for the victim proposal
      if(!(leader < 3 * fork_tail_num_ && leader % 3 ==1 && qc->is_final())) {
        generic_qc_ = *qc;
      } 
    } else if (rollback_num_) {
      if(id_ < 3 * rollback_num_ && id_ % 3 ==1 && qc->is_final()) {
        fake_generic_qc_ = *qc;
      } else {
        generic_qc_ = *qc;
      }
    } else {
      generic_qc_ = *qc;
    } 

    if(qc->is_final()){
      round_ = qc->view()+1;
    }
  //  LOG(ERROR)<<"get new round:"<<round_;
  }
}

std::unique_ptr<Proposal> ProposalManager::AddProposal(std::unique_ptr<Proposal> proposal){
  //LOG(ERROR)<<"ADD PROPOSAL";
  std::unique_lock<std::mutex> lk(txn_mutex_);
  if(generic_qc_.view() < proposal->header().qc().view() || 
      generic_qc_.view() == proposal->header().qc().view() && generic_qc_.slot() <= proposal->header().qc().slot()){
    generic_qc_ = proposal->header().qc();
    lock_qc_ = proposal->header().qc();
  }

  std::unique_ptr<Proposal> commit_ready_proposal_ = nullptr;
  const Proposal * father = nullptr;
  father = GetProposal(proposal->header().prehash());
  if(father != nullptr){
    //LOG(ERROR)<<"get father view:"<<father->header().view();
    // fafather = GetProposal(father->header().prehash());
    // if(father != nullptr){
     // LOG(ERROR)<<"get fafather view:"<<fafather->header().view();
      // fafafather = GetProposal(fafather->header().prehash());
      commit_ready_proposal_ = FetchProposal(proposal->header().prehash());
      // if(fafafather != nullptr){
      //   //commit;
      // //  LOG(ERROR)<<"commit fafafather view:"<<fafafather->header().view();
      //   commit_ready_proposal_ = FetchProposal(fafather->header().prehash());
      // }
    // }
  }
  local_block_[proposal->hash()] = std::move(proposal);
  return commit_ready_proposal_;
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

int ProposalManager::GetLeader(int view){
  //LOG(ERROR)<<" view:"<<view<<" next leader:"<<(view+1)%total_num_ + 1;
  return view % total_num_ + 1;
}

}  // namespace slot_hs1
}  // namespace resdb
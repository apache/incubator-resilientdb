#include "platform/consensus/ordering/ooo_hs/algorithm/proposal_manager.h"

#include <glog/logging.h>
#include "common/utils/utils.h"
#include "common/crypto/signature_verifier.h"

namespace resdb {
namespace ooohs {

ProposalManager::ProposalManager(int32_t id, int limit_count, int slot_num, SignatureVerifier* verifier)
    : id_(id), limit_count_(limit_count), slot_num_(slot_num), verifier_(verifier) {
    round_.resize(slot_num_);
    for(int i = 0; i < slot_num_; ++i){
      round_[i] = 1;
    }
    lock_qc_.resize(slot_num_);
    generic_qc_.resize(slot_num_);
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
  int slot = proposal.header().qc().slot();
  if(proposal.header().qc().view() > lock_qc_[slot].view()){
    return true;
  }
  //LOG(ERROR)<<"qc view:"<<proposal.header().qc().view()<<" lock view:"<<lock_qc_[slot].view()<<" safe fail";
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

  if(!SafeNode(proposal)){
    return false;
  }
  return VerifyQC(proposal.header().qc());
}

std::unique_ptr<Proposal> ProposalManager::GenerateProposal(
    const std::vector<std::unique_ptr<Transaction>>& txns, int slot) {
  std::unique_ptr<Proposal> proposal = std::make_unique<Proposal>();
  {
    std::unique_lock<std::mutex> lk(txn_mutex_);
    for(const auto& txn: txns){
      *proposal->add_transactions() = *txn;
    }
    if(!generic_qc_[slot].hash().empty()){
     //LOG(ERROR)<<" new proposal add qc:"<<generic_qc_[slot].view()<<" slot:"<<generic_qc_[slot].slot()<<" get slot:"<<slot;
      proposal->mutable_header()->set_prehash(generic_qc_[slot].hash());
      *proposal->mutable_header()->mutable_qc() = generic_qc_[slot];
      assert(generic_qc_[slot].slot() >= slot);
    }

    proposal->mutable_header()->set_slot(slot);
    proposal->mutable_header()->set_view(round_[slot]);
    proposal->set_sender(id_);
  }

  proposal->set_hash(GetHash(*proposal));
  return proposal;
}

int ProposalManager::CurrentView(int slot){
  return round_[slot];
}

void ProposalManager::AddQC(std::unique_ptr<QC> qc){
  std::unique_lock<std::mutex> lk(txn_mutex_);
  UpdateGenericQC(*qc);
}

void ProposalManager::UpdateGenericQC(const QC& qc){

int view = qc.view();
  int slot = qc.slot();
  if(view == 0){
    return;
  }

  for(int i = 0; i <= slot; ++i){
    if(generic_qc_[i].view() == 0 || generic_qc_[i].view() < view){
      generic_qc_[i] = qc;
      round_[i] = generic_qc_[i].view()+1;
      //LOG(ERROR)<<"get new round:"<<round_[i]<<" slot:"<<i<<" gene view:"<<generic_qc_[i].view()<<" slot:"<<generic_qc_[i].slot();
    }
  }

/*
  for(int i = 0; i < slot_num_; ++i){
    if(generic_qc_[i].view()>0){
      LOG(ERROR)<<"slot i:"<<i<<" view:"<<generic_qc_[i].view()<<" slot:"<<generic_qc_[i].slot();
      assert(generic_qc_[i].slot() >=i);
    }
  }
  */
}

std::vector<std::unique_ptr<Proposal>> ProposalManager::AddProposal(std::unique_ptr<Proposal> proposal){
    int view = proposal->header().view();
    int slot = proposal->header().slot();

  //LOG(ERROR)<<" add protocol view:"<<view<<" slot:"<<slot;
  std::string hash = proposal->hash();
  //LOG(ERROR)<<"ADD PROPOSAL";
  std::unique_lock<std::mutex> lk(txn_mutex_);
  UpdateGenericQC(proposal->header().qc());

  std::vector<std::unique_ptr<Proposal>> commit_ready_proposal_list;
  const Proposal * father = nullptr, * fafather = nullptr, * fafafather = nullptr;
  father = GetProposal(proposal->header().prehash());
  if(father != nullptr){
    //LOG(ERROR)<<"get father view:"<<father->header().view()<<" slot:"<<father->header().slot();
    fafather = GetProposal(father->header().prehash());
    for(int i = 0; i <=slot; ++i){
      if(lock_qc_[i].view() < father->header().qc().view()){
        lock_qc_[i]  = father->header().qc();
      }
    }
    if(fafather != nullptr){
      //LOG(ERROR)<<"get fafather view:"<<fafather->header().view()<<" slot:"<<fafather->header().slot();
      fafafather = GetProposal(fafather->header().prehash());
      if(fafafather != nullptr){
        //commit;
        //LOG(ERROR)<<"commit fafafather view:"<<fafafather->header().view()<<" slot:"<<fafafather->header().slot();
        if(next_commit_[view] <= fafafather->header().slot()){
          for(int i = next_commit_[view]; i <=fafafather->header().slot(); ++i){
            std::unique_ptr<Proposal> commit_ready_proposal = FetchProposal(fafafather->header().view(), i);
            if(commit_ready_proposal != nullptr) {
              commit_ready_proposal_list.push_back(std::move(commit_ready_proposal));
            }
          }
          next_commit_[view] = fafafather->header().slot()+1;
        }
      }
    }
  }
  assert(local_block_hash_.find(std::make_pair(view, slot)) == local_block_hash_.end());
  //LOG(ERROR)<<"add proposal view:"<<view<<" slot:"<<slot;
  local_block_[hash] = std::move(proposal);
  local_block_hash_[std::make_pair(view, slot)] = hash;
  return commit_ready_proposal_list;
}

const Proposal * ProposalManager::GetProposal(const std::string& hash){
  auto it = local_block_.find(hash);
  if(it == local_block_.end()){
    return nullptr;
  }
  return it->second.get();
}

std::unique_ptr<Proposal> ProposalManager::FetchProposal(int view, int slot){
 //LOG(ERROR)<<"fetch view:"<<view<<" slot:"<<slot;
  auto it = local_block_hash_.find(std::make_pair(view, slot));
  assert( it != local_block_hash_.end());

  std::string hash = it->second;

  auto hit = local_block_.find(hash);
  assert(hit != local_block_.end());
  std::unique_ptr<Proposal> ret = std::move(hit->second);
  local_block_.erase(hit);

  local_block_hash_.erase(it);
  return ret;
}

}  // namespace ooohs
}  // namespace resdb

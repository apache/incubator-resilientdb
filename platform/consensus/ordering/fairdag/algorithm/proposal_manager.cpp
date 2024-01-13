#include "platform/consensus/ordering/fairdag/algorithm/proposal_manager.h"

#include <glog/logging.h>
#include "common/utils/utils.h"
#include "common/crypto/signature_verifier.h"

namespace resdb {
namespace fairdag {

ProposalManager::ProposalManager(int32_t id, int limit_count)
    : id_(id), limit_count_(limit_count) {
    round_ = 0;
}

std::unique_ptr<Proposal> ProposalManager::GenerateProposal(
    const std::vector<std::unique_ptr<Transaction>>& txns) {
  std::unique_ptr<Proposal> proposal = std::make_unique<Proposal>();
  std::string data;
  {
    std::unique_lock<std::mutex> lk(txn_mutex_);
    for(const auto& txn: txns){
      *proposal->add_transactions() = *txn;
      std::string tmp;
      txn->SerializeToString(&tmp);
      data += tmp;
    }
    proposal->mutable_header()->set_proposer_id(id_);
    proposal->mutable_header()->set_create_time(GetCurrentTime());
    proposal->mutable_header()->set_round(round_);
    proposal->set_sender(id_);

    GetMetaData(proposal.get());  
  }

  std::string header_data;
  proposal->header().SerializeToString(&header_data);
  data +=  header_data;

  std::string hash = SignatureVerifier::CalculateHash(data);
  proposal->set_hash(hash);

  round_++;
  return proposal;
}

int ProposalManager::CurrentRound(){
  return round_;
}

void ProposalManager::AddLocalBlock(std::unique_ptr<Proposal> proposal){
  std::unique_lock<std::mutex> lk(local_mutex_);
  local_block_[proposal->hash()] = std::move(proposal);
}

const Proposal * ProposalManager::GetLocalBlock(const std::string& hash) {
    std::unique_lock<std::mutex> lk(local_mutex_);
  auto bit = local_block_.find(hash);
  if(bit == local_block_.end()){
    LOG(ERROR)<<" block not exist:"<<hash.size();
    return nullptr;
  }
  return bit->second.get();
}

std::unique_ptr<Proposal> ProposalManager::FetchLocalBlock(const std::string& hash) {
    std::unique_lock<std::mutex> lk(local_mutex_);
  auto bit = local_block_.find(hash);
  if(bit == local_block_.end()){
    LOG(ERROR)<<" block not exist:"<<hash.size();
    return nullptr;
  }
  auto tmp = std::move(bit->second);
  local_block_.erase(bit);
  return tmp;
}

void ProposalManager::AddBlock(std::unique_ptr<Proposal> proposal){
    std::unique_lock<std::mutex> lk(txn_mutex_);
    LOG(ERROR)<<"add block hash :"<<proposal->hash().size()<<" round:"<<proposal->header().round()<<" proposer:"<<proposal->header().proposer_id();
    block_[proposal->hash()] = std::move(proposal);
}

int ProposalManager::GetReferenceNum(const Proposal&req) {
    int round = req.header().round();
    int proposer = req.header().proposer_id();
    std::unique_lock<std::mutex> lk(txn_mutex_);
    return reference_[std::make_pair(round, proposer)];
}

std::unique_ptr<Proposal> ProposalManager::FetchRequest(int round, int sender) {
  std::unique_lock<std::mutex> lk(txn_mutex_);
  auto it = cert_list_[round].find(sender);
  if(it == cert_list_[round].end()){
    //LOG(ERROR)<<" cert from sender:"<<sender<<" round:"<<round<<" not exist";
    return nullptr;
  }
  std::string hash = it->second->hash();
  auto bit = block_.find(hash);
  if(bit == block_.end()){
    LOG(ERROR)<<" block from sender:"<<sender<<" round:"<<round<<" not exist";
    assert(1==0);
    return nullptr;
  }
  auto tmp = std::move(bit->second);
  cert_list_[round].erase(sender);
  //LOG(ERROR)<<" featch sender done, round:"<<round<<" sender:"<<sender;
  if(sender == id_){
    FetchLocalBlock(hash);
  }
  return tmp;
}

const Proposal * ProposalManager::GetRequest(int round, int sender) {
    std::unique_lock<std::mutex> lk(txn_mutex_);
  auto it = cert_list_[round].find(sender);
  if(it == cert_list_[round].end()){
    LOG(ERROR)<<" cert from sender:"<<sender<<" round:"<<round<<" not exist";
    return nullptr;
  }
  std::string hash = it->second->hash();
  auto bit = block_.find(hash);
  if(bit == block_.end()){
    LOG(ERROR)<<" block from sender:"<<sender<<" round:"<<round<<" not exist";
    return nullptr;
  }
  return bit->second.get();
}

void ProposalManager::AddCert(std::unique_ptr<Certificate> cert) {
  int proposer = cert->proposer();
  int round = cert->round();
  std::string hash = cert->hash();

  //LOG(ERROR)<<"add cert sender:"<<proposer<<" round:"<<round;
  std::unique_lock<std::mutex> lk(txn_mutex_);

  //LOG(ERROR)<<"strong cert size:"<<cert->strong_cert().cert_size();
  for(auto& link : cert->strong_cert().cert()){
    int link_round = link.round();
    int link_proposer = link.proposer();
    //LOG(ERROR)<<"refer round:"<<link_round<<" proposer:"<<link_proposer;
    reference_[std::make_pair(link_round, link_proposer)]++; 
  }

  cert->mutable_strong_cert()->Clear();
  cert->mutable_metadata()->Clear();
  
  auto tmp = std::make_unique<Certificate>(*cert);
  cert_list_[round][proposer] = std::move(cert);
  latest_cert_from_sender_[proposer] = std::move(tmp);
}

void ProposalManager::GetMetaData(Proposal * proposal) {
  if(round_ == 0){
    return;
  }
  assert(cert_list_[round_-1].size() >= limit_count_) ;
  assert (cert_list_[round_ - 1].find(id_) != cert_list_[round_ - 1].end()) ;

  std::set<int> meta_ids;
  for (const auto& preview_cert : cert_list_[round_ - 1]) {
    *proposal->mutable_header()->mutable_strong_cert()->add_cert() = *preview_cert.second;
    meta_ids.insert(preview_cert.first);
     //LOG(ERROR)<<"add strong round:"<<preview_cert.second->round()
     //<<" id:"<<preview_cert.second->proposer()<<" first:"<<preview_cert.first<<" limit:"<<limit_count_;
    if (meta_ids.size() == limit_count_){
      break;
    }
  }

  for (const auto& meta : latest_cert_from_sender_) {
    //LOG(ERROR)<<"check:"<<meta.first<<" proposal round:"<<proposal->header().round();
    if (meta_ids.find(meta.first) != meta_ids.end()) {
      continue;
    }
    if (meta.second->round() > round_) {
      for (int j = round_; j >= 0; --j) {
        if (cert_list_[j].find(meta.first) != cert_list_[j].end()) {
          LOG(ERROR)<<" add weak cert from his:"<<j<<" proposer:"<<meta.first;
          *proposal->mutable_header()->mutable_weak_cert()->add_cert() = *cert_list_[j][meta.first];
          break;
        }
      }
    } else {
      assert(meta.second->round() <= round_);
      //LOG(ERROR)<<"add weak cert:"<<meta.second->round()<<" proposer:"<<meta.second->proposer();
      *proposal->mutable_header()->mutable_weak_cert()->add_cert() = *meta.second;
    }
  }
}

bool ProposalManager::Ready(){
    std::unique_lock<std::mutex> lk(txn_mutex_);
  if(round_ == 0){
    return true;
  }
  if(cert_list_[round_-1].size() < limit_count_) {
    return false;
  }
  if(cert_list_[round_ - 1].find(id_) == cert_list_[round_ - 1].end()) {
    return false;
  }
  return true;
}

}  // namespace tusk
}  // namespace resdb

#include "platform/consensus/ordering/fides/algorithm/proposal_manager.h"

#include <glog/logging.h>

#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"

namespace resdb {
namespace fides {

ProposalManager::ProposalManager(int32_t id, int limit_count, oe_enclave_t* enclave)
    : id_(id), limit_count_(limit_count), enclave_(enclave)  {
  round_ = 0;
}

bool ProposalManager::VerifyHash(const Proposal &proposal){
  std::string data;
  for (const auto& txn : proposal.transactions()) {
    std::string tmp;
    txn.SerializeToString(&tmp);
    data += tmp;
  }

  std::string header_data;
  proposal.header().SerializeToString(&header_data);
  data += header_data;

  std::string hash = SignatureVerifier::CalculateHash(data);
  return hash == proposal.hash();
}

// TODO: 4.Add counter value into block
std::unique_ptr<Proposal> ProposalManager::GenerateProposal(
    const std::vector<std::unique_ptr<Transaction>>& txns) {
  std::unique_ptr<Proposal> proposal = std::make_unique<Proposal>();
  std::string data;
  {
    std::unique_lock<std::mutex> lk(txn_mutex_);
    for (const auto& txn : txns) {
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
  data += header_data;

  std::string hash = SignatureVerifier::CalculateHash(data);
  proposal->set_hash(hash);
  
  // Send counter value and attestation to enclave
  size_t previous_cert_size = proposal->header().strong_cert().cert().size();
  unsigned char** attestation_array = new unsigned char*[previous_cert_size];
  size_t* attestation_size_array = new size_t[previous_cert_size];
  uint32_t* counter_value_array = new uint32_t[previous_cert_size];
  int i = 0;

  std::string serialized_data;
  for(auto& link : proposal->header().strong_cert().cert()) {
    // LOG(ERROR)<<"attestation is: "<<link.counter().attestation();
    counter_value_array[i] = link.counter().value();
    serialized_data = link.counter().attestation();
    size_t size = serialized_data.size();
    attestation_array[i] = new unsigned char[size];
    std::memcpy(attestation_array[i], serialized_data.data(), size);
    attestation_size_array[i] = size;
    i++;
  }

// /*
  int ret;
  uint32_t counter_value, default_index = 0;
  result = get_counter(enclave_, &ret, &default_index, previous_cert_size, limit_count_, counter_value_array, 
                      attestation_size_array, attestation_array, &counter_value);
  if (result != OE_OK) {
    ret = 1;
  }
  if (ret != 0) {
    std::cerr << "Host: get_counter failed with " << ret << std::endl;
  }
  
  // std::unique_ptr<CounterInfo> counter = std::make_unique<CounterInfo>();
  CounterInfo* counter = proposal->mutable_header()->mutable_counter();
  counter->set_value(counter_value);
  counter->set_attestation("Fake Attestation");
  // proposal->set_allocated_counter(counter.get());

  // LOG(ERROR)<<"round: "<<round_<<"counter_value: "<<counter_value;
// */

  round_++;
  return proposal;
}

int ProposalManager::CurrentRound() { return round_; }

void ProposalManager::AddLocalBlock(std::unique_ptr<Proposal> proposal) {
  std::unique_lock<std::mutex> lk(local_mutex_);
  local_block_[proposal->hash()] = std::move(proposal);
}

const Proposal* ProposalManager::GetLocalBlock(const std::string& hash) {
  std::unique_lock<std::mutex> lk(local_mutex_);
  auto bit = local_block_.find(hash);
  if (bit == local_block_.end()) {
    LOG(ERROR) << " block not exist:" << hash.size();
    return nullptr;
  }
  return bit->second.get();
}

std::unique_ptr<Proposal> ProposalManager::FetchLocalBlock(
    const std::string& hash) {
  std::unique_lock<std::mutex> lk(local_mutex_);
  auto bit = local_block_.find(hash);
  if (bit == local_block_.end()) {
    //LOG(ERROR) << " block not exist:" << hash.size();
    return nullptr;
  }
  auto tmp = std::move(bit->second);
  local_block_.erase(bit);
  return tmp;
}

void ProposalManager::AddBlock(std::unique_ptr<Proposal> proposal) {
  std::unique_lock<std::mutex> lk(txn_mutex_);
  //LOG(ERROR) << "add block hash :" << proposal->hash().size()
 //            << " round:" << proposal->header().round()
 //            << " proposer:" << proposal->header().proposer_id();
  block_[proposal->hash()] = std::move(proposal);
}

bool ProposalManager::CheckBlock(const std::string& hash){
    std::unique_lock<std::mutex> lk(txn_mutex_);
    //LOG(ERROR)<<"add block hash :"<<proposal->hash().size()<<" round:"<<proposal->header().round()<<" proposer:"<<proposal->header().proposer_id();
    return block_.find(hash) != block_.end();
}

int ProposalManager::GetReferenceNum(const Proposal& req) {
  int round = req.header().round();
  int round_1 = round + 1;
  int proposer = req.header().proposer_id();
  std::unique_lock<std::mutex> lk(txn_mutex_);
  // return reference_[std::make_pair(round, proposer)];
  std::unordered_set<int> reference_proposer;
  for (auto& proposer_1 : reference_[std::make_pair(round, proposer)]) {
    for (auto& proposer_2 : reference_[std::make_pair(round_1, proposer_1)]) {
      if (reference_proposer.count(proposer_2) == 0)
        reference_proposer.insert(proposer_2);
    }
  }
  return reference_proposer.size();
}

std::unique_ptr<Proposal> ProposalManager::FetchRequest(int round, int sender) {
  std::unique_lock<std::mutex> lk(txn_mutex_);
  auto it = cert_list_[round].find(sender);
  if (it == cert_list_[round].end()) {
    // LOG(ERROR)<<" cert from sender:"<<sender<<" round:"<<round<<" not exist";
    return nullptr;
  }
  std::string hash = it->second->hash();
  auto bit = block_.find(hash);
  if (bit == block_.end()) {
    LOG(ERROR) << " block from sender:" << sender << " round:" << round
               << " not exist";
    assert(1 == 0);
    return nullptr;
  }
  auto tmp = std::move(bit->second);
  //cert_list_[round].erase(sender);
  //LOG(ERROR)<<" featch sender done, round:"<<round<<" sender:"<<sender;
  if (sender == id_) {
    FetchLocalBlock(hash);
  }
  return tmp;
}

const Proposal* ProposalManager::GetRequest(int round, int sender) {
  std::unique_lock<std::mutex> lk(txn_mutex_);
  auto it = cert_list_[round].find(sender);
  if (it == cert_list_[round].end()) {
    //LOG(ERROR) << " cert from sender:" << sender << " round:" << round
    //           << " not exist";
    return nullptr;
  }
  std::string hash = it->second->hash();
  auto bit = block_.find(hash);
  if (bit == block_.end()) {
    LOG(ERROR) << " block from sender:" << sender << " round:" << round
               << " not exist";
    return nullptr;
  }
  return bit->second.get();
}


// TODO: 6.Remove Cert Check counter value instead.
void ProposalManager::AddCert(std::unique_ptr<Certificate> cert) {
  int proposer = cert->proposer();
  int round = cert->round();
  std::string hash = cert->hash();

  // LOG(ERROR)<<"add cert sender:"<<proposer<<" round:"<<round;
  std::unique_lock<std::mutex> lk(txn_mutex_);

  // LOG(ERROR)<<"strong cert size:"<<cert->strong_cert().cert_size();
  std::unordered_set<int> refered_proposer;
  for (auto& link : cert->strong_cert().cert()) {
    int link_round = link.round();
    int link_proposer = link.proposer();  
    reference_[std::make_pair(link_round, link_proposer)].push_back(cert->proposer());
  }

  cert->mutable_strong_cert()->Clear();
  cert->mutable_metadata()->Clear();

  auto tmp = std::make_unique<Certificate>(*cert);
  cert_list_[round][proposer] = std::move(cert);
  latest_cert_from_sender_[proposer] = std::move(tmp);
}

bool ProposalManager::CheckCert(int round, int sender) {
  std::unique_lock<std::mutex> lk(txn_mutex_);
  return cert_list_[round].find(sender) != cert_list_[round].end();
}

void ProposalManager::GetMetaData(Proposal* proposal) {
  if (round_ == 0) {
    return;
  }
  assert(cert_list_[round_ - 1].size() >= limit_count_);
  assert(cert_list_[round_ - 1].find(id_) != cert_list_[round_ - 1].end());

  //LOG(ERROR)<<"get metadata:"<<" proposal round:"<<proposal->header().round();
  std::set<int> meta_ids;
  for (const auto& preview_cert : cert_list_[round_ - 1]) {
    *proposal->mutable_header()->mutable_strong_cert()->add_cert() =
        *preview_cert.second;
    meta_ids.insert(preview_cert.first);
     //LOG(ERROR)<<"add strong round:"<<preview_cert.second->round()
    //<<" id:"<<preview_cert.second->proposer()<<" first:"<<preview_cert.first<<" limit:"<<limit_count_<<" proposal round:"<<proposal->header().round();
    if (meta_ids.size() == limit_count_) {
      break;
    }
  }
  //LOG(ERROR)<<"strong link:"<<proposal->header().strong_cert().cert_size()<<" round:"<<proposal->header().round();

  for (const auto& meta : latest_cert_from_sender_) {
    // LOG(ERROR)<<"check:"<<meta.first<<" proposal
    // round:"<<proposal->header().round();
    if (meta_ids.find(meta.first) != meta_ids.end()) {
      continue;
    }
    if (meta.second->round() >= round_) {
      for (int j = round_-1; j >= 0; --j) {
        if (cert_list_[j].find(meta.first) != cert_list_[j].end()) {
          //LOG(ERROR) << " add weak cert from his:" << j
          //           << " proposer:" << meta.first<< " cert round:"<<cert_list_[j][meta.first]->round()<< " proposal round:"<<proposal->header().round();
          *proposal->mutable_header()->mutable_weak_cert()->add_cert() =
              *cert_list_[j][meta.first];
          break;
        }
      }
    } else {
      assert(meta.second->round() <= round_);
      //LOG(ERROR)<<"add weak cert:"<<meta.second->round()<<" proposer:"<<meta.second->proposer()<<" proposal round:"<<proposal->header().round();
      *proposal->mutable_header()->mutable_weak_cert()->add_cert() =
          *meta.second;
    }
  }
    //LOG(ERROR)<<"weak link:"<<proposal->header().weak_cert().cert_size()<<" round:"<<proposal->header().round();
}

bool ProposalManager::Ready() {
  std::unique_lock<std::mutex> lk(txn_mutex_);
  if (round_ == 0) {
    return true;
  }
  if (cert_list_[round_ - 1].size() < limit_count_) {
    return false;
  }
  if (cert_list_[round_ - 1].find(id_) == cert_list_[round_ - 1].end()) {
    return false;
  }
  return true;
}

std::vector<CounterInfo> ProposalManager::GetCounterFromRound(int round) {
  std::vector<CounterInfo> counters;
  for (const auto& preview_cert : cert_list_[round]) {
    counters.push_back(preview_cert.second->counter());
    if (counters.size() == limit_count_) {
      break;
    }
  }
  return counters;
}

}  // namespace fides
}  // namespace resdb

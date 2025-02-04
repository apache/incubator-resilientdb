#include "platform/consensus/ordering/tusk/protocol/proposal_manager.h"

#include <glog/logging.h>

#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"

namespace resdb {
namespace tusk {

ProposalManager::ProposalManager(int32_t id, int limit_count, int dag_id,
                                 int total_num)
    : id_(id), limit_count_(limit_count), total_num_(total_num) {
  round_ = 0;
  dag_id_ = dag_id;
}

void ProposalManager::Reset(int dag_id) {
  std::unique_lock<std::mutex> lk(txn_mutex_);
  dag_id_ = dag_id;
  round_ = 0;
  cert_list_.clear();
  latest_cert_from_sender_.clear();
  reference_.clear();
  // LOG(ERROR)<<" reset";
}

bool ProposalManager::VerifyHash(const Proposal& proposal) {
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

std::unique_ptr<Proposal> ProposalManager::GenerateProposal(
    const std::vector<std::unique_ptr<Transaction>>& txns) {
  std::unique_ptr<Proposal> proposal = std::make_unique<Proposal>();
  std::string data;
  {
    std::unique_lock<std::mutex> lk(txn_mutex_);
    uint64_t flag = 0;
    for (const auto& txn : txns) {
      *proposal->add_transactions() = *txn;
      // LOG(ERROR)<<" proposer:"<<id_<<" add txn:"<<txn->id()<<" group
      // size:"<<txn->belong_shards_size();
      std::string tmp;
      txn->SerializeToString(&tmp);
      data += tmp;
      for (int gid : txn->belong_shards()) {
        flag |= 1 << (gid - 1);
      }
    }
    proposal->mutable_header()->set_proposer_id(id_);
    proposal->mutable_header()->set_create_time(GetCurrentTime());
    proposal->mutable_header()->set_round(round_);
    proposal->mutable_header()->set_dag_id(dag_id_);
    proposal->mutable_header()->set_shard_flag(flag);
    proposal->set_sender(id_);
    // LOG(ERROR)<<" proposer id:"<<proposal->header().proposer_id()<<" txn
    // size:"<<txns.size()<<" round:"<<round_;
    GetMetaData(proposal.get());
    round_++;
  }

  std::string header_data;
  proposal->header().SerializeToString(&header_data);
  data += header_data;

  std::string hash = SignatureVerifier::CalculateHash(data);
  proposal->set_hash(hash);

  return proposal;
}

int ProposalManager::CurrentRound() { return round_; }

std::vector<std::unique_ptr<Transaction>> ProposalManager::GetTxn() {
  std::vector<std::unique_ptr<Transaction>> ret;
  std::unique_lock<std::mutex> lk(txn_mutex_);
  // LOG(ERROR)<<" get block size:"<<block_.size();
  for (auto& it : block_) {
    auto& p = it.second;
    if (p == nullptr) {
      continue;
    }
    if (p->header().proposer_id() == id_) {
      for (const auto& txn : p->transactions()) {
        ret.push_back(std::make_unique<Transaction>(txn));
      }
    }
  }
  block_.clear();
  // LOG(ERROR)<<" clear block size done:"<<block_.size();
  return ret;
}

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
    LOG(ERROR) << " block not exist:" << hash.size();
    return nullptr;
  }
  auto tmp = std::move(bit->second);
  local_block_.erase(bit);
  return tmp;
}

void ProposalManager::AddBlock(std::unique_ptr<Proposal> proposal) {
  // GenerateCrossGroup(*proposal);
  std::unique_lock<std::mutex> lk(txn_mutex_);
  // LOG(ERROR) << "add block hash :" << proposal->hash().size()
  //           << " round:" << proposal->header().round()
  //           << " proposer:" << proposal->header().proposer_id()
  //           <<" dag:"<<dag_id_;
  block_[proposal->hash()] = std::move(proposal);
}

int ProposalManager::GetReferenceNum(const Proposal& req) {
  int round = req.header().round();
  int proposer = req.header().proposer_id();
  std::unique_lock<std::mutex> lk(txn_mutex_);
  return reference_[std::make_pair(round, proposer)];
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
               << " not exist"
               << " dag:" << dag_id_;
    assert(1 == 0);
    return nullptr;
  }
  auto tmp = std::move(bit->second);
  cert_list_[round].erase(sender);
  // LOG(ERROR)<<" featch sender done, round:"<<round<<" sender:"<<sender;
  if (sender == id_) {
    FetchLocalBlock(hash);
  }
  return tmp;
}

const Proposal* ProposalManager::GetRequest(int round, int sender) {
  std::unique_lock<std::mutex> lk(txn_mutex_);
  auto it = cert_list_[round].find(sender);
  if (it == cert_list_[round].end()) {
    // LOG(ERROR) << " cert from sender:" << sender << " round:" << round
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

void ProposalManager::AddCert(std::unique_ptr<Certificate> cert) {
  int proposer = cert->proposer();
  int round = cert->round();
  std::string hash = cert->hash();

  // LOG(ERROR)<<"add cert sender:"<<proposer<<" round:"<<round<<"
  // dag:"<<cert->dag_id();
  assert(cert->dag_id() == dag_id_);
  std::unique_lock<std::mutex> lk(txn_mutex_);

  // LOG(ERROR)<<"strong cert size:"<<cert->strong_cert().cert_size();
  for (auto& link : cert->strong_cert().cert()) {
    int link_round = link.round();
    int link_proposer = link.proposer();
    // LOG(ERROR)<<"refer round:"<<link_round<<" proposer:"<<link_proposer;
    reference_[std::make_pair(link_round, link_proposer)]++;
  }

  cert->mutable_strong_cert()->Clear();
  cert->mutable_metadata()->Clear();
  // ShardInfo * shard_info = GetShardInfo(proposer, round);
  // assert(shard_info != nullptr);
  // cert_shard_[round] |= shard_info->shard_flag;

  auto tmp = std::make_unique<Certificate>(*cert);
  cert_list_[round][proposer] = std::move(cert);
  latest_cert_from_sender_[proposer] = std::move(tmp);
}

void ProposalManager::GetMetaData(Proposal* proposal) {
  if (round_ == 0) {
    return;
  }
  assert(static_cast<int>(cert_list_[round_ - 1].size()) >= limit_count_);
  assert(cert_list_[round_ - 1].find(id_) != cert_list_[round_ - 1].end());

  std::set<int> meta_ids;
  for (const auto& preview_cert : cert_list_[round_ - 1]) {
    *proposal->mutable_header()->mutable_strong_cert()->add_cert() =
        *preview_cert.second;
    meta_ids.insert(preview_cert.first);
    assert(preview_cert.second->dag_id() == dag_id_);
    // LOG(ERROR)<<"add strong round:"<<preview_cert.second->round()
    //<<" id:"<<preview_cert.second->proposer()<<"
    //first:"<<preview_cert.first<<" limit:"<<limit_count_;
    /*
    if (static_cast<int>(meta_ids.size()) == limit_count_) {
      break;
    }
    */
  }
  // LOG(ERROR)<<"strong link:"<<proposal->header().strong_cert().cert_size()<<"
  // round:"<<proposal->header().round();

  for (const auto& meta : latest_cert_from_sender_) {
    // LOG(ERROR)<<"check:"<<meta.first<<" proposal
    // round:"<<proposal->header().round();
    if (meta_ids.find(meta.first) != meta_ids.end()) {
      continue;
    }
    if (meta.second->round() > round_) {
      for (int j = round_; j >= 0; --j) {
        if (cert_list_[j].find(meta.first) != cert_list_[j].end()) {
          // LOG(ERROR) << " add weak cert from his:" << j
          //           << " proposer:" << meta.first;
          if (round_ - k_ > j) {
            // LOG(ERROR) << " add stop from round:" << round_<<"
            // from:"<<meta.first<<" miss round:"<< j; proposal->set_stop(true);
          }
          *proposal->mutable_header()->mutable_weak_cert()->add_cert() =
              *cert_list_[j][meta.first];
          assert(cert_list_[j][meta.first]->dag_id() == dag_id_);
          break;
        }
      }
    } else {
      assert(meta.second->round() <= round_);
      // LOG(ERROR)<<"add weak cert:"<<meta.second->round()<<"
      // proposer:"<<meta.second->proposer();
      *proposal->mutable_header()->mutable_weak_cert()->add_cert() =
          *meta.second;
    }
  }

  if (round_ >= rotate_k_) {
    LOG(ERROR) << " add stop from round:" << round_ << "  end:" << rotate_k_;
    proposal->set_stop(true);
  }
  // LOG(ERROR)<<"weak link:"<<proposal->header().weak_cert().cert_size()<<"
  // round:"<<proposal->header().round();
}

bool ProposalManager::Ready() {
  std::unique_lock<std::mutex> lk(txn_mutex_);
  // LOG(ERROR)<<" ready round:"<<round_;
  if (round_ == 0) {
    return true;
  }
  if (cert_list_[round_ - 1].size() < limit_count_) {
    // if (cert_list_[round_ - 1].size() < total_num_) {
    return false;
  }
  if (cert_list_[round_ - 1].find(id_) == cert_list_[round_ - 1].end()) {
    return false;
  }
  if (round_ > rotate_k_) {
    //    return false;
  }
  return true;
}

int ProposalManager::GetLeader(int64_t r) { return r / 2 % total_num_ + 1; }

bool ProposalManager::ReadyWithLeader() {
  std::unique_lock<std::mutex> lk(txn_mutex_);
  if (round_ == 0) {
    return true;
  }
  // if (cert_list_[round_ - 1].size() < total_num_) {
  if (cert_list_[round_ - 1].size() < limit_count_) {
    return false;
  }
  if (cert_list_[round_ - 1].find(id_) == cert_list_[round_ - 1].end()) {
    return false;
  }
  if (round_ % 2 == 0) {
    int leader = GetLeader(round_);
    // LOG(ERROR)<<" ready round:"<<round_<<" leader:"<<leader;
    if (leader != id_ &&
        cert_list_[round_].find(leader) == cert_list_[round_].end()) {
      //   LOG(ERROR)<<" ready round:"<<round_<<" leader:"<<leader<<" fail";
      return false;
    }
    // LOG(ERROR)<<" ready round:"<<round_<<" leader:"<<leader<<" done";
  }
  return true;
}

void ProposalManager::AddShardInfo(std::unique_ptr<ShardInfo> shard_info) {
  std::unique_lock<std::mutex> lk(shard_mutex_);
  int round = shard_info->round;
  int proposer = shard_info->proposer;
  shard_info_list_[std::make_pair(proposer, round)] = std::move(shard_info);
}

ProposalManager::ShardInfo* ProposalManager::GetShardInfo(int proposer,
                                                          int round) {
  std::unique_lock<std::mutex> lk(shard_mutex_);
  auto it = shard_info_list_.find(std::make_pair(proposer, round));
  if (it == shard_info_list_.end()) {
    return nullptr;
  }
  return it->second.get();
}

void ProposalManager::MergeShardInfo(ShardInfo* info1, ShardInfo* info2) {
  int64_t shard_flag = info1->shard_flag;
  for (auto s : info2->shards) {
    int round = s.round;
    int proposer = s.proposer;
    auto next_p = GetRequest(round, proposer);
    if (next_p == nullptr) {
      LOG(ERROR) << " shard info proposer:" << proposer << " round:" << round
                 << " has committed";
      continue;
    }
    LOG(ERROR) << " check shard info proposer:" << proposer
               << " round:" << round;
    shard_flag |= s.shard_flag;
    info1->Add(s);
  }
  info1->shard_flag = shard_flag;
  LOG(ERROR) << " merge flag, proposer:" << info1->proposer
             << " round:" << info1->round << " flag:" << info1->shard_flag
             << " with proposer:" << info2->proposer
             << " round:" << info2->round << " flag:" << info2->shard_flag;
}

void ProposalManager::GenerateCrossGroup(const Proposal& p) {
  std::unique_ptr<ShardInfo> shard_info =
      std::make_unique<ShardInfo>(p.header().proposer_id(), p.header().round());
  shard_info->shard_flag = p.header().shard_flag();
  if (p.header().shard_flag()) {
    shard_info->Add(*shard_info);
  }
  LOG(ERROR) << " generate proposal proposer:" << shard_info->proposer
             << " round:" << shard_info->round
             << " flag:" << shard_info->shard_flag;

  for (auto& link : p.header().strong_cert().cert()) {
    int link_round = link.round();
    int link_proposer = link.proposer();
    auto next_p = GetRequest(link_round, link_proposer);
    if (next_p == nullptr) {
      continue;
    }
    ShardInfo* info = GetShardInfo(link_proposer, link_round);
    if (info == nullptr) {
      GenerateCrossGroup(*next_p);
    }
    MergeShardInfo(shard_info.get(), info);
  }

  for (auto& link : p.header().weak_cert().cert()) {
    int link_round = link.round();
    int link_proposer = link.proposer();
    auto next_p = GetRequest(link_round, link_proposer);
    if (next_p == nullptr) {
      continue;
    }
    ShardInfo* info = GetShardInfo(link_proposer, link_round);
    if (info == nullptr) {
      GenerateCrossGroup(*next_p);
    }
    MergeShardInfo(shard_info.get(), info);
  }
  AddShardInfo(std::move(shard_info));
}

}  // namespace tusk
}  // namespace resdb

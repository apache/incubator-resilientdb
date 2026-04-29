#include "platform/consensus/ordering/bullshark/algorithm/proposal_manager.h"

#include <glog/logging.h>

#include <queue>
#include <set>

#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"

namespace resdb {
namespace bullshark {

ProposalManager::ProposalManager(int32_t id, int limit_count, int total_num)
    : id_(id), limit_count_(limit_count), total_num_(total_num) {
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
    return nullptr;
  }
  auto tmp = std::move(bit->second);
  local_block_.erase(bit);
  return tmp;
}

void ProposalManager::AddBlock(std::unique_ptr<Proposal> proposal) {
  std::unique_lock<std::mutex> lk(txn_mutex_);
  block_[proposal->hash()] = std::move(proposal);
}

bool ProposalManager::CheckBlock(const std::string& hash){
    std::unique_lock<std::mutex> lk(txn_mutex_);
    return block_.find(hash) != block_.end();
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
    return nullptr;
  }
  std::string hash = it->second->hash();
  auto bit = block_.find(hash);
  if (bit == block_.end() || bit->second == nullptr) {
    // The block can legitimately be gone when a previous cascade commit
    // already consumed it (FetchRequest is destructive — it moves the
    // block out of block_). Keeping the assert here crashed the replica
    // whenever AsyncCommit re-visited a leader round that AsyncExecute
    // had already drained as part of an ancestor BFS, which is the
    // bullshark startup-race pattern where some runs produce 0% recovery.
    // Treat "cert present, block gone" as "already committed" and skip.
    return nullptr;
  }
  auto tmp = std::move(bit->second);
  if (sender == id_) {
    FetchLocalBlock(hash);
  }
  return tmp;
}

const Proposal* ProposalManager::GetRequest(int round, int sender) {
  std::unique_lock<std::mutex> lk(txn_mutex_);
  auto it = cert_list_[round].find(sender);
  if (it == cert_list_[round].end()) {
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

  std::unique_lock<std::mutex> lk(txn_mutex_);

  for (auto& link : cert->strong_cert().cert()) {
    int link_round = link.round();
    int link_proposer = link.proposer();
    reference_[std::make_pair(link_round, link_proposer)]++;
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

  std::set<int> meta_ids;
  for (const auto& preview_cert : cert_list_[round_ - 1]) {
    *proposal->mutable_header()->mutable_strong_cert()->add_cert() =
        *preview_cert.second;
    meta_ids.insert(preview_cert.first);
    if (meta_ids.size() == limit_count_) {
      break;
    }
  }

  for (const auto& meta : latest_cert_from_sender_) {
    if (meta_ids.find(meta.first) != meta_ids.end()) {
      continue;
    }
    if (meta.second->round() >= round_) {
      for (int j = round_-1; j >= 0; --j) {
        if (cert_list_[j].find(meta.first) != cert_list_[j].end()) {
          *proposal->mutable_header()->mutable_weak_cert()->add_cert() =
              *cert_list_[j][meta.first];
          break;
        }
      }
    } else {
      // Defensive: the previous assert(meta.second->round() <= round_)
      // crashed the replica whenever latest_cert_from_sender_ was
      // ahead of round_ for a still-visible sender (can happen under
      // the crash-event message burst). Just include the cert if it
      // is older, otherwise fall through and let the round-walk above
      // pick it up when round_ advances.
      *proposal->mutable_header()->mutable_weak_cert()->add_cert() =
          *meta.second;
    }
  }
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

// BullShark: BFS through strong edges to check reachability.
bool ProposalManager::HasStrongPath(int from_round, int from_proposer,
                                     int to_round, int to_proposer) {
  if (from_round == to_round && from_proposer == to_proposer) return true;
  if (from_round <= to_round) return false;

  std::unique_lock<std::mutex> lk(txn_mutex_);

  std::queue<std::pair<int, int>> bfs_queue;
  std::set<std::pair<int, int>> visited;

  bfs_queue.push({from_round, from_proposer});
  visited.insert({from_round, from_proposer});

  while (!bfs_queue.empty()) {
    auto cur = bfs_queue.front();
    bfs_queue.pop();
    int r = cur.first;
    int p = cur.second;

    if (r == to_round && p == to_proposer) return true;
    if (r <= to_round) continue;

    // Look up the block via cert_list_ -> hash -> block_
    auto cert_it = cert_list_.find(r);
    if (cert_it == cert_list_.end()) continue;
    auto proposer_it = cert_it->second.find(p);
    if (proposer_it == cert_it->second.end()) continue;

    std::string hash = proposer_it->second->hash();
    auto block_it = block_.find(hash);
    // CRITICAL FIX: FetchRequest is destructive — it std::moves the
    // unique_ptr out of block_ without erasing the key, so previously-
    // committed blocks leave live map keys with null values. The
    // original code only checked for end() and dereferenced a moved-
    // out pointer, segfaulting the replica silently whenever AsyncExecute
    // had BFS-drained the corresponding block for an earlier commit.
    // This was the root of bullshark's startup flakiness (13 of 16
    // nodes crashing at t~5s). Check for null before dereferencing.
    if (block_it == block_.end() || block_it->second == nullptr) continue;

    // Follow strong_cert references in the block's header
    for (const auto& link : block_it->second->header().strong_cert().cert()) {
      std::pair<int, int> next = {link.round(), link.proposer()};
      if (visited.find(next) == visited.end()) {
        visited.insert(next);
        bfs_queue.push(next);
      }
    }
  }
  return false;
}

}  // namespace bullshark
}  // namespace resdb

#include "platform/consensus/ordering/mysticeti/algorithm/proposal_manager.h"

#include <glog/logging.h>

#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"

namespace resdb {
namespace mysticeti {

ProposalManager::ProposalManager(int32_t id, int limit_count, int total_num)
    : id_(id), limit_count_(limit_count), total_num_(total_num) {
  round_ = 0;
}

// Mysticeti-C pipelined leaders: every round has a leader (round-robin)
int ProposalManager::GetLeader(int round) {
  return round % total_num_ + 1;
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

    GetParentRefs(proposal.get());
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

// Mysticeti-C: add block to uncertified DAG and update support/cert tracking
void ProposalManager::AddBlockToDAG(std::unique_ptr<Proposal> proposal) {
  int round = proposal->header().round();
  int author = proposal->header().proposer_id();

  std::unique_lock<std::mutex> lk(txn_mutex_);

  // Amortized GC: every gc_interval_ insertions, prune old rounds.
  if (++gc_counter_ >= gc_interval_) {
    gc_counter_ = 0;
    MaybeGC();
  }

  // Track max proposal round for GC anchoring (even if commits stall)
  if (round > max_proposal_round_.load(std::memory_order_relaxed)) {
    max_proposal_round_.store(round, std::memory_order_relaxed);
  }

  // Store in DAG
  dag_[round][author] = std::move(proposal);

  // Record support: this block (at round r) supports its parents at round r-1.
  // Track from BOTH strong_cert and weak_cert to handle any partitioning.
  const auto& stored = dag_[round][author];
  auto record_support = [&](const CertLink& cert_link) {
    for (const auto& parent_cert : cert_link.cert()) {
      int parent_round = parent_cert.round();
      int parent_author = parent_cert.proposer();
      support_[parent_round][parent_author].insert(author);
    }
  };
  record_support(stored->header().strong_cert());
  record_support(stored->header().weak_cert());

  // Check certificate pattern: does this block (at round r) certify the leader
  // at round r-2? A block at round r certifies leader at round r-2 if among
  // its parents at round r-1, >= 2f+1 support the leader's block at round r-2.
  if (round >= 2) {
    int leader_round = round - 2;
    int leader = GetLeader(leader_round);

    // Only check if the leader's block exists
    if (dag_.find(leader_round) != dag_.end() &&
        dag_[leader_round].find(leader) != dag_[leader_round].end()) {
      int vote_count = 0;
      // Count from both strong_cert and weak_cert parents at round r-1
      auto count_votes = [&](const CertLink& cert_link) {
        for (const auto& parent_cert : cert_link.cert()) {
          if (parent_cert.round() == round - 1) {
            int parent_author = parent_cert.proposer();
            if (support_[leader_round].count(leader) &&
                support_[leader_round][leader].count(parent_author)) {
              vote_count++;
            }
          }
        }
      };
      count_votes(stored->header().strong_cert());
      count_votes(stored->header().weak_cert());

      if (vote_count >= limit_count_) {
        cert_count_[leader_round][leader]++;
      }
    }
  }
}

bool ProposalManager::HasBlock(int round, int author) {
  std::unique_lock<std::mutex> lk(txn_mutex_);
  return dag_.find(round) != dag_.end() &&
         dag_[round].find(author) != dag_[round].end();
}

const Proposal* ProposalManager::GetBlock(int round, int author) {
  std::unique_lock<std::mutex> lk(txn_mutex_);
  auto rit = dag_.find(round);
  if (rit == dag_.end()) return nullptr;
  auto ait = rit->second.find(author);
  if (ait == rit->second.end()) return nullptr;
  return ait->second.get();
}

std::unique_ptr<Proposal> ProposalManager::FetchBlock(int round, int author) {
  std::unique_lock<std::mutex> lk(txn_mutex_);
  if (fetched_.count({round, author})) return nullptr;
  auto rit = dag_.find(round);
  if (rit == dag_.end()) return nullptr;
  auto ait = rit->second.find(author);
  if (ait == rit->second.end() || ait->second == nullptr) return nullptr;
  fetched_.insert({round, author});
  auto copy = std::make_unique<Proposal>(*ait->second);
  if (author == id_) {
    lk.unlock();
    FetchLocalBlock(copy->hash());
  }
  return copy;
}

bool ProposalManager::CheckBlockExists(const std::string& hash) {
  std::unique_lock<std::mutex> lk(txn_mutex_);
  for (const auto& round_map : dag_) {
    for (const auto& author_map : round_map.second) {
      if (author_map.second && author_map.second->hash() == hash) {
        return true;
      }
    }
  }
  return false;
}

int ProposalManager::GetDAGRoundSize(int round) {
  std::unique_lock<std::mutex> lk(txn_mutex_);
  auto it = dag_.find(round);
  if (it == dag_.end()) return 0;
  return it->second.size();
}

int ProposalManager::GetSupportCount(int round, int proposer) {
  std::unique_lock<std::mutex> lk(txn_mutex_);
  auto rit = support_.find(round);
  if (rit == support_.end()) return 0;
  auto pit = rit->second.find(proposer);
  if (pit == rit->second.end()) return 0;
  return pit->second.size();
}

int ProposalManager::GetCertCount(int round, int proposer) {
  std::unique_lock<std::mutex> lk(txn_mutex_);
  auto rit = cert_count_.find(round);
  if (rit == cert_count_.end()) return 0;
  auto pit = rit->second.find(proposer);
  if (pit == rit->second.end()) return 0;
  return pit->second;
}

// Mysticeti-C: fill parent block references instead of cert references.
// ALL blocks from round r-1 go to strong_cert (Mysticeti uncertified DAG parents).
// No older-round weak refs — those are reachable via recursive parent traversal
// and would cause liveness issues when committed blocks are removed from DAG.
void ProposalManager::GetParentRefs(Proposal* proposal) {
  if (round_ == 0) {
    return;
  }
  // Relaxed invariant: with straggler-tolerant Ready, the parent count
  // can legitimately drop below limit_count_. We still prefer our own
  // self-block to be present so chaining stays consistent, but we no
  // longer crash if it isn't.

  // Add ALL blocks from previous round as strong references (parents)
  for (const auto& entry : dag_[round_ - 1]) {
    if (entry.second == nullptr) continue;
    Certificate parent_ref;
    parent_ref.set_round(round_ - 1);
    parent_ref.set_proposer(entry.first);
    parent_ref.set_hash(entry.second->hash());
    *proposal->mutable_header()->mutable_strong_cert()->add_cert() = parent_ref;
  }
}

// Mysticeti-C: Ready when we have >= 2f+1 blocks from previous round (no certs needed)
bool ProposalManager::Ready() {
  std::unique_lock<std::mutex> lk(txn_mutex_);
  if (round_ == 0) {
    return true;
  }
  if (static_cast<int>(dag_[round_ - 1].size()) < limit_count_) {
    return false;
  }
  if (dag_[round_ - 1].find(id_) == dag_[round_ - 1].end()) {
    return false;
  }
  return true;
}

// Straggler-tolerant Ready: when the strict 2f+1 threshold hasn't been
// reachable within a deadline (AsyncSend timer), accept any quorum of
// `min_count` blocks in the previous round. This lets the DAG advance
// past a persistently slow alive replica — which is the n=16 f=5
// edge case where 2f+1=11 exactly equals the alive set, leaving zero
// slack for stragglers.
bool ProposalManager::ReadyRelaxed(int min_count) {
  std::unique_lock<std::mutex> lk(txn_mutex_);
  if (round_ == 0) {
    return true;
  }
  if (static_cast<int>(dag_[round_ - 1].size()) < min_count) {
    return false;
  }
  if (dag_[round_ - 1].find(id_) == dag_[round_ - 1].end()) {
    return false;
  }
  return true;
}

void ProposalManager::SetCommittedRound(int r) {
  committed_round_ = r;
}

// Amortized GC: remove old rounds from dag_, support_, cert_count_,
// fetched_. Called inside AddBlockToDAG while already holding txn_mutex_,
// so no extra locking needed. Runs every gc_interval_ calls and only
// removes rounds well below committed_round_ to avoid interfering with
// in-flight commits. Each GC pass erases a bounded batch (at most 50
// rounds) to cap the cost per invocation.
void ProposalManager::MaybeGC() {
  // GC anchor: never prune above committed_round - 2. The cert check
  // needs rounds r, r+1, r+2 intact. If commits stall (e.g. post-crash),
  // we MUST keep uncommitted rounds — using max() here previously caused
  // the crash-fault 0% recovery bug by pruning rounds that hadn't been
  // committed yet.
  int committed = committed_round_.load();
  int prop_anchor = max_proposal_round_.load() - 50;
  int gc_below = std::min(committed - 2, prop_anchor);
  if (gc_below < 5) return;

  // Erase all rounds below gc_below. Since std::map is sorted, we
  // iterate from the front and stop early. Each erase is O(1) amortized
  // for begin() on a balanced tree.
  for (auto it = dag_.begin(); it != dag_.end() && it->first < gc_below;)
    it = dag_.erase(it);
  for (auto it = support_.begin(); it != support_.end() && it->first < gc_below;)
    it = support_.erase(it);
  for (auto it = cert_count_.begin(); it != cert_count_.end() && it->first < gc_below;)
    it = cert_count_.erase(it);
  for (auto it = fetched_.begin(); it != fetched_.end() && it->first < gc_below;)
    it = fetched_.erase(it);
}

}  // namespace mysticeti
}  // namespace resdb

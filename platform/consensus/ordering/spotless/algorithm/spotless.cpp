#include "platform/consensus/ordering/spotless/algorithm/spotless.h"

#include <glog/logging.h>

#include <algorithm>
#include <chrono>

#include "common/utils/utils.h"

namespace resdb {
namespace spotless {
namespace {

std::string BuildCertPayload(int32_t instance_id, int32_t view,
                             const std::string& hash) {
  return std::to_string(instance_id) + ":" + std::to_string(view) + ":" +
         hash;
}

std::string BuildRapidPayload(int32_t instance_id, int32_t from_view,
                              int32_t target_view, int32_t sender_id) {
  return std::to_string(instance_id) + ":" + std::to_string(from_view) + ":" +
         std::to_string(target_view) + ":" + std::to_string(sender_id);
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

std::string BuildTxnKey(const Transaction& txn) {
  if (!txn.hash().empty()) {
    return txn.hash();
  }
  std::string key = txn.data();
  key.append("#");
  key.append(std::to_string(txn.proxy_id()));
  key.append("#");
  key.append(std::to_string(txn.user_seq()));
  key.append("#");
  key.append(std::to_string(txn.timestamp()));
  return key;
}

bool HasBetterCert(const Certificate& lhs, const Certificate& rhs) {
  if (lhs.hash().empty()) {
    return false;
  }
  if (rhs.hash().empty()) {
    return true;
  }
  if (lhs.view() != rhs.view()) {
    return lhs.view() > rhs.view();
  }
  return lhs.hash() != rhs.hash();
}

}  // namespace

SpotLess::SpotLess(int id, int f, int total_num, int block_size,
                   SignatureVerifier* verifier)
    : ProtocolBase(id, f, total_num),
      verifier_(verifier),
      batch_size_(block_size),
      timeout_ms_(100),
      total_instances_(std::max(1, total_num)),
      next_execute_slot_(1),
      execute_id_(1),
      global_stats_(Stats::GetGlobalStats()),
      start_(false) {

  for (int instance_id = 0; instance_id < total_instances_; ++instance_id) {
    InstanceContext context;
    context.instance_id = instance_id;
    context.manager = std::make_unique<ProposalManager>(
        id_, instance_id, 2 * f_ + 1, verifier_);
    context.view_start_time = GetCurrentTime();
    instances_.emplace(instance_id, std::move(context));
  }

  for (int instance_id = 0; instance_id < total_instances_; ++instance_id) {
    send_threads_.emplace_back(&SpotLess::AsyncSend, this, instance_id);
  }
  commit_thread_ = std::thread(&SpotLess::AsyncCommit, this);
  timeout_thread_ = std::thread(&SpotLess::AsyncViewTimeout, this);
}

SpotLess::~SpotLess() {
  Stop();
  vote_cv_.notify_all();
  timeout_cv_.notify_all();
  for (auto& send_thread : send_threads_) {
    if (send_thread.joinable()) {
      send_thread.join();
    }
  }
  if (commit_thread_.joinable()) {
    commit_thread_.join();
  }
  if (timeout_thread_.joinable()) {
    timeout_thread_.join();
  }
}

void SpotLess::Start() {
  start_ = true;
  vote_cv_.notify_all();
  timeout_cv_.notify_all();
}

int SpotLess::LeaderFor(int instance_id, int view) const {
  return ((instance_id + view - 1) % total_num_) + 1;
}

bool SpotLess::IsLeader(int instance_id, int view) const {
  return LeaderFor(instance_id, view) == id_;
}

int SpotLess::ComputeInstanceId(const Transaction& txn) const {
  if (total_instances_ <= 1) {
    return 0;
  }
  const std::string key = BuildTxnKey(txn);
  int instance_id = 0;
  if (key.empty()) {
    instance_id = (txn.proxy_id() + txn.user_seq()) % total_instances_;
  } else {
    std::hash<std::string> hasher;
    instance_id = hasher(key) % total_instances_;
  }
  
  //LOG(ERROR) << "[spotless_dbg] ComputeInstanceId node:" << id_
  //           << " proxy:" << txn.proxy_id()
  //           << " seq:" << txn.user_seq()
  //           << " total_instances:" << total_instances_
  //           << " result_instance:" << instance_id;
  
  return instance_id;
}

LockFreeQueue<Transaction>* SpotLess::GetInstanceQueue(int instance_id) const {
  return const_cast<LockFreeQueue<Transaction>*>(&txns_);
}

bool SpotLess::HasReadyInstanceLocked(int instance_id) const {
  auto it = instances_.find(instance_id);
  if (it == instances_.end()) {
    return false;
  }
  const InstanceContext& instance = it->second;
  const int view = instance.manager->CurrentView();
  bool is_leader = IsLeader(instance.instance_id, view);
  bool not_proposed = instance.proposed_views.find(view) == instance.proposed_views.end();
  return is_leader && not_proposed;
}

void SpotLess::NoteViewProgressLocked(InstanceContext* instance, int view) {
  if (instance == nullptr) {
    return;
  }
  instance->view_start_time = GetCurrentTime();
  if (instance->timeout_sent_view >= view) {
    instance->timeout_sent_view = view - 1;
  }
}

std::vector<std::unique_ptr<Transaction>> SpotLess::PopTransactionsForInstance(
    int instance_id) {
  std::vector<std::unique_ptr<Transaction>> txns;
  LockFreeQueue<Transaction>* queue = nullptr;
  {
    std::unique_lock<std::mutex> lk(mutex_);
    queue = GetInstanceQueue(0);
  }
  if (queue == nullptr) {
    return txns;
  }

  auto txn = queue->Pop(); 
  if (txn == nullptr) {
    return txns;  
  }

  const std::string txn_key = BuildTxnKey(*txn);
  bool committed = false;
  {
    std::unique_lock<std::mutex> lk(mutex_);
    committed = !txn_key.empty() && committed_txns_.count(txn_key) > 0;
    if (committed) {
      seen_txns_.erase(txn_key);
    }
  }
  
  if (!committed) {
    txns.push_back(std::move(txn));
  }

  for (int i = 1; i < batch_size_; ++i) {
    auto txn = queue->Pop(100);
    if (txn == nullptr) {
      break;
    }

    *txns[0]->add_subtxn() = *txn;
	/*
    const std::string txn_key = BuildTxnKey(*txn);
    bool committed = false;
    {
      std::unique_lock<std::mutex> lk(mutex_);
      committed = !txn_key.empty() && committed_txns_.count(txn_key) > 0;
      if (committed) {
        seen_txns_.erase(txn_key);
      }
    }
    if (committed) {
      continue;  
    }
    txns.push_back(std::move(txn));
*/
  }
  
  return txns;
}

SyncMessage SpotLess::BuildSyncMessageLocked(
    const Proposal& proposal, const Certificate& highest_cert) const {
  SyncMessage sync;
  sync.set_instance_id(proposal.header().instance_id());
  sync.set_view(proposal.header().view());
  sync.set_sender_id(id_);
  sync.mutable_claim()->set_instance_id(proposal.header().instance_id());
  sync.mutable_claim()->set_view(proposal.header().view());
  sync.mutable_claim()->set_hash(proposal.hash());
  sync.mutable_claim()->set_prehash(proposal.header().prehash());
  sync.mutable_claim()->set_proposer_id(proposal.header().proposer_id());
  if (!highest_cert.hash().empty()) {
    *sync.mutable_highest_cert() = highest_cert;
  }
  sync.set_retransmit(true);
  *sync.mutable_proposal() = proposal;

  const std::string payload =
      BuildCertPayload(sync.instance_id(), sync.view(), sync.claim().hash());
  auto signature_or = verifier_->SignMessage(payload);
  if (signature_or.ok()) {
    *sync.mutable_sign() = *signature_or;
  }
  return sync;
}

RapidSync SpotLess::BuildRapidSyncLocked(int instance_id, int from_view,
                                         int target_view,
                                         const Certificate& highest_cert) const {
  RapidSync msg;
  msg.set_instance_id(instance_id);
  msg.set_from_view(from_view);
  msg.set_target_view(target_view);
  msg.set_sender_id(id_);
  if (!highest_cert.hash().empty()) {
    *msg.mutable_highest_cert() = highest_cert;
  }
  msg.set_retransmit(true);
  const std::string payload =
      BuildRapidPayload(instance_id, from_view, target_view, id_);
  auto signature_or = verifier_->SignMessage(payload);
  if (signature_or.ok()) {
    *msg.mutable_sign() = *signature_or;
  }
  return msg;
}

Certificate SpotLess::BuildCertificateLocked(
    int instance_id, int view, const std::string& hash,
    const std::map<int, SyncMessage>& votes) const {
  Certificate cert;
  cert.set_instance_id(instance_id);
  cert.set_view(view);
  cert.set_hash(hash);
  /*
  for (const auto& entry : votes) {
    cert.add_signer_ids(entry.first);
    *cert.add_signatures() = entry.second.sign();
  }
  */
  return cert;
}

bool SpotLess::VerifySync(const SyncMessage& sync) const {
  if (sync.instance_id() != sync.claim().instance_id() ||
      sync.view() != sync.claim().view()) {
    return false;
  }
  const std::string payload =
      BuildCertPayload(sync.instance_id(), sync.view(), sync.claim().hash());
  if (!verifier_->VerifyMessage(payload, sync.sign())) {
    return false;
  }
  if (!sync.highest_cert().hash().empty()) {
    auto it = instances_.find(sync.instance_id());
    if (it == instances_.end() ||
        !it->second.manager->VerifyCertificate(sync.highest_cert())) {
      return false;
    }
  }
  return true;
}

bool SpotLess::VerifyRapidSync(const RapidSync& msg) const {
  const std::string payload = BuildRapidPayload(msg.instance_id(), msg.from_view(),
                                                msg.target_view(),
                                                msg.sender_id());
  if (!verifier_->VerifyMessage(payload, msg.sign())) {
    return false;
  }
  auto it = instances_.find(msg.instance_id());
  if (it == instances_.end()) {
    return false;
  }
  if (!msg.highest_cert().hash().empty() &&
      !it->second.manager->VerifyCertificate(msg.highest_cert())) {
    return false;
  }
  return true;
}

bool SpotLess::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  if (txn == nullptr) {
    return false;
  }

  const std::string txn_key = BuildTxnKey(*txn);
  LockFreeQueue<Transaction>* queue = nullptr;
  {
    std::unique_lock<std::mutex> lk(mutex_);
    if (!txn_key.empty()) {
      if (committed_txns_.count(txn_key) > 0 || seen_txns_.count(txn_key) > 0) {
        return true;
      }
      seen_txns_.insert(txn_key);
    }
    queue = GetInstanceQueue(0);  
  }

  if (queue == nullptr) {
    return false;
  }

  start_ = true;
  txn->set_proposer(id_);
  queue->Push(std::move(txn));
  vote_cv_.notify_all();
  timeout_cv_.notify_all();
  return true;
}

void SpotLess::AsyncSend(int instance_id) {
  while (!IsStop()) {
    int view = 0;
    bool ready = false;
    {
      std::unique_lock<std::mutex> lk(mutex_);
      vote_cv_.wait_for(lk, std::chrono::milliseconds(100), [&] {
        ready = IsStop() || (start_.load() && HasReadyInstanceLocked(instance_id));
        return ready;
      });
      if (IsStop()) {
        return;
      }
      
      if (!ready) {
        continue;
      }
      
      LOG(ERROR)<<"instance id redy:"<<HasReadyInstanceLocked(instance_id)<<" instance id:"<<instance_id;
      if(!HasReadyInstanceLocked(instance_id)){
        continue;
      }
      InstanceContext& instance = instances_[instance_id];
      view = instance.manager->CurrentView();
      instance.proposed_views.insert(view);
      NoteViewProgressLocked(&instance, view);
    }

    LOG(ERROR)<<" broadcast proposal 1:"<<instance_id<<" view:"<<view;
    std::vector<std::unique_ptr<Transaction>> txns =
        PopTransactionsForInstance(instance_id);
    LOG(ERROR)<<" broadcast proposal 2:"<<instance_id<<" view:"<<view;
    
    std::unique_ptr<Proposal> proposal;
    {
      std::unique_lock<std::mutex> lk(mutex_);
      proposal = instances_[instance_id].manager->GenerateProposal(view, txns);
    }

    if (!txns.empty()) {
      global_stats_->AddBlockSize(txns.size());
    }
    LOG(ERROR)<<" broadcast proposal 3:"<<instance_id<<" view:"<<view;
    Broadcast(MessageType::ProposalMsg, *proposal);
    LOG(ERROR)<<" broadcast proposal 4:"<<instance_id<<" view:"<<view<<" ready:"<<HasReadyInstanceLocked(instance_id);
  }
}

bool SpotLess::ReceiveProposal(std::unique_ptr<Proposal> proposal) {
  if (proposal == nullptr) {
    return false;
  }

  start_ = true;
  const int instance_id = proposal->header().instance_id();
  const int view = proposal->header().view();
  const std::string hash = proposal->hash();
  std::unique_ptr<SyncMessage> sync_to_send;

  {
    std::unique_lock<std::mutex> lk(instance_mutex_[instance_id]);
    auto it = instances_.find(instance_id);
    if (it == instances_.end()) {
      LOG(ERROR) << "[spotless_dbg] recv_proposal_reject node:" << id_
                 << " from:" << proposal->header().proposer_id()
                 << " instance:" << instance_id << " view:" << view
                 << " reason:unknown_instance";
      return false;
    }
    InstanceContext& instance = it->second;
    if (LeaderFor(instance_id, view) != proposal->header().proposer_id()) {
      LOG(ERROR) << "[spotless_dbg] recv_proposal_reject node:" << id_
                 << " from:" << proposal->header().proposer_id()
                 << " instance:" << instance_id << " view:" << view
                 << " reason:leader_mismatch";
      LOG(ERROR) << "spotless proposal leader mismatch, instance:" << instance_id
                 << " view:" << view;
      return false;
    }
    if (!instance.manager->VerifyProposal(*proposal)) {
      LOG(ERROR) << "[spotless_dbg] recv_proposal_reject node:" << id_
                 << " from:" << proposal->header().proposer_id()
                 << " instance:" << instance_id << " view:" << view
                 << " reason:verify_fail";
      return false;
    }
    instance.manager->RecordProposal(std::make_unique<Proposal>(*proposal));
    if (view > instance.manager->CurrentView()) {
      instance.manager->AdvanceView(view);
    }
    NoteViewProgressLocked(&instance,
                           std::max(view, instance.manager->CurrentView()));

    LOG(ERROR) << "[spotless_dbg] recv_proposal_ok node:" << id_
               << " from:" << proposal->header().proposer_id()
               << " instance:" << instance_id << " view:" << view
               << " txns:" << proposal->transactions_size();

    auto accepted_it = instance.accepted_hash.find(view);
    if (accepted_it == instance.accepted_hash.end()) {
      instance.accepted_hash[view] = hash;
      if (instance.sync_sent_views.insert(view).second) {
        sync_to_send = std::make_unique<SyncMessage>(BuildSyncMessageLocked(
            *proposal, instance.manager->GetHighestPreparedCert()));
      }
    }
  }

  if (sync_to_send != nullptr) {
    LOG(ERROR) << "[spotless_dbg] send_sync node:" << id_
               << " instance:" << instance_id << " view:" << view;
    Broadcast(MessageType::SyncMsg, *sync_to_send);
  }
  return true;
}

bool SpotLess::ReceiveSync(std::unique_ptr<SyncMessage> sync) {
  if (sync == nullptr) {
    return false;
  }

  start_ = true;
  std::unique_ptr<Proposal> committed;
  bool should_notify = false;
  const int instance_id = sync->instance_id();
  const int view = sync->view();
  const std::string hash = sync->claim().hash();

  {
    std::unique_lock<std::mutex> lk(instance_mutex_[instance_id]);
    auto it = instances_.find(instance_id);
    if (it == instances_.end()) {
      LOG(ERROR) << "[spotless_dbg] recv_sync_reject node:" << id_
                 << " from:" << sync->sender_id()
                 << " instance:" << instance_id << " view:" << view
                 << " reason:unknown_instance";
      return false;
    }
    InstanceContext& instance = it->second;
    if (!VerifySync(*sync)) {
      LOG(ERROR) << "[spotless_dbg] recv_sync_reject node:" << id_
                 << " from:" << sync->sender_id()
                 << " instance:" << instance_id << " view:" << view
                 << " reason:verify_fail";
      return false;
    }

    if (!sync->highest_cert().hash().empty()) {
      instance.manager->UpdateHighestPrepared(sync->highest_cert());
    }
    if (!sync->proposal().hash().empty() && sync->proposal().hash() == hash &&
        instance.manager->VerifyProposal(sync->proposal())) {
      instance.manager->RecordProposal(std::make_unique<Proposal>(sync->proposal()));
    }

    auto& votes = sync_votes_[instance_id][view][hash];
    votes[sync->sender_id()] = *sync;
    const int vote_count = votes.size();
    LOG(ERROR) << "[spotless_dbg] recv_sync_vote node:" << id_
               << " from:" << sync->sender_id()
               << " instance:" << instance_id << " view:" << view
               << " vote_count:" << vote_count;

    if (view > instance.manager->CurrentView() && vote_count >= f_ + 1) {
      LOG(ERROR) << "[spotless_dbg] sync_f1 node:" << id_
                 << " instance:" << instance_id << " view:" << view
                 << " vote_count:" << vote_count;
      instance.manager->AdvanceView(view);
      NoteViewProgressLocked(&instance, view);
      should_notify = true;
    }

    if (vote_count >= 2 * f_ + 1 && instance.prepared_views.insert(view).second) {
      LOG(ERROR) << "[spotless_dbg] sync_qc node:" << id_
                 << " instance:" << instance_id << " view:" << view
                 << " vote_count:" << vote_count;
      Certificate cert = BuildCertificateLocked(instance_id, view, hash, votes);
      committed = instance.manager->MarkPrepared(cert);
      int view_before_advance = instance.manager->CurrentView();
      instance.manager->AdvanceView(view + 1);
      int view_after_advance = instance.manager->CurrentView();
      LOG(ERROR) << "[spotless_dbg] sync_qc_advance node:" << id_
                 << " instance:" << instance_id
                 << " view_before:" << view_before_advance
                 << " view_after:" << view_after_advance
                 << " target_view:" << (view + 1);
      NoteViewProgressLocked(&instance, view + 1);
      should_notify = true;
      LOG(ERROR) << "[spotless_dbg] sync_qc node:" << id_
                 << " instance:" << instance_id << " view:" << view
                 << " vote_count:" << vote_count
                 << " committed:"<< (committed == nullptr);
    }
  }

  if (committed != nullptr) {
    CommitProposal(std::move(committed));
  }
  if (should_notify) {
    std::unique_lock<std::mutex> lk(mutex_);
    vote_cv_.notify_all();
    timeout_cv_.notify_all();
  }
  return true;
}

bool SpotLess::ReceiveRapidSync(std::unique_ptr<RapidSync> rapid_sync) {
  if (rapid_sync == nullptr) {
    return false;
  }

  start_ = true;
  bool should_notify = false;
  {
	std::unique_lock<std::mutex> lk(instance_mutex_[rapid_sync->instance_id()]);
    auto it = instances_.find(rapid_sync->instance_id());
    if (it == instances_.end()) {
      LOG(ERROR) << "[spotless_dbg] recv_rapid_sync_reject node:" << id_
                 << " from:" << rapid_sync->sender_id()
                 << " instance:" << rapid_sync->instance_id()
                 << " target_view:" << rapid_sync->target_view()
                 << " reason:unknown_instance";
      return false;
    }
    InstanceContext& instance = it->second;
    if (!VerifyRapidSync(*rapid_sync)) {
      LOG(ERROR) << "[spotless_dbg] recv_rapid_sync_reject node:" << id_
                 << " from:" << rapid_sync->sender_id()
                 << " instance:" << rapid_sync->instance_id()
                 << " target_view:" << rapid_sync->target_view()
                 << " reason:verify_fail";
      return false;
    }

    if (!rapid_sync->highest_cert().hash().empty()) {
      instance.manager->UpdateHighestPrepared(rapid_sync->highest_cert());
      auto& best =
          rapid_sync_high_cert_[instance.instance_id][rapid_sync->target_view()];
      if (HasBetterCert(rapid_sync->highest_cert(), best)) {
        best = rapid_sync->highest_cert();
      }
    }

    auto& ack = rapid_sync_ack_[instance.instance_id][rapid_sync->target_view()];
    ack.insert(rapid_sync->sender_id());
    LOG(ERROR) << "[spotless_dbg] recv_rapid_sync node:" << id_
               << " from:" << rapid_sync->sender_id()
               << " instance:" << rapid_sync->instance_id()
               << " from_view:" << rapid_sync->from_view()
               << " target_view:" << rapid_sync->target_view()
               << " ack_size:" << ack.size()
               << " highest_cert_view:" << rapid_sync->highest_cert().view();
    if (rapid_sync->target_view() > instance.manager->CurrentView() &&
        static_cast<int>(ack.size()) >= f_ + 1) {
      auto best_it = rapid_sync_high_cert_[instance.instance_id].find(
          rapid_sync->target_view());
      if (best_it != rapid_sync_high_cert_[instance.instance_id].end() &&
          !best_it->second.hash().empty()) {
        instance.manager->UpdateHighestPrepared(best_it->second);
      }
      LOG(ERROR) << "[spotless_dbg] advance_view_rvs node:" << id_
                 << " instance:" << rapid_sync->instance_id()
                 << " target_view:" << rapid_sync->target_view()
                 << " ack_size:" << ack.size();
      instance.manager->AdvanceView(rapid_sync->target_view());
      NoteViewProgressLocked(&instance, rapid_sync->target_view());
      should_notify = true;
    }
  }

  if (should_notify) {
    std::unique_lock<std::mutex> lk(mutex_);
    vote_cv_.notify_all();
    timeout_cv_.notify_all();
  }
  return true;
}

void SpotLess::SendRapidSync(int instance_id, int target_view) {
  RapidSync msg;
  {
	std::unique_lock<std::mutex> lk(instance_mutex_[instance_id]);
    auto it = instances_.find(instance_id);
    if (it == instances_.end()) {
      return;
    }
    msg = BuildRapidSyncLocked(instance_id, it->second.manager->CurrentView(),
                               target_view,
                               it->second.manager->GetHighestPreparedCert());
  }
  LOG(ERROR) << "[spotless_dbg] send_rapid_sync node:" << id_
             << " instance:" << instance_id
             << " from_view:" << msg.from_view()
             << " target_view:" << target_view
             << " highest_cert_view:" << msg.highest_cert().view();
  Broadcast(MessageType::RapidSyncMsg, msg);
}

void SpotLess::AsyncViewTimeout() {
  const int timeout_us = timeout_ms_ * 1000*100;
  while (!IsStop()) {
    std::vector<std::pair<int, int>> timeouts;
    {
      std::unique_lock<std::mutex> lk(mutex_);
      timeout_cv_.wait_for(lk, std::chrono::microseconds(timeout_us));
      if (IsStop()) {
        return;
      }
      if (!start_) {
        continue;
      }
      const uint64_t now = GetCurrentTime();
      for (auto& entry : instances_) {
        InstanceContext& instance = entry.second;
        const int view = instance.manager->CurrentView();
        if (view <= instance.timeout_sent_view) {
          continue;
        }
        if (now > instance.view_start_time &&
            now - instance.view_start_time >= static_cast<uint64_t>(timeout_us)) {
          instance.timeout_sent_view = view;
          LOG(ERROR) << "[spotless_dbg] timeout_fire node:" << id_
                     << " instance:" << instance.instance_id
                     << " view:" << view
                     << " elapsed_us:" << (now - instance.view_start_time)
                     << " target_view:" << (view + 1);
          timeouts.push_back(std::make_pair(instance.instance_id, view + 1));
        }
      }
    }

    for (const auto& timeout : timeouts) {
      SendRapidSync(timeout.first, timeout.second);
    }
  }
}

void SpotLess::CommitProposal(std::unique_ptr<Proposal> proposal) {
  if (proposal == nullptr) {
    return;
  }
  //LOG(ERROR) << "[spotless_dbg] commit_enqueue node:" << id_
  //           << " instance:" << proposal->header().instance_id()
  //           << " view:" << proposal->header().view()
  //           << " txns:" << proposal->transactions_size();
  commit_q_.Push(std::move(proposal));
}

void SpotLess::RememberCommittedTransactionsLocked(const Proposal& proposal) {
  for (const auto& txn : proposal.transactions()) {
    const std::string txn_key = BuildTxnKey(txn);
    if (txn_key.empty()) {
      continue;
    }
    committed_txns_.insert(txn_key);
    seen_txns_.erase(txn_key);
  }
}

void SpotLess::AsyncCommit() {
  struct ReadyCommit {
    int slot = 0;
    int instance_id = 0;
    int view = 0;
    std::string hash;
    std::vector<Transaction> txns;
  };

  while (!IsStop()) {
    auto proposal = commit_q_.Pop();
    if (proposal == nullptr) {
      continue;
    }

    std::vector<ReadyCommit> ready_commits;
    const int commit_view = proposal->header().view();
    const int instance_id = proposal->header().instance_id();
    const std::string proposal_hash = proposal->hash();
    {
      std::unique_lock<std::mutex> lk(mutex_);
      const int slot_id = ++instance_commit_slots_[instance_id];
      //LOG(ERROR) << "[spotless_dbg] commit_slot_assign node:" << id_
      //           << " instance:" << instance_id
      //           << " slot:" << slot_id
      //           << " commit_view:" << commit_view;
      auto& slot = committed_by_slot_[slot_id][instance_id];
      if (slot == nullptr) {
        slot = std::move(proposal);
      }

      while (true) {
        auto it = committed_by_slot_.find(next_execute_slot_);
        if (it == committed_by_slot_.end()) {
          LOG(ERROR) << "[spotless_dbg] commit_wait node:" << id_
                     << " next_slot:" << next_execute_slot_
                     << " reason:slot_not_ready";
          break;
        }
        if (static_cast<int>(it->second.size()) < total_instances_) {
          std::string missing_instances;
          for (int instance_id_check = 0; instance_id_check < total_instances_;
               ++instance_id_check) {
            if (it->second.find(instance_id_check) != it->second.end()) {
              continue;
            }
            if (!missing_instances.empty()) {
              missing_instances.append(",");
            }
            missing_instances.append(std::to_string(instance_id_check));
          }
          LOG(ERROR) << "[spotless_dbg] commit_wait node:" << id_
                     << " next_slot:" << next_execute_slot_
                     << " ready_instances:" << it->second.size()
                     << " missing_instances:" << missing_instances;
          break;
        }

        std::vector<const Proposal*> ordered_proposals;
        ordered_proposals.reserve(it->second.size());
        for (const auto& entry : it->second) {
          ordered_proposals.push_back(entry.second.get());
        }
        std::sort(ordered_proposals.begin(), ordered_proposals.end(),
                  [](const Proposal* lhs, const Proposal* rhs) {
                    if (lhs->header().view() != rhs->header().view()) {
                      return lhs->header().view() < rhs->header().view();
                    }
                    return lhs->header().instance_id() <
                           rhs->header().instance_id();
                  });

        for (const Proposal* committed_ptr : ordered_proposals) {
          const Proposal& committed = *committed_ptr;
          RememberCommittedTransactionsLocked(committed);
          if (committed.create_time() > 0) {
            global_stats_->AddLatency(GetCurrentTime() - committed.create_time());
          }
          ReadyCommit ready;
          ready.slot = next_execute_slot_;
          ready.instance_id = committed.header().instance_id();
          ready.view = committed.header().view();
          ready.hash = committed.hash();
          for (const auto& txn : committed.transactions()) {
            ready.txns.push_back(txn);
          }
          ready_commits.push_back(std::move(ready));
        }
        LOG(ERROR)<<" commit ready txns:"<<ready_commits.size();
        committed_by_slot_.erase(it);
        ++next_execute_slot_;
      }
    }

    for (auto& ready : ready_commits) {
      LOG(ERROR) << "[spotless_dbg] execute_instance node:" << id_
                 << " slot:" << ready.slot
                 << " instance:" << ready.instance_id
                 << " commit_view:" << ready.view
                 << " txn_count:" << ready.txns.size();
      for (auto& txn : ready.txns) {
        //LOG(ERROR) << "[spotless_dbg] commit_txn node:" << id_
        //           << " exec_id:" << txn.id()
        //           << " slot:" << ready.slot
        //           << " instance:" << ready.instance_id
        //           << " proxy:" << txn.proxy_id()
        // << " sub txn size:"<<txn.subtxn_size();
	for(auto& subtxn: *txn.mutable_subtxn()){
		subtxn.set_id(execute_id_++);
		Commit(subtxn);
}
txn.clear_subtxn();
        txn.set_id(execute_id_++);
        Commit(txn);
      }
    }
  }
}

}  // namespace spotless
}  // namespace resdb

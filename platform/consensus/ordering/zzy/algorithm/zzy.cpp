#include "platform/consensus/ordering/zzy/algorithm/zzy.h"

#include <glog/logging.h>

#include <chrono>
#include <thread>

#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"
#include "platform/proto/resdb.pb.h"

namespace resdb {
namespace zzy {

namespace {

std::string BuildPrepareDigest(int view, int64_t seq, const std::string& hash,
                               int replica) {
  return std::to_string(view) + ":" + std::to_string(seq) + ":" + hash + ":" +
         std::to_string(replica);
}

}  // namespace

ZZY::ZZY(const ResDBConfig& config, int id, int f, int total_num,
         int batch_size, SignatureVerifier* verifier)
    : ProtocolBase(id, f, total_num),
      batch_size_(batch_size),
      view_(1),
      seq_(1),
      is_stop_(false),
      client_time_limit_(100000),
      config_(config),
      verifier_(verifier),
      global_stats_(Stats::GetGlobalStats()),
      commit_seq_(0) {
  commit_thread_ = std::thread(&ZZY::AsyncCommit, this);
  if (IsPrimary()) {
    send_thread_ = std::thread(&ZZY::AsyncSend, this);
  }
  if (IsClient()) {
    client_thread_ = std::thread(&ZZY::AsyncClientCheck, this);
  }
}

ZZY::~ZZY() {
  is_stop_ = true;
  client_cv_.notify_all();
  if (commit_thread_.joinable()) {
    commit_thread_.join();
  }
  if (send_thread_.joinable()) {
    send_thread_.join();
  }
  if (client_thread_.joinable()) {
    client_thread_.join();
  }
}

bool ZZY::IsStop() { return is_stop_; }

int ZZY::PrimaryId() const { return (view_ - 1) % total_num_ + 1; }

int ZZY::ClientId() const { return PrimaryId(); }

bool ZZY::IsPrimary() const { return id_ == PrimaryId(); }

bool ZZY::IsClient() const { return id_ == ClientId(); }

int64_t ZZY::ParseLocalId(const Transaction& txn) const {
  if (txn.local_id() > 0) {
    return txn.local_id();
  }
  BatchUserRequest batch;
  if (batch.ParseFromString(txn.data())) {
    return batch.local_id();
  }
  return 0;
}

void ZZY::SetFailFunc(std::function<void(const Transaction& txn)> func) {
  fail_func_ = func;
}

void ZZY::SetSpeculativeExecuteFunc(
    std::function<int(const Transaction& txn)> func) {
  speculative_execute_func_ = std::move(func);
}

void ZZY::SetClientCompleteFunc(
    std::function<void(int64_t local_id)> func) {
  client_complete_func_ = std::move(func);
}

void ZZY::AsyncCommit() {
  while (!IsStop()) {
    auto proposal = commit_q_.Pop();
    if (proposal == nullptr) {
      continue;
    }

    const int64_t seq = proposal->seq();
    const int bucket = seq % 1000;
    const std::string hash = proposal->hash();

    std::unique_ptr<Transaction> txn = nullptr;
    while (txn == nullptr && !IsStop()) {
      std::unique_lock<std::mutex> lk(mutex_[bucket]);
      auto it = data_[bucket].find(hash);
      if (it != data_[bucket].end()) {
        txn = std::move(it->second);
        data_[bucket].erase(it);
      }
    }
    if (txn == nullptr) {
      continue;
    }

    global_stats_->AddLatency(GetCurrentTime() - txn->create_time());
    global_stats_->IncCommit();
    commit_(*txn);
    {
      std::unique_lock<std::mutex> lk(commit_mutex_[bucket]);
      committed_[bucket].insert(hash);
      enqueued_[bucket].erase(hash);
    }
  }
}

void ZZY::AsyncClientCheck() {
  while (!IsStop()) {
    int64_t retry_proposal = 0;
    int64_t retry_commit = 0;
    {
      std::unique_lock<std::mutex> lk(client_mutex_);
      client_cv_.wait_for(lk, std::chrono::microseconds(10000), [&] {
        return IsStop();
      });
      const int64_t now = GetCurrentTime();
      for (const auto& it : client_start_time_) {
        const int64_t local_id = it.first;
        if (client_done_.find(local_id) != client_done_.end()) {
          continue;
        }
        if (now - it.second <= client_time_limit_) {
          continue;
        }
        if (client_last_retry_time_[local_id] > 0 &&
            now - client_last_retry_time_[local_id] <= client_time_limit_) {
          continue;
        }
        client_last_retry_time_[local_id] = now;
        const int reply_num =
            static_cast<int>(client_reply_senders_[local_id].size());
        if (reply_num >= total_num_) {
          continue;
        }
        if (reply_num >= 2 * f_ + 1) {
          retry_commit = local_id;
        } else {
          retry_proposal = local_id;
        }
        break;
      }
    }

    // Only retry the earliest unfinished request. Retrying every timed-out
    // round at once creates bursty traffic after partition recovery and keeps
    // the ordered executor oscillating around old gaps.
    if (retry_proposal > 0) {
      RebroadcastPendingTransaction(retry_proposal);
    } else if (retry_commit > 0) {
      SendClientCommit(retry_commit);
      TryClientSlowComplete(retry_commit);
    }
  }
}

void ZZY::AsyncSend() {
  const int target_batch_size = batch_size_ > 0 ? batch_size_ : 1;
  while (!IsStop()) {
    auto txn = txns_.Pop();
    if (txn == nullptr) {
      continue;
    }

    std::vector<std::unique_ptr<Transaction>> txns;
    txns.push_back(std::move(txn));
    for (int i = 1; i < target_batch_size; ++i) {
      auto next_txn = txns_.Pop(100);
      if (next_txn == nullptr) {
        break;
      }
      txns.push_back(std::move(next_txn));
    }

/*
    const int water_mark = config_.GetConfigData().water_mark();
    while (water_mark > 0 && seq_ - commit_seq_ > water_mark && !IsStop()) {
      std::this_thread::sleep_for(std::chrono::microseconds(1000));
    }
    */
    if (IsStop()) {
      return;
    }

    TransactionBatch batch;
    const int64_t send_start_time = GetCurrentTime();
    for (auto& txn : txns) {
      const int64_t local_id = ParseLocalId(*txn);
      txn->set_local_id(local_id);
      if (txn->uid() == 0) {
        txn->set_uid(local_id);
      }
      txn->set_seq(seq_++);
      txn->set_proposer(id_);
      txn->set_view(view_);
      txn->set_create_time(send_start_time);

      MarkClientPending(local_id, send_start_time);
      if (IsClient() && local_id > 0) {
        std::unique_lock<std::mutex> lk(client_mutex_);
        client_pending_txn_[local_id] = *txn;
      }
      *batch.add_transactions() = *txn;
    }

    if (batch.transactions_size() <= 0) {
      continue;
    }
    LOG(ERROR) << "zzy propose batch size:" << batch.transactions_size()
               << " first seq:" << batch.transactions(0).seq();
    Broadcast(MessageType::ProposeBatch, batch);
    for (const auto& proposed_txn : batch.transactions()) {
      ReceivePropose(std::make_unique<Transaction>(proposed_txn));
    }
  }
}

void ZZY::MarkClientPending(int64_t local_id, int64_t create_time) {
  if (!IsClient() || local_id <= 0) {
    return;
  }
  std::unique_lock<std::mutex> lk(client_mutex_);
  if (client_done_.find(local_id) != client_done_.end()) {
    return;
  }
  if (client_start_time_.find(local_id) == client_start_time_.end()) {
    client_start_time_[local_id] =
        create_time > 0 ? create_time : GetCurrentTime();
  }
  client_cv_.notify_all();
}

bool ZZY::HasPrePrepare(int bucket, int64_t seq, const std::string& hash) {
  return pre_prepare_[bucket].find(seq) != pre_prepare_[bucket].end() &&
         pre_prepare_[bucket][seq] == hash;
}

bool ZZY::MarkPrePrepare(int bucket, int64_t seq, const std::string& hash) {
  pre_prepare_[bucket][seq] = hash;
  return true;
}

bool ZZY::VerifyPrePrepare(const Transaction& txn) {
  if (txn.proposer() != PrimaryId()) {
    LOG(ERROR) << "invalid pre-prepare primary:" << txn.proposer()
               << " expect:" << PrimaryId();
    return false;
  }
  if (txn.view() != 0 && txn.view() != view_) {
    LOG(ERROR) << "invalid pre-prepare view:" << txn.view()
               << " expect:" << view_;
    return false;
  }
  return true;
}

bool ZZY::VerifyPrepare(const Proposal& proposal) {
  if (proposal.proposer() == PrimaryId()) {
    LOG(ERROR) << "primary should not send prepare";
    return false;
  }
  if (proposal.view() != 0 && proposal.view() != view_) {
    LOG(ERROR) << "invalid prepare view:" << proposal.view();
    return false;
  }
  return true;
}

bool ZZY::VerifyOrderReply(const Proposal& reply) {
  if (reply.proposer() <= 0 || reply.proposer() > total_num_) {
    return false;
  }
  if (reply.view() != 0 && reply.view() != view_) {
    return false;
  }
  const std::string digest = BuildPrepareDigest(
      reply.view() == 0 ? view_ : reply.view(), reply.seq(), reply.hash(),
      reply.proposer());
  if (!reply.has_data_signature()) {
    return false;
  }
  return verifier_->VerifyMessage(digest, reply.data_signature());
}

bool ZZY::TryFastCommit(int64_t seq, const std::string& hash) {
  const int bucket = seq % 1000;
  {
    std::unique_lock<std::mutex> clk(commit_mutex_[bucket]);
    if (committed_[bucket].find(hash) != committed_[bucket].end() ||
        enqueued_[bucket].find(hash) != enqueued_[bucket].end()) {
      return false;
    }
  }

  {
    std::unique_lock<std::mutex> lk(mutex_[bucket]);
    if (!HasPrePrepare(bucket, seq, hash)) {
      return false;
    }
    if (data_[bucket].find(hash) == data_[bucket].end()) {
      return false;
    }
  }

  {
    std::unique_lock<std::mutex> clk(commit_mutex_[bucket]);
    if (committed_[bucket].find(hash) != committed_[bucket].end() ||
        enqueued_[bucket].find(hash) != enqueued_[bucket].end()) {
      return false;
    }
    enqueued_[bucket].insert(hash);
  }

  auto commit_proposal = std::make_unique<Proposal>();
  commit_proposal->set_hash(hash);
  commit_proposal->set_seq(seq);
  commit_proposal->set_view(view_);
  commit_q_.Push(std::move(commit_proposal));
  commit_seq_++;
  return true;
}

bool ZZY::TrySlowCommit(int64_t seq, const std::string& hash) {
  const int bucket = seq % 1000;
  {
    std::unique_lock<std::mutex> clk(commit_mutex_[bucket]);
    if (committed_[bucket].find(hash) != committed_[bucket].end() ||
        enqueued_[bucket].find(hash) != enqueued_[bucket].end()) {
      return false;
    }
  }

  {
    std::unique_lock<std::mutex> lk(mutex_[bucket]);
    if (!HasPrePrepare(bucket, seq, hash)) {
      return false;
    }
    if (data_[bucket].find(hash) == data_[bucket].end()) {
      return false;
    }
  }

  {
    std::unique_lock<std::mutex> clk(commit_mutex_[bucket]);
    if (committed_[bucket].find(hash) != committed_[bucket].end() ||
        enqueued_[bucket].find(hash) != enqueued_[bucket].end()) {
      return false;
    }
    enqueued_[bucket].insert(hash);
  }

  auto commit_proposal = std::make_unique<Proposal>();
  commit_proposal->set_hash(hash);
  commit_proposal->set_seq(seq);
  commit_proposal->set_view(view_);
  commit_q_.Push(std::move(commit_proposal));
  commit_seq_++;
  return true;
}

bool ZZY::TryPendingCommit(int64_t seq, const std::string& hash) {
  const int bucket = seq % 1000;
  {
    std::unique_lock<std::mutex> lk(mutex_[bucket]);
    if (pending_commit_[bucket].find(hash) == pending_commit_[bucket].end()) {
      return false;
    }
  }

  if (!TrySlowCommit(seq, hash)) {
    return false;
  }

  std::unique_lock<std::mutex> lk(mutex_[bucket]);
  pending_commit_[bucket].erase(hash);
  return true;
}

void ZZY::SendPrepareToReplicas(int64_t seq, const std::string& hash,
                                int view) {
  Proposal prepare;
  prepare.set_hash(hash);
  prepare.set_seq(seq);
  prepare.set_proposer(id_);
  prepare.set_view(view);
  Broadcast(MessageType::Prepare, prepare);
}

void ZZY::SendOrderReplyToClient(const Transaction& txn) {
  const int64_t local_id = ParseLocalId(txn);
  Proposal reply;
  reply.set_hash(txn.hash());
  reply.set_seq(txn.seq());
  reply.set_proposer(id_);
  reply.set_view(txn.view() == 0 ? view_ : txn.view());
  reply.set_local_id(local_id);

  const std::string digest =
      BuildPrepareDigest(reply.view(), reply.seq(), reply.hash(), id_);
  const auto signature_or = verifier_->SignMessage(digest);
  if (!signature_or.ok()) {
    LOG(ERROR) << "sign order reply fail";
    return;
  }
  *reply.mutable_data_signature() = *signature_or;

  if (IsClient()) {
    ReceiveOrderReply(std::make_unique<Proposal>(reply));
    return;
  }
  SendMessage(MessageType::ACK, reply, ClientId());
}

void ZZY::SpeculativeExecute(const Transaction& txn) {
  if (!speculative_execute_func_) {
    return;
  }

  const int bucket = txn.seq() % 1000;
  {
    std::unique_lock<std::mutex> lk(mutex_[bucket]);
    if (!speculated_[bucket].insert(txn.hash()).second) {
      return;
    }
  }
  speculative_execute_func_(txn);
}

bool ZZY::VerifyCommitQC(const Proposal& proposal) {
  if (proposal.qc().qc_size() < 2 * f_ + 1) {
    LOG(ERROR) << "commit qc not enough:" << proposal.qc().qc_size();
    return false;
  }

  const int64_t seq = proposal.seq();
  const std::string& hash = proposal.hash();
  const int view = proposal.view() == 0 ? view_ : proposal.view();

  std::set<int32_t> signers;
  for (const auto& qc : proposal.qc().qc()) {
    if (qc.seq() != seq) {
      return false;
    }
    if (qc.view() != 0 && qc.view() != view) {
      return false;
    }
    if (!signers.insert(qc.replica()).second) {
      return false;
    }
    const std::string digest =
        BuildPrepareDigest(view, seq, hash, qc.replica());
    if (!verifier_->VerifyMessage(digest, qc.sign())) {
      return false;
    }
  }
  return true;
}

bool ZZY::TryClientFastComplete(int64_t local_id) {
  if (!IsClient()) {
    return false;
  }
  bool ready = false;
  {
    std::unique_lock<std::mutex> lk(client_mutex_);
    if (client_done_.find(local_id) != client_done_.end()) {
      return true;
    }
    if (static_cast<int>(client_reply_senders_[local_id].size()) >=
        total_num_) {
      ready = true;
    }
  }
  if (!ready) {
    return false;
  }
  // Fast path completes only after collecting 3f+1 replies. Broadcast the
  // resulting certificate so every replica can move from speculative execution
  // to final commit instead of leaving non-client replicas blocked at seq 1.
  SendClientCommit(local_id);
  return CompleteClientRequest(local_id);
}

bool ZZY::TryClientSlowComplete(int64_t local_id) {
  if (!IsClient()) {
    return false;
  }
  bool ready = false;
  {
    std::unique_lock<std::mutex> lk(client_mutex_);
    if (client_done_.find(local_id) != client_done_.end()) {
      return true;
    }
    if (static_cast<int>(client_commit_ack_senders_[local_id].size()) >=
        2 * f_ + 1) {
      ready = true;
    }
  }
  return ready ? CompleteClientRequest(local_id) : false;
}

bool ZZY::CompleteClientRequest(int64_t local_id) {
  {
    std::unique_lock<std::mutex> lk(client_mutex_);
    if (client_done_.find(local_id) != client_done_.end()) {
      return true;
    }
    client_done_.insert(local_id);
  }

  if (client_complete_func_) {
    client_complete_func_(local_id);
  }
  {
    std::unique_lock<std::mutex> lk(client_mutex_);
    client_reply_senders_.erase(local_id);
    client_replies_.erase(local_id);
    client_commit_ack_senders_.erase(local_id);
    client_start_time_.erase(local_id);
    client_commit_sent_.erase(local_id);
    client_pending_txn_.erase(local_id);
    client_last_retry_time_.erase(local_id);
  }
  return true;
}

bool ZZY::TryClientCommitLocal(int64_t local_id) {
  if (!IsClient()) {
    return false;
  }

  int64_t seq = 0;
  std::string hash;
  {
    std::unique_lock<std::mutex> lk(client_mutex_);
    auto it = client_replies_.find(local_id);
    if (it == client_replies_.end() || it->second.empty()) {
      return false;
    }
    seq = it->second.front().seq();
    hash = it->second.front().hash();
  }
  return TryFastCommit(seq, hash);
}

void ZZY::RebroadcastPendingTransaction(int64_t local_id) {
  if (!IsClient()) {
    return;
  }

  Transaction txn;
  {
    std::unique_lock<std::mutex> lk(client_mutex_);
    auto it = client_pending_txn_.find(local_id);
    if (it == client_pending_txn_.end()) {
      return;
    }
    txn = it->second;
  }

  LOG(ERROR) << "zzy rebroadcast pre-prepare, local_id:" << local_id
             << " seq:" << txn.seq();
  Broadcast(MessageType::Propose, txn);
  ReceivePropose(std::make_unique<Transaction>(txn));
}

void ZZY::SendClientCommit(int64_t local_id) {
  if (!IsClient()) {
    return;
  }

  Proposal commit;
  int view = view_;
  {
    std::unique_lock<std::mutex> lk(client_mutex_);
    if (client_done_.find(local_id) != client_done_.end()) {
      return;
    }
    auto& replies = client_replies_[local_id];
    if (static_cast<int>(replies.size()) < 2 * f_ + 1) {
      LOG(ERROR) << "not enough replies for commit certificate, local_id:"
                 << local_id;
      return;
    }
    client_commit_sent_.insert(local_id);

    commit.set_hash(replies.front().hash());
    commit.set_seq(replies.front().seq());
    commit.set_local_id(local_id);
    commit.set_proposer(id_);
    if (replies.front().view() != 0) {
      view = replies.front().view();
    }
    commit.set_view(view);

    for (int i = 0; i < 2 * f_ + 1; ++i) {
      auto* qc = commit.mutable_qc()->add_qc();
      qc->set_data_hash(BuildPrepareDigest(
          replies[i].view() == 0 ? view : replies[i].view(), replies[i].seq(),
          replies[i].hash(), replies[i].proposer()));
      *qc->mutable_sign() = replies[i].data_signature();
      qc->set_seq(replies[i].seq());
      qc->set_view(replies[i].view() == 0 ? view : replies[i].view());
      qc->set_replica(replies[i].proposer());
    }
  }

  LOG(ERROR) << "zzy broadcast commit certificate, local_id:" << local_id
             << " seq:" << commit.seq();
  ReceiveCommit(std::make_unique<Proposal>(commit));
  Broadcast(MessageType::Commit, commit);
}

bool ZZY::ReceiveOrderReply(std::unique_ptr<Proposal> reply) {
  if (!IsClient()) {
    return false;
  }
  if (!VerifyOrderReply(*reply)) {
    LOG(ERROR) << "invalid order reply from:" << reply->proposer();
    return false;
  }

  const int64_t local_id = reply->local_id();
  if (local_id <= 0) {
    return false;
  }

  {
    std::unique_lock<std::mutex> lk(client_mutex_);
    if (client_done_.find(local_id) != client_done_.end()) {
      return true;
    }
    if (!client_reply_senders_[local_id].insert(reply->proposer()).second) {
      return true;
    }
    client_replies_[local_id].push_back(*reply);
  }

  if (TryClientFastComplete(local_id)) {
    return true;
  }

  // Slow path is timeout-driven. If 2f+1 replies arrive but not all 3f+1,
  // AsyncClientCheck sends Commit after client_time_limit_.
  return true;
}

bool ZZY::ReceiveClientCommitACK(std::unique_ptr<Proposal> ack) {
  if (!IsClient()) {
    return false;
  }

  const int64_t local_id = ack->local_id();
  if (local_id <= 0) {
    return false;
  }

  {
    std::unique_lock<std::mutex> lk(client_mutex_);
    if (client_done_.find(local_id) != client_done_.end()) {
      return true;
    }
    client_commit_ack_senders_[local_id].insert(ack->proposer());
  }
  TryClientSlowComplete(local_id);
  return true;
}

bool ZZY::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  if (!IsPrimary()) {
    LOG(ERROR) << "reject order on non-primary replica:" << id_;
    return false;
  }

  const int water_mark = config_.GetConfigData().water_mark();
  if (water_mark > 0 && seq_ - commit_seq_ > water_mark) {
    return false;
  }

  const int64_t local_id = ParseLocalId(*txn);
  txn->set_local_id(local_id);
  if (txn->uid() == 0) {
    txn->set_uid(local_id);
  }
  txns_.Push(std::move(txn));
  return true;
}

bool ZZY::ReceiveProposeBatch(std::unique_ptr<TransactionBatch> batch) {
  bool ret = true;
  for (const auto& txn : batch->transactions()) {
    ret = ReceivePropose(std::make_unique<Transaction>(txn)) && ret;
  }
  return ret;
}

bool ZZY::ReceivePropose(std::unique_ptr<Transaction> txn) {
  if (!VerifyPrePrepare(*txn)) {
    return false;
  }

  const std::string hash = txn->hash();
  const int64_t seq = txn->seq();
  const int bucket = seq % 1000;

  Transaction order_txn;
  {
    std::unique_lock<std::mutex> lk(mutex_[bucket]);
    if (committed_[bucket].find(hash) != committed_[bucket].end()) {
      return true;
    }
    data_[bucket][hash] = std::move(txn);
    MarkPrePrepare(bucket, seq, hash);
    order_txn = *data_[bucket][hash];
  }

  SpeculativeExecute(order_txn);
  SendOrderReplyToClient(order_txn);
  TryPendingCommit(seq, hash);

  if (IsPrimary()) {
    return true;
  }

  SendPrepareToReplicas(seq, hash, view_);
  {
    std::unique_lock<std::mutex> lk(mutex_[bucket]);
    received_[bucket][seq].insert(id_);
  }
  return true;
}

bool ZZY::ReceivePrepare(std::unique_ptr<Proposal> proposal) {
  if (!VerifyPrepare(*proposal)) {
    return false;
  }

  const int64_t seq = proposal->seq();
  const std::string hash = proposal->hash();
  const int proposer = proposal->proposer();
  const int bucket = seq % 1000;

  {
    std::unique_lock<std::mutex> lk(mutex_[bucket]);
    if (committed_[bucket].find(hash) != committed_[bucket].end()) {
      return true;
    }
    if (!HasPrePrepare(bucket, seq, hash)) {
      LOG(ERROR) << "prepare before pre-prepare, seq:" << seq;
      return false;
    }
    received_[bucket][seq].insert(proposer);
  }
  return true;
}

bool ZZY::ReceiveCommit(std::unique_ptr<Proposal> proposal) {
  if (!VerifyCommitQC(*proposal)) {
    LOG(ERROR) << "commit verify fail";
    return false;
  }

  const int64_t seq = proposal->seq();
  const int64_t local_id = proposal->local_id();
  const std::string hash = proposal->hash();

  if (!TrySlowCommit(seq, hash)) {
    const int bucket = seq % 1000;
    bool already_final = false;
    {
      std::unique_lock<std::mutex> clk(commit_mutex_[bucket]);
      already_final = committed_[bucket].find(hash) != committed_[bucket].end() ||
                      enqueued_[bucket].find(hash) != enqueued_[bucket].end();
    }
    if (!already_final) {
      std::unique_lock<std::mutex> lk(mutex_[bucket]);
      pending_commit_[bucket][hash] = *proposal;
    }
  }

  Proposal ack;
  ack.set_hash(hash);
  ack.set_seq(seq);
  ack.set_proposer(id_);
  ack.set_local_id(local_id);
  ack.set_view(proposal->view() == 0 ? view_ : proposal->view());
  if (IsClient()) {
    ReceiveClientCommitACK(std::make_unique<Proposal>(ack));
    return true;
  }
  SendMessage(MessageType::CommitACK, ack, ClientId());
  return true;
}

}  // namespace zzy
}  // namespace resdb

#include "platform/consensus/ordering/3pc/commitment_3pc.h"

namespace resdb {

int Commitment3PC::ProcessNewRequest(std::unique_ptr<Context> context,
                                     std::unique_ptr<Request> user_request) {
  // Keep the existing PBFT ingress checks exactly the same.
  if (context == nullptr || context->signature.signature().empty()) {
    LOG(ERROR) << "user request doesn't contain signature, reject";
    return -2;
  }

  if (uint64_t seq = duplicate_manager_->CheckIfExecuted(user_request->hash())) {
    LOG(ERROR) << "This request is already executed with seq: " << seq;
    user_request->set_seq(seq);
    message_manager_->SendResponse(std::move(user_request));
    return -2;
  }

  // Under the assignment assumptions, the current primary is the single 3PC
  // coordinator. Non-coordinator replicas forward the request there.
  if (!IsCoordinator()) {
    LOG(INFO) << "NOT COORDINATOR, Coordinator is " << CoordinatorId();
    replica_communicator_->SendMessage(*user_request, CoordinatorId());
    {
      std::lock_guard<std::mutex> lk(rc_mutex_);
      request_complained_.push(
          std::make_pair(std::move(context), std::move(user_request)));
    }
    return -3;
  }

  // Verify the client payload signature before entering 3PC.
  bool valid = verifier_->VerifyMessage(user_request->data(),
                                        user_request->data_signature());
  if (!valid) {
    LOG(ERROR) << "request is not valid: "
               << user_request->data_signature().DebugString();
    LOG(ERROR) << "msg size: " << user_request->data().size();
    return -2;
  }

  if (pre_verify_func_ && !pre_verify_func_(*user_request)) {
    LOG(ERROR) << "check by the user func fail";
    return -2;
  }

  global_stats_->IncClientRequest();

  // Reuse the proposed-request duplicate suppression from PBFT.
  if (duplicate_manager_->CheckAndAddProposed(user_request->hash())) {
    LOG(INFO) << "request is already proposed, reject";
    return -2;
  }

  auto seq = message_manager_->AssignNextSeq();
  if (!seq.ok()) {
    LOG(ERROR) << "AssignNextSeq() failed";
    duplicate_manager_->EraseProposed(user_request->hash());
    global_stats_->SeqFail();

    Request response;
    response.set_type(Request::TYPE_RESPONSE);
    response.set_sender_id(config_.GetSelfInfo().id());
    response.set_proxy_id(user_request->proxy_id());
    response.set_ret(-2);
    response.set_hash(user_request->hash());
    replica_communicator_->SendMessage(response, response.proxy_id());
    return -2;
  }

  // Replace PBFT PRE_PREPARE with a 3PC PREPARE.
  // Preserve:
  //   - data()            original batched client payload
  //   - data_signature()  client signature
  //   - hash()            duplicate tracking key
  //   - proxy_id()        return path to proxy/client
  user_request->set_type(Request::TYPE_3PC_PREPARE);
  user_request->set_current_view(message_manager_->GetCurrentView());
  user_request->set_seq(*seq);
  user_request->set_sender_id(config_.GetSelfInfo().id());
  user_request->set_primary_id(config_.GetSelfInfo().id());

  // Coordinator-side 3PC bookkeeping.
  {
    std::lock_guard<std::mutex> lk(txn_mu_);
    ThreePCTxnState& txn = txn_state_[*seq];
    txn.seq = *seq;
    txn.phase = ThreePCPhase::kReady;  // PREPARE sent, waiting on votes
    txn.hash = user_request->hash();
    txn.coordinator_id = config_.GetSelfInfo().id();
    txn.proxy_id = user_request->proxy_id();
    txn.original_request = *user_request;

    // Coordinator votes commit for its own local participant.
    txn.vote_commit_from.insert(config_.GetSelfInfo().id());
  }

  global_stats_->RecordStateTime("3pc_prepare");

  replica_communicator_->BroadCast(*user_request);

  // This will likely not do anything but mirrors the PBFT implementation
  return MaybeBroadcastPreCommit(*seq);
}

int Commitment3PC::ProcessPrepareMsg(std::unique_ptr<Context> context,
                                     std::unique_ptr<Request> request) {
  if (global_stats_->IsFaulty() || context == nullptr ||
      context->signature.signature().empty()) {
    LOG(ERROR) << "3PC PREPARE missing valid signed context";
    return -2;
  }

  if (request->sender_id() != CoordinatorId()) {
    LOG(ERROR) << "3PC PREPARE not from coordinator. sender:"
               << request->sender_id()
               << " coordinator:" << CoordinatorId();
    return -2;
  }

  if (uint64_t seq = duplicate_manager_->CheckIfExecuted(request->hash())) {
    LOG(INFO) << "3PC PREPARE already executed at seq:" << seq;
    return 0;
  }

  // Verify the original client payload carried by the PREPARE.
  bool valid = verifier_->VerifyMessage(request->data(),
                                        request->data_signature());
  if (!valid) {
    LOG(ERROR) << "3PC PREPARE client payload signature invalid";
    return -2;
  }

  if (pre_verify_func_ && !pre_verify_func_(*request)) {
    LOG(ERROR) << "3PC PREPARE rejected by user pre_verify_func";
    return -2;
  }

  if (duplicate_manager_->CheckAndAddProposed(request->hash())) {
    LOG(INFO) << "3PC PREPARE duplicate proposed hash, ignore";
    return 0;
  }

  {
    std::lock_guard<std::mutex> lk(txn_mu_);
    ThreePCTxnState& txn = txn_state_[request->seq()];
    if (txn.phase == ThreePCPhase::kCommit) {
      return 0;
    }

    txn.seq = request->seq();
    txn.phase = ThreePCPhase::kReady;
    txn.hash = request->hash();
    txn.coordinator_id = request->sender_id();
    txn.proxy_id = request->proxy_id();
    txn.original_request = *request;
  }

  global_stats_->RecordStateTime("3pc_prepare_recv");

  Request vote;
  vote.set_type(Request::TYPE_3PC_VOTE_COMMIT);
  vote.set_seq(request->seq());
  vote.set_hash(request->hash());
  vote.set_proxy_id(request->proxy_id());
  vote.set_current_view(message_manager_->GetCurrentView());
  vote.set_sender_id(config_.GetSelfInfo().id());
  vote.set_primary_id(CoordinatorId());

  replica_communicator_->SendMessage(vote, CoordinatorId());
  return 0;
}

int Commitment3PC::ProcessVoteCommitMsg(std::unique_ptr<Context> context,
                                        std::unique_ptr<Request> request) {
  if (context == nullptr || context->signature.signature().empty()) {
    LOG(ERROR) << "3PC VOTE_COMMIT missing valid signed context";
    return -2;
  }

  if (!IsCoordinator()) {
    LOG(ERROR) << "only coordinator handles 3PC VOTE_COMMIT";
    return -2;
  }

  if (request == nullptr) {
    LOG(ERROR) << "3PC VOTE_COMMIT missing request";
    return -2;
  }

  const uint64_t seq = request->seq();
  const uint32_t voter_id = request->sender_id();

  {
    std::lock_guard<std::mutex> lk(txn_mu_);
    auto it = txn_state_.find(seq);
    if (it == txn_state_.end()) {
      LOG(ERROR) << "unknown 3PC txn seq: " << seq;
      return -2;
    }

    ThreePCTxnState& txn = it->second;
    if (txn.phase != ThreePCPhase::kReady) {
      // Late/duplicate vote or already advanced.
      return 0;
    }

    if (request->hash() != txn.hash) {
      LOG(ERROR) << "3PC VOTE_COMMIT hash mismatch for seq: " << seq;
      return -2;
    }

    if (voter_id == config_.GetSelfInfo().id()) {
      LOG(ERROR) << "coordinator self vote should be implicit for seq: " << seq;
      return -2;
    }

    txn.vote_commit_from.insert(voter_id);
  }

  return MaybeBroadcastPreCommit(seq);
}

int Commitment3PC::ProcessVoteAbortMsg(std::unique_ptr<Context> context,
                                       std::unique_ptr<Request> request) {
  if (context == nullptr || context->signature.signature().empty()) {
    LOG(ERROR) << "3PC VOTE_ABORT missing valid signed context";
    return -2;
  }

  if (!IsCoordinator()) {
    LOG(ERROR) << "only coordinator handles 3PC VOTE_ABORT";
    return -2;
  }

  std::string hash;
  {
    std::lock_guard<std::mutex> lk(txn_mu_);
    auto it = txn_state_.find(request->seq());
    if (it == txn_state_.end()) {
      return -2;
    }
    if (it->second.phase == ThreePCPhase::kAbort ||
        it->second.phase == ThreePCPhase::kCommit) {
      return 0;
    }

    it->second.phase = ThreePCPhase::kAbort;
    hash = it->second.hash;
  }

  duplicate_manager_->EraseProposed(hash);
  global_stats_->RecordStateTime("3pc_abort");

  Request abort;
  abort.set_type(Request::TYPE_3PC_GLOBAL_ABORT);
  abort.set_seq(request->seq());
  abort.set_hash(hash);
  abort.set_proxy_id(request->proxy_id());
  abort.set_current_view(message_manager_->GetCurrentView());
  abort.set_sender_id(config_.GetSelfInfo().id());
  abort.set_primary_id(config_.GetSelfInfo().id());

  replica_communicator_->BroadCast(abort);
  return 0;
}

int Commitment3PC::ProcessPreCommitMsg(std::unique_ptr<Context> context,
                                       std::unique_ptr<Request> request) {
  if (context == nullptr || context->signature.signature().empty()) {
    LOG(ERROR) << "3PC PRECOMMIT missing valid signed context";
    return -2;
  }

  if (request->sender_id() != CoordinatorId()) {
    LOG(ERROR) << "3PC PRECOMMIT not from coordinator. sender:"
               << request->sender_id()
               << " coordinator:" << CoordinatorId();
    return -2;
  }

  {
    std::lock_guard<std::mutex> lk(txn_mu_);
    auto it = txn_state_.find(request->seq());
    if (it == txn_state_.end()) {
      LOG(ERROR) << "3PC PRECOMMIT for unknown seq:" << request->seq();
      return -2;
    }

    ThreePCTxnState& txn = it->second;
    if (txn.phase == ThreePCPhase::kCommit) {
      return 0;
    }
    if (txn.phase == ThreePCPhase::kAbort) {
      return -2;
    }

    txn.phase = ThreePCPhase::kPreCommit;
  }

  global_stats_->RecordStateTime("3pc_precommit_recv");

  Request ack;
  ack.set_type(Request::TYPE_3PC_PRECOMMIT_ACK);
  ack.set_seq(request->seq());
  ack.set_hash(request->hash());
  ack.set_proxy_id(request->proxy_id());
  ack.set_current_view(message_manager_->GetCurrentView());
  ack.set_sender_id(config_.GetSelfInfo().id());
  ack.set_primary_id(CoordinatorId());

  replica_communicator_->SendMessage(ack, CoordinatorId());
  return 0;
}

int Commitment3PC::ProcessPreCommitAckMsg(std::unique_ptr<Context> context,
                                          std::unique_ptr<Request> request) {
  if (context == nullptr || context->signature.signature().empty()) {
    LOG(ERROR) << "3PC PRECOMMIT_ACK missing valid signed context";
    return -2;
  }

  if (!IsCoordinator()) {
    LOG(ERROR) << "only coordinator handles 3PC PRECOMMIT_ACK";
    return -2;
  }

  {
    std::lock_guard<std::mutex> lk(txn_mu_);
    auto it = txn_state_.find(request->seq());
    if (it == txn_state_.end()) {
      return -2;
    }

    ThreePCTxnState& txn = it->second;
    if (txn.phase != ThreePCPhase::kPreCommit) {
      return 0;
    }

    txn.precommit_ack_from.insert(request->sender_id());
  }

  return MaybeBroadcastGlobalCommit(request->seq());
}

int Commitment3PC::ProcessGlobalCommitMsg(std::unique_ptr<Context> context,
                                          std::unique_ptr<Request> request) {
  if (context == nullptr || context->signature.signature().empty()) {
    LOG(ERROR) << "3PC GLOBAL_COMMIT missing valid signed context";
    return -2;
  }

  Request exec_request;
  {
    std::lock_guard<std::mutex> lk(txn_mu_);
    auto it = txn_state_.find(request->seq());
    if (it == txn_state_.end()) {
      LOG(ERROR) << "3PC GLOBAL_COMMIT for unknown seq:" << request->seq();
      return -2;
    }

    ThreePCTxnState& txn = it->second;
    if (txn.phase == ThreePCPhase::kCommit) {
      return 0;
    }
    if (txn.phase == ThreePCPhase::kAbort) {
      return -2;
    }

    txn.phase = ThreePCPhase::kCommit;
    exec_request = txn.original_request;
  }

  global_stats_->RecordStateTime("3pc_global_commit_recv");
  duplicate_manager_->EraseProposed(exec_request.hash());

  return ExecuteCommittedTxn(exec_request);
}

int Commitment3PC::ProcessGlobalAbortMsg(std::unique_ptr<Context> context,
                                         std::unique_ptr<Request> request) {
  if (context == nullptr || context->signature.signature().empty()) {
    LOG(ERROR) << "3PC GLOBAL_ABORT missing valid signed context";
    return -2;
  }

  {
    std::lock_guard<std::mutex> lk(txn_mu_);
    auto it = txn_state_.find(request->seq());
    if (it == txn_state_.end()) {
      return -2;
    }
    it->second.phase = ThreePCPhase::kAbort;
    duplicate_manager_->EraseProposed(it->second.hash);
  }

  global_stats_->RecordStateTime("3pc_global_abort_recv");
  return 0;
}

int Commitment3PC::MaybeBroadcastPreCommit(uint64_t seq) {
  std::unique_ptr<Request> precommit;

  {
    std::lock_guard<std::mutex> lk(txn_mu_);
    auto it = txn_state_.find(seq);
    if (it == txn_state_.end()) {
      return -2;
    }

    ThreePCTxnState& txn = it->second;
    if (txn.phase != ThreePCPhase::kReady) {
      return 0;
    }

    // Assignment assumption: all 4 replicas participate.
    // That means the coordinator waits for a commit vote from every replica,
    // including its own implicit local vote.
    const size_t expected_votes = static_cast<size_t>(config_.GetReplicaNum());
    if (txn.vote_commit_from.size() < expected_votes) {
      return 0;
    }

    txn.phase = ThreePCPhase::kPreCommit;
    txn.precommit_ack_from.insert(config_.GetSelfInfo().id());

    precommit = std::make_unique<Request>();
    precommit->set_type(Request::TYPE_3PC_PRECOMMIT);
    precommit->set_seq(txn.seq);
    precommit->set_hash(txn.hash);
    precommit->set_proxy_id(txn.proxy_id);
    precommit->set_sender_id(config_.GetSelfInfo().id());
    precommit->set_primary_id(config_.GetSelfInfo().id());
    precommit->set_current_view(message_manager_->GetCurrentView());
  }

  global_stats_->RecordStateTime("3pc_precommit");
  replica_communicator_->BroadCast(*precommit);
  return 0;
}

int Commitment3PC::MaybeBroadcastGlobalCommit(uint64_t seq) {
  std::unique_ptr<Request> global_commit;
  Request exec_request;

  {
    std::lock_guard<std::mutex> lk(txn_mu_);
    auto it = txn_state_.find(seq);
    if (it == txn_state_.end()) {
      return -2;
    }

    ThreePCTxnState& txn = it->second;
    if (txn.phase != ThreePCPhase::kPreCommit) {
      return 0;
    }

    const size_t expected_acks = static_cast<size_t>(config_.GetReplicaNum());
    if (txn.precommit_ack_from.size() < expected_acks) {
      return 0;
    }

    txn.phase = ThreePCPhase::kCommit;

    global_commit = std::make_unique<Request>();
    global_commit->set_type(Request::TYPE_3PC_GLOBAL_COMMIT);
    global_commit->set_seq(txn.seq);
    global_commit->set_hash(txn.hash);
    global_commit->set_proxy_id(txn.proxy_id);
    global_commit->set_current_view(message_manager_->GetCurrentView());
    global_commit->set_sender_id(config_.GetSelfInfo().id());
    global_commit->set_primary_id(config_.GetSelfInfo().id());

    exec_request = txn.original_request;
  }

  global_stats_->RecordStateTime("3pc_global_commit");
  replica_communicator_->BroadCast(*global_commit);

  // Coordinator also executes once the commit decision is final.
  duplicate_manager_->EraseProposed(exec_request.hash());
  return ExecuteCommittedTxn(exec_request);
}

int Commitment3PC::ExecuteCommittedTxn(const Request& committed_request) {
  // MessageManager extension:
  //   int ExecuteOrderedRequest(std::unique_ptr<Request> request);
  //
  // Set metadata of request and send directly to the execution/reply path.
  auto exec_request = std::make_unique<Request>(committed_request);
  exec_request->set_seq(committed_request.seq());
  exec_request->set_current_view(message_manager_->GetCurrentView());
  exec_request->set_primary_id(CoordinatorId());

  return message_manager_->ExecuteOrderedRequest(std::move(exec_request));
}

}  // namespace resdb

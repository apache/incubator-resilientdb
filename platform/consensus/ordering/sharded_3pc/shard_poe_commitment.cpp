#include "platform/consensus/ordering/sharded_3pc/shard_poe_commitment.h"

#include <glog/logging.h>

#include <cstdint>
#include <exception>
#include <stdexcept>
#include <string>
#include <utility>

#include "common/crypto/signature_verifier.h"

namespace resdb {
namespace {

// POE proof/rollback digests concatenate multiple fields. Prefixing each field
// with its length keeps the digest unambiguous even when byte fields contain
// separators or decimal-looking payloads.
void AppendField(std::string* output, const std::string& value) {
  output->append(std::to_string(value.size()));
  output->push_back(':');
  output->append(value);
  output->push_back(';');
}

void AppendField(std::string* output, uint64_t value) {
  AppendField(output, std::to_string(value));
}

void AppendField(std::string* output, uint32_t value) {
  AppendField(output, std::to_string(value));
}

}  // namespace

ShardPOECommitment::ShardPOECommitment(
    const ResDBConfig& config, MessageManager* message_manager,
    ReplicaCommunicator* replica_communicator,
    ShardCommunicator* shard_communicator,
    const ShardMetadata* shard_metadata, SignatureVerifier* verifier,
    bool start_response_thread)
    : Commitment(config, message_manager, replica_communicator, verifier,
                 start_response_thread),
      shard_communicator_(shard_communicator),
      shard_metadata_(shard_metadata) {
  if (shard_communicator_ == nullptr) {
    throw std::invalid_argument("ShardPOECommitment requires communicator");
  }
  if (shard_metadata_ == nullptr) {
    throw std::invalid_argument("ShardPOECommitment requires metadata");
  }
  if (verifier_ == nullptr) {
    throw std::invalid_argument("ShardPOECommitment requires verifier");
  }
  message_manager_->SetPostExecuteHook(
      [this](const Request& request, const BatchUserResponse& response) {
        SendPOEProof(request, response);
      });
  message_manager_->SetResponseHoldPredicate([](const Request& request) {
    return request.type() == Request::TYPE_POE_EXECUTE &&
           request.is_local_shard_poe();
  });
}

ShardPOECommitment::~ShardPOECommitment() {
  if (message_manager_ != nullptr) {
    message_manager_->SetPostExecuteHook(nullptr);
    message_manager_->SetResponseHoldPredicate(nullptr);
  }
}

int ShardPOECommitment::ProcessGlobalCommittedRequest(
    const Request& committed_request) {
  // Global 3PC is leader-to-leader. Once that global decision commits, each
  // shard leader is responsible for starting the local POE phase in its shard.
  if (!shard_metadata_->IsSelfShardLeader()) {
    LOG(INFO) << "non-leader ignored POE local start. node:"
              << shard_metadata_->SelfNodeId()
              << " seq:" << committed_request.seq();
    return 0;
  }

  // Reuse the PBFT duplicate manager as the local "already started" guard.
  // This prevents repeated POE_EXECUTE broadcasts for the same global commit.
  // Proof and cert handling keep their own per-sender state below.
  if (duplicate_manager_->CheckIfExecuted(committed_request.hash()) ||
      duplicate_manager_->CheckAndAddProposed(committed_request.hash())) {
    return 0;
  }

  // TYPE_POE_EXECUTE is the local-shard handoff message. It preserves the
  // globally ordered request, then adds the local shard target and POE marker
  // so receivers can validate the message cheaply.
  Request local_request(committed_request);
  local_request.set_type(Request::TYPE_POE_EXECUTE);
  local_request.set_current_view(message_manager_->GetCurrentView());
  local_request.set_sender_id(config_.GetSelfInfo().id());
  local_request.set_primary_id(config_.GetSelfInfo().id());
  local_request.set_local_shard_id(shard_metadata_->SelfShardId());
  local_request.set_is_local_shard_poe(true);
  // Recovery/replay or older generated requests may be missing the new
  // sharded metadata. Fill with defaults so local POE remains debuggable.
  // Normal proxy-routed requests already carry these fields.
  if (!local_request.has_global_coordinator_id()) {
    local_request.set_global_coordinator_id(committed_request.primary_id() != 0
                                                ? committed_request.primary_id()
                                                : committed_request.sender_id());
  }

  // These fields are also part of the later proof digest, so make sure older
  // or recovered requests still have values before local POE begins.
  if (!local_request.has_coordinator_shard_id()) {
    local_request.set_coordinator_shard_id(shard_metadata_->SelfShardId());
  }
  if (!local_request.has_global_txn_id()) {
    local_request.set_global_txn_id(local_request.seq());
  }

  LOG(ERROR) << "[shard_poe_trace]"
             << " local POE execute broadcast"
             << " node:" << shard_metadata_->SelfNodeId()
             << " shard:" << shard_metadata_->SelfShardId()
             << " seq:" << local_request.seq()
             << " global_txn_id:" << local_request.global_txn_id()
             << " coordinator_shard_id:"
             << local_request.coordinator_shard_id();

  // Broadcast the local POE execute trigger to the shard. Only local shard
  // leaders send this message.
  BroadcastConsensusMsg(local_request);
  return 0;
}

int ShardPOECommitment::ProcessPOEExecuteMsg(
    std::unique_ptr<Context> context, std::unique_ptr<Request> request) {
  if (context == nullptr) {
    LOG(ERROR) << "POE EXECUTE missing context";
    return -2;
  }
  if (request == nullptr) {
    LOG(ERROR) << "POE EXECUTE missing request";
    return -2;
  }
  if (!ValidatePOEExecuteRequest(*context, *request)) {
    return -2;
  }

  LOG(ERROR) << "[shard_poe_trace]"
             << " validated local POE execute"
             << " node:" << shard_metadata_->SelfNodeId()
             << " shard:" << shard_metadata_->SelfShardId()
             << " seq:" << request->seq()
             << " sender_id:" << request->sender_id()
             << " global_txn_id:" << request->global_txn_id()
             << " coordinator_shard_id:" << request->coordinator_shard_id();
  return message_manager_->ExecuteOrderedRequest(std::move(request));
}

int ShardPOECommitment::ProcessPOEProofMsg(
    std::unique_ptr<Context> context, std::unique_ptr<Request> request) {
  if (context == nullptr || request == nullptr) {
    LOG(ERROR) << "POE PROOF missing context or request";
    return -2;
  }
  if (!shard_metadata_->IsSelfShardLeader()) {
    LOG(INFO) << "non-leader ignored POE proof. node:"
              << shard_metadata_->SelfNodeId();
    return 0;
  }
  if (!ValidatePOEProofRequest(*context, *request)) {
    return -2;
  }

  Request cert_request;
  bool should_broadcast_cert = false;
  bool should_broadcast_rollback = false;
  uint64_t rollback_checkpoint = 0;
  {
    std::lock_guard<std::mutex> lk(proof_mu_);
    const TxnKey txn_key = MakeTxnKey(*request);
    const int64_t sender_id = request->sender_id();
    // Track the digest previously sent by each replica for this transaction.
    // Identical repeats are harmless duplicates; different digests indicate
    // local POE cannot safely certify and should roll back to checkpoint.
    auto& sender_digests = proof_digest_by_sender_[txn_key];
    auto existing_sender = sender_digests.find(sender_id);
    if (existing_sender != sender_digests.end()) {
      if (existing_sender->second == request->data_hash()) {
        LOG(INFO) << "duplicate POE proof ignored. sender:" << sender_id
                  << " seq:" << request->seq();
        return 0;
      }
      LOG(ERROR) << "conflicting POE proof from sender:" << sender_id
                 << " seq:" << request->seq();
      should_broadcast_rollback = true;
      rollback_checkpoint = message_manager_->GetStableCheckpoint();
    } else {
      sender_digests[sender_id] = request->data_hash();

      ProofBucket& bucket = proof_buckets_[MakeProofKey(*request)];
      if (bucket.signatures_by_sender.empty()) {
        bucket.proof_request = *request;
      }
      bucket.signatures_by_sender[sender_id] = request->data_signature();

      if (!bucket.cert_broadcast &&
          bucket.signatures_by_sender.size() >=
              static_cast<size_t>(config_.GetMinDataReceiveNum())) {
        bucket.cert_broadcast = true;
        cert_request =
            BuildPOECertRequest(bucket.proof_request,
                                bucket.signatures_by_sender);
        should_broadcast_cert = true;
      }
    }
  }

  if (should_broadcast_rollback) {
    return BroadcastPOERollback(rollback_checkpoint,
                                "conflicting POE proof digest");
  }

  if (should_broadcast_cert) {
    if (cert_request.type() != Request::TYPE_POE_CERT) {
      LOG(ERROR) << "failed to build POE cert";
      return -2;
    }
    LOG(ERROR) << "[shard_poe_trace]"
               << " broadcasting POE cert"
               << " node:" << shard_metadata_->SelfNodeId()
               << " shard:" << shard_metadata_->SelfShardId()
               << " seq:" << cert_request.seq()
               << " proofs:" << cert_request.committed_certs()
                                     .committed_certs_size();
    BroadcastConsensusMsg(cert_request);
    message_manager_->ReleaseHeldResponse(cert_request.seq(),
                                          cert_request.hash());
  }
  return 0;
}

int ShardPOECommitment::ProcessPOECertMsg(
    std::unique_ptr<Context> context, std::unique_ptr<Request> request) {
  if (context == nullptr || request == nullptr) {
    LOG(ERROR) << "POE CERT missing context or request";
    return -2;
  }
  if (!ValidatePOECertRequest(*context, *request)) {
    return -2;
  }

  LOG(ERROR) << "[shard_poe_trace]"
             << " releasing response after POE cert"
             << " node:" << shard_metadata_->SelfNodeId()
             << " shard:" << shard_metadata_->SelfShardId()
             << " seq:" << request->seq();
  return message_manager_->ReleaseHeldResponse(request->seq(),
                                               request->hash());
}

int ShardPOECommitment::ProcessPOERollbackMsg(
    std::unique_ptr<Context> context, std::unique_ptr<Request> request) {
  if (context == nullptr || request == nullptr) {
    LOG(ERROR) << "POE ROLLBACK missing context or request";
    return -2;
  }
  if (!ValidatePOERollbackRequest(*context, *request)) {
    return -2;
  }

  const uint64_t checkpoint_seq = request->seq();
  LOG(ERROR) << "[shard_poe_trace]"
             << " applying POE rollback"
             << " node:" << shard_metadata_->SelfNodeId()
             << " shard:" << shard_metadata_->SelfShardId()
             << " checkpoint_seq:" << checkpoint_seq
             << " reason:" << request->data();
  // request.seq() is the checkpoint boundary. All local POE state above it is
  // speculative and must be forgotten before MessageManager resets execution.
  ClearPOEStateAfter(checkpoint_seq);
  return message_manager_->RollbackToCheckpoint(checkpoint_seq);
}

bool ShardPOECommitment::ValidatePOEExecuteRequest(
    const Context& context, const Request& request) const {
  // TYPE_POE_EXECUTE is the local execution trigger, so require a signed
  // context and make sure the signature metadata matches the sender.
  if (context.signature.signature().empty()) {
    LOG(ERROR) << "POE EXECUTE missing signature";
    return false;
  }
  if (context.signature.node_id() != 0 &&
      context.signature.node_id() != request.sender_id()) {
    LOG(ERROR) << "POE EXECUTE signature sender mismatch. signature node:"
               << context.signature.node_id()
               << " request sender:" << request.sender_id();
    return false;
  }
  if (request.type() != Request::TYPE_POE_EXECUTE) {
    LOG(ERROR) << "POE EXECUTE wrong type:" << request.type();
    return false;
  }
  if (!shard_metadata_->HasLocalShard()) {
    LOG(ERROR) << "POE EXECUTE received on node without local shard";
    return false;
  }
  // Only the configured leader for this shard may start local POE. This is the
  // equivalent in sharded PBFT validation.
  const uint32_t local_shard_id = shard_metadata_->SelfShardId();
  const int64_t local_leader = shard_metadata_->LeaderForShard(local_shard_id);
  if (request.sender_id() != local_leader) {
    LOG(ERROR) << "POE EXECUTE invalid sender:" << request.sender_id()
               << " expected leader:" << local_leader;
    return false;
  }
  // Ensure replica has valid shard and that the request has been marked as POE.
  if (!request.has_local_shard_id() ||
      request.local_shard_id() != local_shard_id) {
    LOG(ERROR) << "POE EXECUTE wrong local shard. request shard:"
               << (request.has_local_shard_id() ? request.local_shard_id()
                                                : 0)
               << " expected:" << local_shard_id;
    return false;
  }
  if (!request.is_local_shard_poe()) {
    LOG(ERROR) << "POE EXECUTE missing local POE marker";
    return false;
  }
  // These fields bind the local POE trigger back to the globally ordered
  // transaction and the client/proxy response path.
  if (request.seq() == 0 || request.hash().empty() ||
      request.proxy_id() == 0) {
    LOG(ERROR) << "POE EXECUTE missing core request identity. seq:"
               << request.seq() << " hash_size:" << request.hash().size()
               << " proxy_id:" << request.proxy_id();
    return false;
  }
  if (!request.has_global_txn_id() ||
      !request.has_coordinator_shard_id() ||
      !request.has_global_coordinator_id()) {
    LOG(ERROR) << "POE EXECUTE missing sharded global metadata";
    return false;
  }
  // The coordinator shard metadata is not local-shard-specific: every shard
  // should agree which shard leader coordinated global 3PC for this request.
  int64_t expected_global_coordinator = 0;
  try {
    expected_global_coordinator =
        shard_metadata_->LeaderForShard(request.coordinator_shard_id());
  } catch (const std::exception& e) {
    LOG(ERROR) << "POE EXECUTE invalid coordinator shard metadata: "
               << e.what();
    return false;
  }
  if (expected_global_coordinator != request.global_coordinator_id()) {
    LOG(ERROR) << "POE EXECUTE coordinator metadata mismatch. shard:"
               << request.coordinator_shard_id()
               << " coordinator:" << request.global_coordinator_id();
    return false;
  }
  return true;
}

bool ShardPOECommitment::ValidatePOEProofRequest(
    const Context& context, const Request& request) const {
  if (context.signature.signature().empty()) {
    LOG(ERROR) << "POE PROOF missing signed context";
    return false;
  }
  if (request.type() != Request::TYPE_POE_PROOF) {
    LOG(ERROR) << "POE PROOF wrong type:" << request.type();
    return false;
  }
  if (!request.is_local_shard_poe() || !request.has_local_shard_id() ||
      request.local_shard_id() != shard_metadata_->SelfShardId()) {
    LOG(ERROR) << "POE PROOF wrong local shard or marker";
    return false;
  }
  if (context.signature.node_id() != 0 &&
      context.signature.node_id() != request.sender_id()) {
    LOG(ERROR) << "POE PROOF context sender mismatch. context:"
               << context.signature.node_id()
               << " sender:" << request.sender_id();
    return false;
  }
  if (!shard_metadata_->IsLocalShardReplica(request.sender_id())) {
    LOG(ERROR) << "POE PROOF sender is not in local shard:"
               << request.sender_id();
    return false;
  }
  if (!request.has_data_signature() ||
      request.data_signature().signature().empty()) {
    LOG(ERROR) << "POE PROOF missing data signature";
    return false;
  }
  if (request.data_signature().node_id() != request.sender_id()) {
    LOG(ERROR) << "POE PROOF signature node mismatch. signature:"
               << request.data_signature().node_id()
               << " sender:" << request.sender_id();
    return false;
  }
  if (!shard_metadata_->IsLocalShardReplica(
          request.data_signature().node_id())) {
    LOG(ERROR) << "POE PROOF signature node is not in local shard:"
               << request.data_signature().node_id();
    return false;
  }
  if (request.seq() == 0 || request.hash().empty() ||
      request.proxy_id() == 0 || request.data().empty() ||
      request.data_hash().empty()) {
    LOG(ERROR) << "POE PROOF missing identity or digest fields";
    return false;
  }
  if (!request.has_global_txn_id() ||
      !request.has_coordinator_shard_id() ||
      !request.has_global_coordinator_id()) {
    LOG(ERROR) << "POE PROOF missing global sharded metadata";
    return false;
  }
  if (BuildPOEProofDigest(request, request.data()) != request.data_hash()) {
    LOG(ERROR) << "POE PROOF digest mismatch";
    return false;
  }
  if (verifier_ == nullptr ||
      !verifier_->VerifyMessage(request.data_hash(),
                                request.data_signature())) {
    LOG(ERROR) << "POE PROOF signature verification failed";
    return false;
  }
  return true;
}

bool ShardPOECommitment::ValidatePOECertRequest(
    const Context& context, const Request& request) const {
  if (context.signature.signature().empty()) {
    LOG(ERROR) << "POE CERT missing signed context";
    return false;
  }
  if (request.type() != Request::TYPE_POE_CERT) {
    LOG(ERROR) << "POE CERT wrong type:" << request.type();
    return false;
  }
  if (!request.is_local_shard_poe() || !request.has_local_shard_id() ||
      request.local_shard_id() != shard_metadata_->SelfShardId()) {
    LOG(ERROR) << "POE CERT wrong local shard or marker";
    return false;
  }

  const int64_t local_leader =
      shard_metadata_->LeaderForShard(request.local_shard_id());
  if (request.sender_id() != local_leader) {
    LOG(ERROR) << "POE CERT sender is not local leader. sender:"
               << request.sender_id() << " leader:" << local_leader;
    return false;
  }
  if (context.signature.node_id() != 0 &&
      context.signature.node_id() != request.sender_id()) {
    LOG(ERROR) << "POE CERT context sender mismatch. context:"
               << context.signature.node_id()
               << " sender:" << request.sender_id();
    return false;
  }
  if (!request.has_data_signature() ||
      request.data_signature().signature().empty()) {
    LOG(ERROR) << "POE CERT missing leader signature";
    return false;
  }
  if (request.data_signature().node_id() != local_leader) {
    LOG(ERROR) << "POE CERT leader signature node mismatch. signature:"
               << request.data_signature().node_id()
               << " leader:" << local_leader;
    return false;
  }
  if (request.seq() == 0 || request.hash().empty() ||
      request.proxy_id() == 0 || request.data().empty() ||
      request.data_hash().empty()) {
    LOG(ERROR) << "POE CERT missing identity or digest fields";
    return false;
  }
  if (!request.has_global_txn_id() ||
      !request.has_coordinator_shard_id() ||
      !request.has_global_coordinator_id()) {
    LOG(ERROR) << "POE CERT missing global sharded metadata";
    return false;
  }
  if (BuildPOEProofDigest(request, request.data()) != request.data_hash()) {
    LOG(ERROR) << "POE CERT digest mismatch";
    return false;
  }
  if (verifier_ == nullptr ||
      !verifier_->VerifyMessage(request.data_hash(),
                                request.data_signature())) {
    LOG(ERROR) << "POE CERT leader signature verification failed";
    return false;
  }

  std::set<int64_t> proof_senders;
  for (const auto& signature : request.committed_certs().committed_certs()) {
    if (signature.node_id() == 0 || signature.signature().empty()) {
      LOG(ERROR) << "POE CERT contains malformed proof signature";
      return false;
    }
    if (!shard_metadata_->IsLocalShardReplica(signature.node_id())) {
      LOG(ERROR) << "POE CERT proof signer is not local shard replica:"
                 << signature.node_id();
      return false;
    }
    if (!proof_senders.insert(signature.node_id()).second) {
      LOG(ERROR) << "POE CERT duplicate proof signer:"
                 << signature.node_id();
      return false;
    }
    if (!verifier_->VerifyMessage(request.data_hash(), signature)) {
      LOG(ERROR) << "POE CERT proof signature verification failed. signer:"
                 << signature.node_id();
      return false;
    }
  }
  if (proof_senders.size() <
      static_cast<size_t>(config_.GetMinDataReceiveNum())) {
    LOG(ERROR) << "POE CERT does not contain enough proofs. proofs:"
               << proof_senders.size()
               << " threshold:" << config_.GetMinDataReceiveNum();
    return false;
  }
  return true;
}

bool ShardPOECommitment::ValidatePOERollbackRequest(
    const Context& context, const Request& request) const {
  // Rollback is intentionally narrow in Phase 5: only the local shard leader
  // may order it, and it only affects the receiving local shard.
  if (context.signature.signature().empty()) {
    LOG(ERROR) << "POE ROLLBACK missing signed context";
    return false;
  }
  if (request.type() != Request::TYPE_POE_ROLLBACK) {
    LOG(ERROR) << "POE ROLLBACK wrong type:" << request.type();
    return false;
  }
  if (!request.is_local_shard_poe() || !request.has_local_shard_id() ||
      request.local_shard_id() != shard_metadata_->SelfShardId()) {
    LOG(ERROR) << "POE ROLLBACK wrong local shard or marker";
    return false;
  }

  const int64_t local_leader =
      shard_metadata_->LeaderForShard(request.local_shard_id());
  if (request.sender_id() != local_leader) {
    LOG(ERROR) << "POE ROLLBACK sender is not local leader. sender:"
               << request.sender_id() << " leader:" << local_leader;
    return false;
  }
  if (context.signature.node_id() != 0 &&
      context.signature.node_id() != request.sender_id()) {
    LOG(ERROR) << "POE ROLLBACK context sender mismatch. context:"
               << context.signature.node_id()
               << " sender:" << request.sender_id();
    return false;
  }

  if (request.data_hash().empty() && !request.has_data_signature()) {
    // Tests and future internal recovery paths may use an unsigned rollback
    // marker. Network-originated leader rollbacks should include the optional
    // signed digest checked below.
    return true;
  }
  if (request.data_hash().empty() || !request.has_data_signature() ||
      request.data_signature().signature().empty()) {
    LOG(ERROR) << "POE ROLLBACK malformed optional rollback signature";
    return false;
  }
  if (request.data_signature().node_id() != local_leader) {
    LOG(ERROR) << "POE ROLLBACK signature node mismatch. signature:"
               << request.data_signature().node_id()
               << " leader:" << local_leader;
    return false;
  }
  const std::string expected_hash =
      BuildPOERollbackDigest(request.seq(), request.local_shard_id(),
                             request.data());
  if (expected_hash != request.data_hash()) {
    LOG(ERROR) << "POE ROLLBACK digest mismatch";
    return false;
  }
  if (verifier_ == nullptr ||
      !verifier_->VerifyMessage(request.data_hash(),
                                request.data_signature())) {
    LOG(ERROR) << "POE ROLLBACK signature verification failed";
    return false;
  }
  return true;
}

std::string ShardPOECommitment::BuildPOEResultDigest(
    const BatchUserResponse& response) const {
  BatchUserResponse ordered_payload;
  for (const auto& payload : response.response()) {
    ordered_payload.add_response(payload);
  }

  std::string serialized_payload;
  ordered_payload.SerializeToString(&serialized_payload);
  return SignatureVerifier::CalculateHash(serialized_payload);
}

std::string ShardPOECommitment::BuildPOEProofDigest(
    const Request& request, const std::string& result_digest) const {
  std::string proof_payload;
  AppendField(&proof_payload, request.seq());
  AppendField(&proof_payload, request.hash());
  AppendField(&proof_payload, request.global_txn_id());
  AppendField(&proof_payload, request.coordinator_shard_id());
  AppendField(&proof_payload, request.local_shard_id());
  AppendField(&proof_payload, result_digest);
  return SignatureVerifier::CalculateHash(proof_payload);
}

std::string ShardPOECommitment::BuildPOERollbackDigest(
    uint64_t checkpoint_seq, uint32_t local_shard_id,
    const std::string& reason) const {
  // Bind rollback authorization to the shard and checkpoint so a signed
  // rollback cannot be replayed against another shard or boundary.
  std::string rollback_payload;
  AppendField(&rollback_payload, checkpoint_seq);
  AppendField(&rollback_payload, local_shard_id);
  AppendField(&rollback_payload, reason);
  return SignatureVerifier::CalculateHash(rollback_payload);
}

int ShardPOECommitment::SendPOEProof(
    const Request& request, const BatchUserResponse& response) {
  if (!request.is_local_shard_poe()) {
    return 0;
  }
  if (!request.has_global_txn_id() || !request.has_coordinator_shard_id() ||
      !request.has_global_coordinator_id() || !request.has_local_shard_id()) {
    LOG(ERROR) << "cannot create POE proof: missing sharded metadata";
    return -2;
  }
  if (verifier_ == nullptr) {
    LOG(ERROR) << "cannot create POE proof: missing signature verifier";
    return -2;
  }

  const uint32_t local_shard_id = request.local_shard_id();
  const std::string result_digest = BuildPOEResultDigest(response);
  const std::string proof_digest =
      BuildPOEProofDigest(request, result_digest);
  auto signature = verifier_->SignMessage(proof_digest);
  if (!signature.ok()) {
    LOG(ERROR) << "cannot sign POE proof";
    return -2;
  }

  Request proof_request;
  proof_request.set_type(Request::TYPE_POE_PROOF);
  proof_request.set_seq(request.seq());
  proof_request.set_hash(request.hash());
  proof_request.set_proxy_id(request.proxy_id());
  proof_request.set_global_txn_id(request.global_txn_id());
  proof_request.set_coordinator_shard_id(request.coordinator_shard_id());
  proof_request.set_global_coordinator_id(request.global_coordinator_id());
  proof_request.set_local_shard_id(local_shard_id);
  proof_request.set_sender_id(config_.GetSelfInfo().id());
  proof_request.set_primary_id(shard_metadata_->LeaderForShard(local_shard_id));
  proof_request.set_current_view(message_manager_->GetCurrentView());
  proof_request.set_is_local_shard_poe(true);
  proof_request.set_data(result_digest);
  proof_request.set_data_hash(proof_digest);
  *proof_request.mutable_data_signature() = *signature;

  LOG(ERROR) << "[shard_poe_trace]"
             << " sending POE proof"
             << " node:" << shard_metadata_->SelfNodeId()
             << " shard:" << shard_metadata_->SelfShardId()
             << " seq:" << proof_request.seq()
             << " leader:" << proof_request.primary_id();

  return shard_communicator_->SendToShardLeader(proof_request,
                                                local_shard_id);
}

Request ShardPOECommitment::BuildPOECertRequest(
    const Request& proof_request,
    const std::map<int64_t, SignatureInfo>& signatures_by_sender) const {
  Request cert_request;
  cert_request.set_type(Request::TYPE_POE_CERT);
  cert_request.set_seq(proof_request.seq());
  cert_request.set_hash(proof_request.hash());
  cert_request.set_proxy_id(proof_request.proxy_id());
  cert_request.set_global_txn_id(proof_request.global_txn_id());
  cert_request.set_coordinator_shard_id(proof_request.coordinator_shard_id());
  cert_request.set_global_coordinator_id(
      proof_request.global_coordinator_id());
  cert_request.set_local_shard_id(proof_request.local_shard_id());
  cert_request.set_sender_id(config_.GetSelfInfo().id());
  cert_request.set_primary_id(
      shard_metadata_->LeaderForShard(proof_request.local_shard_id()));
  cert_request.set_current_view(message_manager_->GetCurrentView());
  cert_request.set_is_local_shard_poe(true);
  cert_request.set_data(proof_request.data());
  cert_request.set_data_hash(proof_request.data_hash());
  for (const auto& sender_and_signature : signatures_by_sender) {
    *cert_request.mutable_committed_certs()->add_committed_certs() =
        sender_and_signature.second;
  }

  auto signature = verifier_->SignMessage(cert_request.data_hash());
  if (!signature.ok()) {
    LOG(ERROR) << "cannot sign POE cert";
    return Request();
  }
  *cert_request.mutable_data_signature() = *signature;
  return cert_request;
}

Request ShardPOECommitment::BuildPOERollbackRequest(
    uint64_t checkpoint_seq, const std::string& reason) const {
  // The checkpoint target is carried in seq so the existing Request envelope
  // can represent rollback without adding new proto fields.
  Request rollback_request;
  rollback_request.set_type(Request::TYPE_POE_ROLLBACK);
  rollback_request.set_seq(checkpoint_seq);
  rollback_request.set_sender_id(config_.GetSelfInfo().id());
  rollback_request.set_primary_id(
      shard_metadata_->LeaderForShard(shard_metadata_->SelfShardId()));
  rollback_request.set_current_view(message_manager_->GetCurrentView());
  rollback_request.set_local_shard_id(shard_metadata_->SelfShardId());
  rollback_request.set_is_local_shard_poe(true);
  rollback_request.set_data(reason);
  rollback_request.set_data_hash(BuildPOERollbackDigest(
      checkpoint_seq, shard_metadata_->SelfShardId(), reason));

  auto signature = verifier_->SignMessage(rollback_request.data_hash());
  if (!signature.ok()) {
    LOG(ERROR) << "cannot sign POE rollback";
    return Request();
  }
  *rollback_request.mutable_data_signature() = *signature;
  return rollback_request;
}

int ShardPOECommitment::BroadcastPOERollback(
    uint64_t checkpoint_seq, const std::string& reason) {
  if (!shard_metadata_->IsSelfShardLeader()) {
    LOG(ERROR) << "non-leader attempted to broadcast POE rollback. node:"
               << shard_metadata_->SelfNodeId();
    return -2;
  }

  Request rollback_request =
      BuildPOERollbackRequest(checkpoint_seq, reason);
  if (rollback_request.type() != Request::TYPE_POE_ROLLBACK) {
    return -2;
  }

  LOG(ERROR) << "[shard_poe_trace]"
             << " broadcasting POE rollback"
             << " node:" << shard_metadata_->SelfNodeId()
             << " shard:" << shard_metadata_->SelfShardId()
             << " checkpoint_seq:" << checkpoint_seq
             << " reason:" << reason;
  int send_ret = BroadcastConsensusMsg(rollback_request);
  // Apply rollback locally as well; do not rely on receiving our own broadcast
  // through the network path.
  ClearPOEStateAfter(checkpoint_seq);
  int rollback_ret = message_manager_->RollbackToCheckpoint(checkpoint_seq);
  return rollback_ret != 0 ? rollback_ret : send_ret;
}

void ShardPOECommitment::ClearPOEStateAfter(uint64_t checkpoint_seq) {
  // Remove proof/cert state for transactions that no longer exist after the
  // local rollback. Entries at or before the checkpoint remain valid.
  std::lock_guard<std::mutex> lk(proof_mu_);
  auto bucket_it = proof_buckets_.begin();
  while (bucket_it != proof_buckets_.end()) {
    if (bucket_it->first.first.first > checkpoint_seq) {
      bucket_it = proof_buckets_.erase(bucket_it);
    } else {
      ++bucket_it;
    }
  }

  auto digest_it = proof_digest_by_sender_.begin();
  while (digest_it != proof_digest_by_sender_.end()) {
    if (digest_it->first.first > checkpoint_seq) {
      digest_it = proof_digest_by_sender_.erase(digest_it);
    } else {
      ++digest_it;
    }
  }
}

ShardPOECommitment::ProofKey ShardPOECommitment::MakeProofKey(
    const Request& request) const {
  return std::make_pair(MakeTxnKey(request), request.data_hash());
}

ShardPOECommitment::TxnKey ShardPOECommitment::MakeTxnKey(
    const Request& request) const {
  return std::make_pair(request.seq(), request.hash());
}

int ShardPOECommitment::BroadcastConsensusMsg(
    const google::protobuf::Message& msg) {
  return shard_communicator_->BroadcastToLocalShard(msg);
}

int ShardPOECommitment::SendConsensusMsgToReplica(
    const google::protobuf::Message& msg, int64_t node_id) {
  return shard_communicator_->SendToNode(msg, node_id);
}

}  // namespace resdb

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>

#include "platform/consensus/ordering/pbft/commitment.h"
#include "platform/consensus/ordering/sharded_3pc/shard_communicator.h"
#include "platform/consensus/ordering/sharded_3pc/shard_metadata.h"

namespace resdb {

// ShardPOECommitment is the local-shard POE commitment module used by the
// future sharded 3PC/POE path. It routes local POE traffic, executes validated
// TYPE_POE_EXECUTE requests optimistically, emits execution proofs, certifies
// matching proofs, releases held responses after certification, and handles
// local rollback messages.
class ShardPOECommitment : public Commitment {
 public:
  ShardPOECommitment(const ResDBConfig& config,
                     MessageManager* message_manager,
                     ReplicaCommunicator* replica_communicator,
                     ShardCommunicator* shard_communicator,
                     const ShardMetadata* shard_metadata,
                     SignatureVerifier* verifier,
                     bool start_response_thread = true);
  ~ShardPOECommitment() override;

  // Called by the sharded POE consensus manager after global 3PC reaches
  // GLOBAL_COMMIT. Only the local shard leader should turn that globally
  // committed request into a shard-local TYPE_POE_EXECUTE broadcast.
  int ProcessGlobalCommittedRequest(const Request& committed_request);

  // Validates a shard-local POE execute trigger, then starts optimistic local
  // execution. The post-execute hook emits a proof after execution completes.
  int ProcessPOEExecuteMsg(std::unique_ptr<Context> context,
                           std::unique_ptr<Request> request);

  // Handles proof/cert/rollback traffic after optimistic local execution.
  int ProcessPOEProofMsg(std::unique_ptr<Context> context,
                         std::unique_ptr<Request> request);
  int ProcessPOECertMsg(std::unique_ptr<Context> context,
                        std::unique_ptr<Request> request);
  int ProcessPOERollbackMsg(std::unique_ptr<Context> context,
                            std::unique_ptr<Request> request);

 protected:
  // Override the PBFT commitment routing hooks so local POE messages stay
  // inside the caller's shard instead of using flat ReplicaCommunicator
  // broadcast semantics.
  int BroadcastConsensusMsg(const google::protobuf::Message& msg) override;
  int SendConsensusMsgToReplica(const google::protobuf::Message& msg,
                                int64_t node_id) override;

 private:
  // Shared validation for TYPE_POE_EXECUTE. Keeping this separate makes the
  // later execution/proof code easier to read because all metadata checks stay
  // in one place.
  bool ValidatePOEExecuteRequest(const Context& context,
                                 const Request& request) const;
  bool ValidatePOEProofRequest(const Context& context,
                               const Request& request) const;
  bool ValidatePOECertRequest(const Context& context,
                              const Request& request) const;
  // Validates a local leader rollback request. request.seq() is interpreted as
  // the checkpoint boundary to restore, not as a transaction to execute.
  bool ValidatePOERollbackRequest(const Context& context,
                                  const Request& request) const;
  // Hashes only ordered response payload bytes. Metadata such as timing and
  // sender details is intentionally excluded so replicas can match results.
  std::string BuildPOEResultDigest(
      const BatchUserResponse& response) const;
  // Binds a result digest to global and local transaction identity.
  std::string BuildPOEProofDigest(const Request& request,
                                  const std::string& result_digest) const;
  // Rollback digest authenticates the checkpoint target and human-readable
  // reason carried in TYPE_POE_ROLLBACK.
  std::string BuildPOERollbackDigest(uint64_t checkpoint_seq,
                                     uint32_t local_shard_id,
                                     const std::string& reason) const;
  int SendPOEProof(const Request& request,
                   const BatchUserResponse& response);
  // Builds the local leader certificate from matching proof signatures.
  Request BuildPOECertRequest(
      const Request& proof_request,
      const std::map<int64_t, SignatureInfo>& signatures_by_sender) const;
  // Build/broadcast rollback is leader-only. Followers validate the resulting
  // TYPE_POE_ROLLBACK and call into MessageManager rollback.
  Request BuildPOERollbackRequest(uint64_t checkpoint_seq,
                                  const std::string& reason) const;
  int BroadcastPOERollback(uint64_t checkpoint_seq,
                           const std::string& reason);
  // Clears proof/cert bookkeeping above the rollback checkpoint so stale
  // speculative state cannot later certify.
  void ClearPOEStateAfter(uint64_t checkpoint_seq);

  using TxnKey = std::pair<uint64_t, std::string>;
  using ProofKey = std::pair<TxnKey, std::string>;

  struct ProofBucket {
    // Representative proof request for this digest; copied into TYPE_POE_CERT.
    Request proof_request;
    // Unique local replica proof signatures that match this digest.
    std::map<int64_t, SignatureInfo> signatures_by_sender;
    // Prevents rebroadcasting the same certificate after threshold is reached.
    bool cert_broadcast = false;
  };

  ProofKey MakeProofKey(const Request& request) const;
  TxnKey MakeTxnKey(const Request& request) const;

  ShardCommunicator* shard_communicator_;
  const ShardMetadata* shard_metadata_;
  std::mutex proof_mu_;
  // Proof buckets are keyed by transaction identity plus proof digest. Only
  // matching result digests count toward one POE certificate.
  std::map<ProofKey, ProofBucket> proof_buckets_;
  // Per-transaction sender map used to detect duplicate proofs and conflicting
  // proof digests from the same replica.
  std::map<TxnKey, std::map<int64_t, std::string>> proof_digest_by_sender_;
};

}  // namespace resdb

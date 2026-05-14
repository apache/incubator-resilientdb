#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "platform/consensus/ordering/pbft/commitment.h"

namespace resdb {

// Identifies each transaction phase in 3PC.
enum class ThreePCPhase {
  kInitial = 0,
  kReady,
  kPreCommit,
  kCommit,
  kAbort,
};

// Per request state tracked by the 3PC commitment module.
struct ThreePCTxnState {
  uint64_t seq = 0;
  ThreePCPhase phase = ThreePCPhase::kInitial;
  std::string hash;
  int64_t coordinator_id = 0;           // The coordinator for this transaction, which is responsible for driving the 3PC protocol forward and making the final commit/abort decision.
  int64_t proxy_id = 0;                 // The client proxy that originated this request, used for sending back the response after commit.

  // Full original client request, retained so the coordinator can later
  // send GLOBAL_COMMIT and the original request data.
  Request original_request;

  // 3PC vote tracking.
  std::unordered_set<int64_t> vote_commit_from;
  std::unordered_set<int64_t> precommit_ack_from;
};

/**************************
 * Commitment3PC implements the 3PC protocol, inheriting from PBFT specific functionality.
 * Tracks the state of each in-flight transaction and advances the 3PC state machine
 *   based on incoming messages and timers. It also handles broadcasting the appropriate 3PC messaged.
 * 3PC message types: PREPARE, VOTE_COMMIT, VOTE_ABORT, PRECOMMIT, PRECOMMIT_ACK, GLOBAL_COMMIT, GLOBAL_ABORT.
 ***************************/
class Commitment3PC : public Commitment {
 public:
  // start_response_thread is used to defer starting the response thread in cases like sharded 3PC where the commitment module is replaced and the response manager is initialized in the sharded commitment instead of the PBFT commitment.
  Commitment3PC(const ResDBConfig& config, MessageManager* message_manager,
                ReplicaCommunicator* replica_communicator,
                SignatureVerifier* verifier,
                bool start_response_thread = true)
      : Commitment(config, message_manager, replica_communicator, verifier,
                   start_response_thread) {}

  // Override PBFT commit path with 3PC-specific message handling.
  int ProcessNewRequest(std::unique_ptr<Context> context,
                        std::unique_ptr<Request> user_request) override;
  // 3PC message handlers - each advances the 3PC state machine and broadcasts next messages as needed.
  int ProcessPrepareMsg(std::unique_ptr<Context> context,
                        std::unique_ptr<Request> request);

  int ProcessVoteCommitMsg(std::unique_ptr<Context> context,
                           std::unique_ptr<Request> request);

  int ProcessVoteAbortMsg(std::unique_ptr<Context> context,
                          std::unique_ptr<Request> request);

  int ProcessPreCommitMsg(std::unique_ptr<Context> context,
                          std::unique_ptr<Request> request);

  int ProcessPreCommitAckMsg(std::unique_ptr<Context> context,
                             std::unique_ptr<Request> request);

  int ProcessGlobalCommitMsg(std::unique_ptr<Context> context,
                             std::unique_ptr<Request> request);

  int ProcessGlobalAbortMsg(std::unique_ptr<Context> context,
                            std::unique_ptr<Request> request);

 protected:
  // Expected participant count for a 3PC transaction, for sharded deployments.
  // In normal 3pc defaults to replica count. In sharded 3pc, this is overridden to the shard size.
  virtual size_t ExpectedThreePCParticipantCount() const;
  virtual int OnGlobalCommit(const Request& committed_request);

 private:
  // Check if the current node is the coordinator for a given request.
  bool IsCoordinator(const Request& request) const;
  // Get the current coordinator id.
  int64_t CoordinatorId(const Request& request) const;
  // Maybe broadcast a PRECOMMIT or GLOBAL_COMMIT message, return 1 if broadcast, 0 otherwise, -2 if invalid state.
  int MaybeBroadcastPreCommit(uint64_t seq);
  int MaybeBroadcastGlobalCommit(uint64_t seq);
  // Execute a committed transaction by passing it to the MessageManager's execution path.
  int ExecuteCommittedTxn(const Request& committed_request);
  // Mutex and map to track the state of in-flight transactions by sequence number.
  std::mutex txn_mu_;
  std::unordered_map<uint64_t, ThreePCTxnState> txn_state_;
};

}  // namespace resdb

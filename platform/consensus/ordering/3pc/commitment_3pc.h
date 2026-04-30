#pragma once

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
  uint32_t coordinator_id = 0;
  uint32_t proxy_id = 0;

  // Full original client request, retained so the coordinator can later
  // send GLOBAL_COMMIT and the original request data.
  Request original_request;

  // 3PC vote tracking.
  std::unordered_set<uint32_t> vote_commit_from;
  std::unordered_set<uint32_t> precommit_ack_from;
};

/**************************
 * Commitment3PC implements the 3PC protocol, inheriting from PBFT specific functionality.
 * Tracks the state of each in-flight transaction and advances the 3PC state machine
 *   based on incoming messages and timers. It also handles broadcasting the appropriate 3PC messaged.
 * 3PC message types: PREPARE, VOTE_COMMIT, VOTE_ABORT, PRECOMMIT, PRECOMMIT_ACK, GLOBAL_COMMIT, GLOBAL_ABORT.
 ***************************/
class Commitment3PC : public Commitment {
 public:
  Commitment3PC(const ResDBConfig& config, MessageManager* message_manager,
                ReplicaCommunicator* replica_communicator,
                SignatureVerifier* verifier)
      : Commitment(config, message_manager, replica_communicator, verifier) {}

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

 private:
  // Check if the current node is the coordinator for a given request.
  bool IsCoordinator() const {
    return config_.GetSelfInfo().id() == message_manager_->GetCurrentPrimary();
  }
  // Get the current coordinator id.
  uint32_t CoordinatorId() const {
    return message_manager_->GetCurrentPrimary();
  }
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

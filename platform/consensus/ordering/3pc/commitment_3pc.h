#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "platform/consensus/ordering/pbft/commitment.h"

namespace resdb {

enum class ThreePCPhase {
  kInitial = 0,
  kReady,
  kPreCommit,
  kCommit,
  kAbort,
};

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

class Commitment3PC : public Commitment {
 public:
  Commitment3PC(const ResDBConfig& config, MessageManager* message_manager,
                ReplicaCommunicator* replica_communicator,
                SignatureVerifier* verifier)
      : Commitment(config, message_manager, replica_communicator, verifier) {}

  int ProcessNewRequest(std::unique_ptr<Context> context,
                        std::unique_ptr<Request> user_request) override;

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
  bool IsCoordinator() const {
    return config_.GetSelfInfo().id() == message_manager_->GetCurrentPrimary();
  }

  uint32_t CoordinatorId() const {
    return message_manager_->GetCurrentPrimary();
  }

  int MaybeBroadcastPreCommit(uint64_t seq);
  int MaybeBroadcastGlobalCommit(uint64_t seq);

  int ExecuteCommittedTxn(const Request& committed_request);

  std::mutex txn_mu_;
  std::unordered_map<uint64_t, ThreePCTxnState> txn_state_;
};

}  // namespace resdb

#pragma once

#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "platform/consensus/ordering/spotless/algorithm/proposal_manager.h"
#include "platform/consensus/ordering/spotless/proto/proposal.pb.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace spotless {

class SpotLess : public common::ProtocolBase {
 public:
  SpotLess(int id, int f, int total_num, int block_size,
           SignatureVerifier* verifier);
  ~SpotLess();

  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  bool ReceiveProposal(std::unique_ptr<Proposal> proposal);
  bool ReceiveSync(std::unique_ptr<SyncMessage> sync);
  bool ReceiveRapidSync(std::unique_ptr<RapidSync> rapid_sync);
  void Start();

 private:
  struct InstanceContext {
    int instance_id = 0;
    std::unique_ptr<ProposalManager> manager;
    std::map<int, std::string> accepted_hash;
    std::set<int> proposed_views;
    std::set<int> sync_sent_views;
    std::set<int> prepared_views;
    uint64_t view_start_time = 0;
    int timeout_sent_view = 0;
  };

  void AsyncSend(int instance_id);
  void AsyncCommit();
  void AsyncViewTimeout();

  int LeaderFor(int instance_id, int view) const;
  bool IsLeader(int instance_id, int view) const;
  int ComputeInstanceId(const Transaction& txn) const;
  bool HasReadyInstanceLocked(int instance_id) const;
  void NoteViewProgressLocked(InstanceContext* instance, int view);
  void RememberCommittedTransactionsLocked(const Proposal& proposal);

  LockFreeQueue<Transaction>* GetInstanceQueue(int instance_id) const;
  std::vector<std::unique_ptr<Transaction>> PopTransactionsForInstance(
      int instance_id);

  SyncMessage BuildSyncMessageLocked(const Proposal& proposal,
                                     const Certificate& highest_cert) const;
  RapidSync BuildRapidSyncLocked(int instance_id, int from_view,
                                 int target_view,
                                 const Certificate& highest_cert) const;
  Certificate BuildCertificateLocked(
      int instance_id, int view, const std::string& hash,
      const std::map<int, SyncMessage>& votes) const;

  bool VerifySync(const SyncMessage& sync) const;
  bool VerifyRapidSync(const RapidSync& msg) const;
  void SendRapidSync(int instance_id, int target_view);
  void CommitProposal(std::unique_ptr<Proposal> proposal);

 private:
  LockFreeQueue<Proposal> commit_q_;

  mutable std::mutex mutex_, instance_mutex_[64];
  std::condition_variable vote_cv_;
  std::condition_variable timeout_cv_;

  std::vector<std::thread> send_threads_;
  std::thread commit_thread_;
  std::thread timeout_thread_;

  SignatureVerifier* verifier_;
  int batch_size_;
  int timeout_ms_;
  int total_instances_;
  int next_execute_slot_;
  int execute_id_;

  std::map<int, InstanceContext> instances_;
  LockFreeQueue<Transaction> txns_;
  std::map<int, std::map<int, std::map<std::string, std::map<int, SyncMessage>>>>
      sync_votes_;
  std::map<int, std::map<int, std::set<int>>> rapid_sync_ack_;
  std::map<int, std::map<int, Certificate>> rapid_sync_high_cert_;
  std::map<int, int> instance_commit_slots_;
  std::map<int, std::map<int, std::unique_ptr<Proposal>>> committed_by_slot_;
  std::set<std::string> seen_txns_;
  std::set<std::string> committed_txns_;

  Stats* global_stats_;
  std::atomic<bool> start_;
};

}  // namespace spotless
}  // namespace resdb

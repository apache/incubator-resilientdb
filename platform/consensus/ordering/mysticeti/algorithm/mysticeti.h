#pragma once

#include <map>
#include <queue>
#include <set>
#include <thread>

#include "common/crypto/signature_verifier.h"
#include "platform/statistic/stats.h"
#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "platform/consensus/ordering/mysticeti/proto/proposal.pb.h"
#include "platform/consensus/ordering/mysticeti/algorithm/proposal_manager.h"
#include "platform/config/resdb_config.h"

namespace resdb {
namespace mysticeti {

// Mysticeti-C: uncertified DAG consensus with 3 message delay commits.
// No explicit block certification — blocks reference parent blocks directly.
class Mysticeti : public common::ProtocolBase {
 public:
  Mysticeti(int id, int f, int total_num, SignatureVerifier* verifier,
            const ResDBConfig& config);
  ~Mysticeti();

  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  bool ReceiveBlock(std::unique_ptr<Proposal> proposal);
  // No ReceiveBlockACK or ReceiveBlockCert — Mysticeti uses uncertified DAG

 private:
  void CommitProposal(int round, int proposer);
  void CommitRound(int round);
  void AsyncCommit();
  void AsyncSend();
  void AsyncExecute();

  bool CheckBlock(const Proposal& p);
  void CheckFutureBlock(int round);

  int GetLeader(int64_t r);

 private:
  LockFreeQueue<Proposal> execute_queue_;
  LockFreeQueue<Transaction> txns_;
  // Replace bounded commit_queue_ (4096 cap) with an atomic high-water
  // mark. CommitRound just bumps the mark; AsyncCommit polls it. This
  // eliminates the LockFreeQueue::Push spin-wait that exhausted the
  // InputProcess worker pool after ~40s (4096/100 rounds/sec).
  std::atomic<int> commit_hwm_{-1};
  std::condition_variable commit_cv_;
  std::mutex commit_mutex_;

  std::unique_ptr<ProposalManager> proposal_manager_;
  SignatureVerifier* verifier_;

  // Separate block-processing thread: ReceiveBlock just enqueues; the
  // dag_thread_ does AddBlockToDAG + cert checking + commit triggering.
  // This mirrors Tusk's cert_thread_ pattern and decouples the DAG
  // work from the shared InputProcess worker pool — preventing the
  // pool-exhaustion livelock that killed post-crash throughput.
  LockFreeQueue<Proposal> dag_queue_;
  std::thread dag_thread_;
  void AsyncProcessDAG();

  std::thread send_thread_;
  std::thread commit_thread_, execute_thread_;
  std::thread liveness_thread_;
  void LivenessLoop();
  std::atomic<int64_t> last_block_time_{0};
  std::atomic<int64_t> last_commit_progress_{0};

  std::mutex txn_mutex_, mutex_;
  int limit_count_;
  std::condition_variable vote_cv_;
  int start_ = 0;
  int batch_size_ = 0;
  int execute_id_ = 1;
  std::atomic<int> queue_size_;

  Stats* global_stats_;

  std::mutex future_block_mutex_;
  std::map<int, std::map<std::string, std::unique_ptr<Proposal>>> future_block_;
  int64_t last_commit_time_ = 0;
  ResDBConfig config_;

  // Dedup commit triggers: prevent pushing the same round multiple times
  std::mutex commit_trigger_mutex_;
  std::set<int> committed_triggered_;
};

}  // namespace mysticeti
}  // namespace resdb

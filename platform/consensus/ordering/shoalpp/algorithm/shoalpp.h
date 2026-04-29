#pragma once

#include <deque>
#include <map>
#include <queue>
#include <thread>

#include "common/crypto/signature_verifier.h"
#include "platform/statistic/stats.h"
#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "platform/consensus/ordering/shoalpp/proto/proposal.pb.h"
#include "platform/consensus/ordering/shoalpp/algorithm/proposal_manager.h"
#include "platform/config/resdb_config.h"

namespace resdb {
namespace shoalpp {

class ShoalPP : public common::ProtocolBase {
 public:
  ShoalPP(int id, int f, int total_num, SignatureVerifier* verifier,
           const ResDBConfig& config);
  ~ShoalPP();

  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  bool ReceiveBlock(std::unique_ptr<Proposal> proposal);
  void ReceiveBlockACK(std::unique_ptr<Metadata> metadata);
  void ReceiveBlockCert(std::unique_ptr<Certificate> cert);

 private:
  void CommitProposal(int round, int proposer);
  void CommitRound(int round);
  void AsyncCommit();
  void AsyncSend();
  void AsyncExecute();

  bool VerifyCert(const Certificate& cert);

  bool CheckBlock(const Proposal& p);
  void CheckFutureBlock(int round);
  void CheckFutureCert(const Proposal& proposal);
  void CheckFutureCert(int round, const std::string& hash, int proposer);
  bool CheckCert(const Certificate& cert);
  bool SendBlockAck(std::unique_ptr<Proposal> proposal);

  void AsyncProcessCert();

  int GetLeader(int64_t r);

  // Shoal++ fast commit: track weak votes from uncertified proposals
  void TrackWeakVotes(const Proposal& proposal);
  bool TryFastCommit(int round, int proposer);

 private:
  LockFreeQueue<Proposal> execute_queue_, pending_block_;
  LockFreeQueue<int> commit_queue_;
  LockFreeQueue<Transaction> txns_;

  std::unique_ptr<ProposalManager> proposal_manager_;
  SignatureVerifier* verifier_;

  std::thread send_thread_;
  std::thread commit_thread_, execute_thread_;
  std::mutex txn_mutex_, mutex_;
  int limit_count_;
  std::map<std::string, std::map<int, std::unique_ptr<Metadata>>> received_num_;
  std::condition_variable vote_cv_;
  int start_ = 0;
  int batch_size_ = 0;
  int execute_id_ = 1;
  std::atomic<int> queue_size_;

  Stats* global_stats_;

  std::thread cert_thread_;
  LockFreeQueue<Certificate> cert_queue_;
  std::mutex future_block_mutex_, future_cert_mutex_, check_block_mutex_;
  std::map<int, std::map<std::string, std::unique_ptr<Proposal>>> future_block_;
  std::map<int, std::map<std::string, std::unique_ptr<Certificate>>> future_cert_;
  int64_t last_commit_time_ = 0;
  std::atomic<int64_t> last_commit_progress_{0};
  ResDBConfig config_;

  // Shoal++ weak vote tracking: [parent_round][parent_proposer] -> count
  std::mutex weak_vote_mutex_;
  std::map<int, std::map<int, int>> weak_votes_;
  std::set<std::pair<int, int>> fast_committed_;
  // Dedup: only count the first proposal from each sender per round
  std::map<int, std::set<int>> weak_vote_senders_;  // [round] -> {senders}
};

}  // namespace shoalpp
}  // namespace resdb

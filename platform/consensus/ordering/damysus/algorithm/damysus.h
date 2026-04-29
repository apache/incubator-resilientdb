#pragma once

#include <atomic>
#include <thread>
#include <map>
#include <set>
#include <mutex>
#include <condition_variable>

#include "common/crypto/signature_verifier.h"
#include "platform/statistic/stats.h"
#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "platform/consensus/ordering/damysus/proto/proposal.pb.h"
#include "platform/consensus/ordering/damysus/algorithm/proposal_manager.h"
#include "enclave/sgx_cpp_u.h"
#include "platform/config/resdb_config.h"

namespace resdb {
namespace damysus {

class Damysus : public common::ProtocolBase {
 public:
  Damysus(int id, int f, int total_num, SignatureVerifier* verifier,
          const ResDBConfig& config, oe_enclave_t* enclave);
  ~Damysus();

  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  bool ReceiveNewView(std::unique_ptr<NewViewMsg> msg);
  bool ReceiveProposal(std::unique_ptr<Proposal> proposal);
  bool ReceivePreCommitVote(std::unique_ptr<PreCommitVote> vote);
  bool ReceiveDecide(std::unique_ptr<DecideMsg> decide);

 private:
  void AsyncSend();
  void AsyncCommit();
  // Timeout-driven view change: if no decide received within
  // kViewTimeoutMs, force AdvanceView() and broadcast NewView so the
  // next (alive) leader can take over after a crash.
  void TimeoutLoop();

  int GetLeader(int view);
  bool IsLeader(int view);
  void AdvanceView();

  // TEE wrappers
  Commitment CallTEEprepare(const std::string& block_hash,
                            const Accumulator& acc);
  Commitment CallTEEstore(const Commitment& block_cert);
  Commitment CallTEEsign();
  Accumulator CallAccumList(const std::vector<Commitment>& nv_certs);

 private:
  LockFreeQueue<Transaction> txns_;
  LockFreeQueue<Proposal> commit_queue_;

  std::unique_ptr<ProposalManager> proposal_manager_;
  SignatureVerifier* verifier_;
  oe_enclave_t* enclave_;
  ResDBConfig config_;

  std::thread send_thread_;
  std::thread commit_thread_;
  std::thread timeout_thread_;
  std::atomic<int64_t> last_advance_time_{0};
  std::mutex mutex_;
  std::condition_variable vote_cv_;

  int current_view_;
  int limit_count_;  // f+1
  int batch_size_;
  bool has_proposed_;

  // Message collection: view -> sender -> message
  std::map<int, std::map<int, std::unique_ptr<NewViewMsg>>> new_view_msgs_;
  std::map<int, std::map<int, std::unique_ptr<PreCommitVote>>> precommit_votes_;
  std::set<int> received_decide_views_;  // commit_cert optimization: skip NewView

  Stats* global_stats_;
  int execute_id_;
};

}  // namespace damysus
}  // namespace resdb

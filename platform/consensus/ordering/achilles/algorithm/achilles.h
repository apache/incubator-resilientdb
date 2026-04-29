#pragma once

#include <atomic>
#include <chrono>
#include <thread>
#include <map>
#include <mutex>
#include <condition_variable>

#include "common/crypto/signature_verifier.h"
#include "platform/statistic/stats.h"
#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "platform/consensus/ordering/achilles/proto/proposal.pb.h"
#include "platform/consensus/ordering/achilles/algorithm/proposal_manager.h"
#include "enclave/sgx_cpp_u.h"
#include "platform/config/resdb_config.h"

namespace resdb {
namespace achilles {

class Achilles : public common::ProtocolBase {
 public:
  Achilles(int id, int f, int total_num, SignatureVerifier* verifier,
           const ResDBConfig& config, oe_enclave_t* enclave);
  ~Achilles();

  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  bool ReceiveViewCert(std::unique_ptr<ViewCertificate> vc);
  bool ReceiveProposal(std::unique_ptr<Proposal> proposal);
  bool ReceiveStoreCert(std::unique_ptr<StoreCertificate> sc);
  bool ReceiveDecide(std::unique_ptr<CommitmentCertificate> cc);

 private:
  void AsyncSend();
  void AsyncCommit();
  void TimeoutLoop();
  void ForceAdvanceView();

  int GetLeader(int view);
  bool IsLeader(int view);
  void AdvanceView();

  // TEE wrappers (reuse Damysus enclave ECalls)
  BlockCertificate CallTEEprepare(const std::string& block_hash,
                                   const Accumulator& acc);
  StoreCertificate CallTEEstore(const BlockCertificate& block_cert);
  ViewCertificate CallTEEview();
  Accumulator CallTEEaccum(const std::vector<ViewCertificate>& view_certs);

  // Chained commit: commit block and all uncommitted ancestors
  void ChainCommit(const std::string& block_hash);

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
  std::mutex mutex_;
  std::condition_variable vote_cv_;

  // Timeout-driven view change. Sentinel -1 until the first transaction
  // is received so the timeout doesn't fire during the deploy wait.
  std::atomic<int64_t> last_view_advance_time_{-1};
  std::atomic<int> last_observed_view_{0};
  // Track views for which we received a proposal from the leader.
  // Used by smart timeout: if proposal received → leader alive → wait.
  std::set<int> received_proposal_views_;

  int current_view_;
  int limit_count_;  // f+1
  int batch_size_;
  bool has_proposed_;

  // preb: latest stored block from a leader (Algorithm 1, line 3)
  // Protected by preb_mutex_ — accessed from both network and send threads
  struct PreB {
    std::string block_hash;
    BlockCertificate block_cert;
    CommitmentCertificate commit_cert;
  } preb_;
  std::mutex preb_mutex_;

  // Message collection
  std::map<int, std::map<int, std::unique_ptr<ViewCertificate>>> view_certs_;
  std::map<int, std::map<int, std::unique_ptr<StoreCertificate>>> store_certs_;

  // Optimization: commitment cert received for previous view (skip new-view)
  std::map<int, std::unique_ptr<CommitmentCertificate>> received_commit_certs_;
  // Guard: prevent double Decide broadcast for same view
  std::set<int> decided_views_;

  Stats* global_stats_;
  int execute_id_;
};

}  // namespace achilles
}  // namespace resdb

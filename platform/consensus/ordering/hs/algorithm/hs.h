#pragma once

#include <atomic>
#include <chrono>
#include <thread>

#include "platform/common/queue/lock_free_queue.h"
#include "platform/statistic/stats.h"
#include "platform/config/resdb_config.h"
#include "platform/consensus/ordering/hs/algorithm/proposal_manager.h"
#include "platform/consensus/ordering/hs/proto/proposal.pb.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"

namespace resdb {
namespace hs {

class HotStuff: public common::ProtocolBase {
 public:
  HotStuff(int id, int f, int total_num, SignatureVerifier* verifier,
           const ResDBConfig& config);
  ~HotStuff();

  //  recv txn -> send block with links -> rec block ack -> send block with certs
  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  bool ReceiveProposal(std::unique_ptr<Proposal> proposal);
  bool ReceiveCertificate(std::unique_ptr<Certificate> cert);
  bool ReceiveDecide(std::unique_ptr<QC> qc);


  private:
    bool Ready();
    void StartNewRound();
    void AsyncSend();
    void AsyncCommit();
    void TimeoutLoop();
    void AdvanceViewOnTimeout();

    std::unique_ptr<Certificate> GenerateCertificate(const Proposal& proposal);

    int NextLeader(int view);
    bool IsLeader(int view);

    void CommitProposal(std::unique_ptr<Proposal> p);

 private:
  LockFreeQueue<Transaction> txns_;
  LockFreeQueue<Proposal> commit_q_;

  std::mutex mutex_, n_mutex_, pmutex_[1024];
  //std::mutex mutex_, n_mutex_;
  std::condition_variable vote_cv_;
  std::unique_ptr<ProposalManager> proposal_manager_;
  bool has_sent_;
  SignatureVerifier * verifier_;

  std::thread send_thread_, commit_thread_, timeout_thread_;

  int batch_size_;
  //[view][hash][signer][cert]
  //std::map<std::string, std::map<int, std::unique_ptr<Certificate>> >  receive_[1024];
  std::map<int,  std::map<std::string, std::map<int, std::unique_ptr<Certificate>> > > receive_;

  // Timeout-driven view change. When the current leader is crashed and
  // no Decide arrives, the TimeoutLoop thread bumps the view locally so
  // a new leader can take over. Initialised to -1 as a sentinel so
  // the timeout doesn't fire during the deploy phase before any txn
  // has ever arrived.
  std::atomic<int64_t> last_view_advance_time_{-1};
  std::atomic<int> last_observed_view_{1};

  Stats* global_stats_;
};

}  // namespace tusk
}  // namespace resdb

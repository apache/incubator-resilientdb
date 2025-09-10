#pragma once

#include <thread>

#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/hs1/algorithm/proposal_manager.h"
#include "platform/consensus/ordering/hs1/proto/proposal.pb.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace hs1 {

class HotStuff1: public common::ProtocolBase {
 public:
  HotStuff1(int id, int f, int total_num, SignatureVerifier* verifier, int non_responsive_num, int fork_tail_num, int rollback_num, uint64_t timer_length);
  ~HotStuff1();

  //  recv txn -> send block with links -> rec block ack -> send block with certs
  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  bool ReceiveProposal(std::unique_ptr<Proposal> proposal);
  bool ReceiveCertificate(std::unique_ptr<Certificate> cert);


  private:
    bool Ready();
    void StartNewRound();
    void AsyncSend();
    void AsyncCommit();

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

  std::thread send_thread_, commit_thread_;

  int batch_size_;
  //[view][hash][signer][cert]
  //std::map<std::string, std::map<int, std::unique_ptr<Certificate>> >  receive_[1024];
  std::map<int,  std::map<std::string, std::map<int, std::unique_ptr<Certificate>> > > receive_;
  Stats* global_stats_ = nullptr;

  int non_responsive_num_, fork_tail_num_, rollback_num_;
  uint64_t timer_length_;

  uint64_t last_time_ = 0;

  bool qc_formed_, proposal_received_;
  std::unique_ptr<QC> formed_qc_;

  uint64_t crash_num_ = 0;
};

}  // namespace hs
}  // namespace resdb

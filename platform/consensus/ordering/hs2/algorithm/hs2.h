#pragma once

#include <thread>

#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/hs2/algorithm/proposal_manager.h"
#include "platform/consensus/ordering/hs2/proto/proposal.pb.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace hs2 {

class HotStuff2: public common::ProtocolBase {
 public:
  HotStuff2(int id, int f, int total_num, SignatureVerifier* verifier, int non_responsive_num, int fork_tail_num );
  ~HotStuff2();

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

  int non_responsive_num_;
  int fork_tail_num_;
};

}  // namespace tusk
}  // namespace resdb

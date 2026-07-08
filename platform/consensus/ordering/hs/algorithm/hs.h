#pragma once

#include <set>
#include <thread>

#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/hs/algorithm/proposal_manager.h"
#include "platform/consensus/ordering/hs/proto/proposal.pb.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"

namespace resdb {
namespace hs {

class HotStuff: public common::ProtocolBase {
 public:
  HotStuff(int id, int f, int total_num, SignatureVerifier* verifier );
  ~HotStuff();

  //  recv txn -> send block with links -> rec block ack -> send block with certs
  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  bool ReceiveProposal(std::unique_ptr<Proposal> proposal);
  bool ReceiveCertificate(std::unique_ptr<Certificate> cert);
  bool ReceiveStartView(std::unique_ptr<StartView> start_view);


  private:
    bool Ready();
    void StartNewRound();
    void AsyncSend();
    void AsyncCommit();
    void AsyncViewTimeout();
    void SendStartView(int view);
    void UpdateViewTimer(int view);

    std::unique_ptr<Certificate> GenerateCertificate(const Proposal& proposal);

    int NextLeader(int view);
    bool IsLeader(int view);

    void CommitProposal(std::unique_ptr<Proposal> p);

 private:
  LockFreeQueue<Transaction> txns_;
  LockFreeQueue<Proposal> commit_q_;

  std::mutex mutex_, n_mutex_, pmutex_[1024];
  //std::mutex mutex_, n_mutex_;
  std::condition_variable vote_cv_, timeout_cv_;
  std::unique_ptr<ProposalManager> proposal_manager_;
  bool has_sent_;
  SignatureVerifier * verifier_;

  std::thread send_thread_, commit_thread_, timeout_thread_;

  int batch_size_;
  int timeout_ms_;
  //[view][hash][signer][cert]
  //std::map<std::string, std::map<int, std::unique_ptr<Certificate>> >  receive_[1024];
  std::map<int,  std::map<std::string, std::map<int, std::unique_ptr<Certificate>> > > receive_;
  std::map<int, std::set<int> > start_view_ack_;
  std::set<int> received_proposal_views_;
  int tracked_view_;
  int timeout_sent_view_;
  uint64_t view_start_time_;

};

}  // namespace tusk
}  // namespace resdb

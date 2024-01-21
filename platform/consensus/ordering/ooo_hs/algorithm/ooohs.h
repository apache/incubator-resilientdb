#pragma once

#include <thread>

#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/ooo_hs/algorithm/proposal_manager.h"
#include "platform/consensus/ordering/ooo_hs/proto/proposal.pb.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"

namespace resdb {
namespace ooohs {

class OOOHotStuff: public common::ProtocolBase {
 public:
  OOOHotStuff(int id, int f, int total_num, SignatureVerifier* verifier );
  ~OOOHotStuff();

  //  recv txn -> send block with links -> rec block ack -> send block with certs
  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  bool ReceiveProposal(std::unique_ptr<Proposal> proposal);
  bool ReceiveCertificate(std::unique_ptr<Certificate> cert);


  private:
    bool Ready(int k);
    void StartNewRound();
    void AsyncSend(int k);
    void AsyncCommit();

    std::unique_ptr<Certificate> GenerateCertificate(const Proposal& proposal);

    int NextLeader(int view);
    bool IsLeader(int view);

    void CommitProposal(std::unique_ptr<Proposal> p);
    bool ProcessProposal(std::unique_ptr<Proposal> proposal);

 private:
  LockFreeQueue<Transaction> txns_;
  LockFreeQueue<Proposal> commit_q_;

  std::mutex mutex_, n_mutex_, p_mutex_;
  //std::mutex mutex_, n_mutex_;
  std::condition_variable vote_cv_;
  std::unique_ptr<ProposalManager> proposal_manager_;
  std::vector<int> has_sent_;
  SignatureVerifier * verifier_;

  std::thread commit_thread_;
  std::vector<std::thread> send_thread_;

  int batch_size_;
  int slot_num_;

  //[view][hash][signer][cert]
  //std::map<std::string, std::map<int, std::unique_ptr<Certificate>> >  receive_[1024];
  std::map<std::pair<int,int>,  std::map<std::string, std::map<int, std::unique_ptr<Certificate>> > > receive_;
  std::map<std::pair<int,int>, std::unique_ptr<Proposal> >  proposals_;
  std::map<std::pair<int,int>, std::unique_ptr<Proposal> > pending_commit_;
};

}  // namespace tusk
}  // namespace resdb

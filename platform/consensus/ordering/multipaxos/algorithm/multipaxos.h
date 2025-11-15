#pragma once

#include <thread>

#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/multipaxos/algorithm/proposal_manager.h"
#include "platform/consensus/ordering/multipaxos/proto/proposal.pb.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"

namespace resdb {
namespace multipaxos {

class MultiPaxos: public common::ProtocolBase {
 public:
  MultiPaxos(int id, int f, int total_num, int block_size, SignatureVerifier* verifier );
  ~MultiPaxos();

  //  recv txn -> send block with links -> rec block ack -> send block with certs
  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  bool ReceiveProposal(std::unique_ptr<Proposal> proposal);
  bool ReceiveLearn(std::unique_ptr<Proposal> proposal);
    bool ReceiveAccept(std::unique_ptr<Proposal> proposal);


  private:
    bool Ready();
    void AsyncSend();
    void AsyncCommit();
    void AsyncCommitSeq();
    void AsyncLearn();

    void CommitProposal(std::unique_ptr<Proposal> p);

    void AddCommitData(std::unique_ptr<Proposal> p);
    std::unique_ptr<Proposal> GetCommitData();
    bool Wait() ;

  bool WaitPropose(int round);


 private:
  LockFreeQueue<Transaction> txns_;
  LockFreeQueue<Proposal> commit_q_, learn_q_;

  std::unique_ptr<ProposalManager> proposal_manager_;

  std::thread send_thread_, commit_thread_, commit_seq_thread_, learn_thread_;

  int batch_size_;

  std::mutex mutex_;
  std::map<int,  std::map<int, std::unique_ptr<Proposal>> > receive_;

  std::mutex learn_mutex_, propose_mutex_;
  std::map<int,  std::set<int>> learn_receive_;
  std::map<int,  std::set<int>> accept_receive_;
  std::map<int, int> can_propose_;

  std::mutex n_mutex_;
  std::map<int, std::unique_ptr<Proposal> > commit_data_;
  std::condition_variable vote_cv_, propose_cv_;
  int start_seq_;
  int master_;
};

}  // namespace tusk
}  // namespace resdb

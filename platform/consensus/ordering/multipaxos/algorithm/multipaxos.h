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
  MultiPaxos(int id, int f, int total_num, SignatureVerifier* verifier );
  ~MultiPaxos();

  //  recv txn -> send block with links -> rec block ack -> send block with certs
  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  bool ReceiveProposal(std::unique_ptr<Proposal> proposal);
  bool ReceiveLearn(std::unique_ptr<Proposal> proposal);


  private:
    bool Ready();
    void AsyncSend();
    void AsyncCommit();
    void AsyncCommitSeq();

    void CommitProposal(std::unique_ptr<Proposal> p);

    void AddCommitData(std::unique_ptr<Proposal> p);
    std::unique_ptr<Proposal> GetCommitData();
    bool Wait() ;


 private:
  LockFreeQueue<Transaction> txns_;
  LockFreeQueue<Proposal> commit_q_;

  std::unique_ptr<ProposalManager> proposal_manager_;

  std::thread send_thread_, commit_thread_, commit_seq_thread_;

  int batch_size_;

  std::mutex mutex_;
  std::map<int,  std::map<int, std::unique_ptr<Proposal>> > receive_;

  std::mutex learn_mutex_;
  std::map<int,  std::set<int>> learn_receive_;

  std::mutex n_mutex_;
  std::map<int, std::unique_ptr<Proposal> > commit_data_;
  std::condition_variable vote_cv_;
  int start_seq_;
};

}  // namespace tusk
}  // namespace resdb

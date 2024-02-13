#pragma once

#include <deque>
#include <map>
#include <queue>
#include <thread>

#include "common/crypto/signature_verifier.h"
#include "platform/statistic/stats.h"
#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/fairdag/algorithm/proposal_manager.h"
#include "platform/consensus/ordering/fairdag/proto/proposal.pb.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"

namespace resdb {
namespace fairdag {

class Tusk {
 public:
  Tusk(int id, int f, int total_num, SignatureVerifier* verifier,
  common::ProtocolBase::SingleCallFuncType single_call,  
  common::ProtocolBase :: BroadcastCallFuncType broadcast_call,
  std::function<void(std::vector<std::unique_ptr<Transaction>> txns)> commit
  );
  ~Tusk();

  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  void ReceiveBlockACK(std::unique_ptr<Metadata> metadata);
  void ReceiveBlockCert(std::unique_ptr<Certificate> cert);
  bool ReceiveBlock(std::unique_ptr<Proposal> proposal);

  bool IsStop();
  void Stop();

private:
  void CommitProposal(int round, int proposer);
  void CommitRound(int round);
  void AsyncCommit();
  void AsyncSend();
  void AsyncExecute();

private:
  int GetLeader(int64_t r);
  bool VerifyCert(const Certificate& cert);

  bool SendBlockAck(std::unique_ptr<Proposal> proposal);
  bool CheckCert(const Proposal& proposal);
  void CheckFutureBlock(int round);

 private:
  int id_;
  int f_;
  std::atomic<int> local_txn_id_;
  LockFreeQueue<Proposal> execute_queue_;
  LockFreeQueue<int> commit_queue_;
  LockFreeQueue<Transaction> txns_;

  std::unique_ptr<ProposalManager> proposal_manager_;
  SignatureVerifier* verifier_;

  std::thread send_thread_; 
  std::thread commit_thread_, execute_thread_;
  std::mutex txn_mutex_, mutex_, ftxn_mutex_;
  int limit_count_;
  std::map<std::string, std::map<int, std::unique_ptr<Metadata>> > received_num_;
  std::condition_variable vote_cv_;
  int start_ = 0;
  int batch_size_ = 0;
  int execute_id_ = 1;
  std::atomic<bool> is_stop_;
  int total_num_;
  common::ProtocolBase::SingleCallFuncType single_call_;
  common::ProtocolBase::BroadcastCallFuncType broadcast_call_;
  std::function<void(std::vector<std::unique_ptr<Transaction>> txns)> commit_;
  std::map<int, std::map<std::string, std::unique_ptr<Proposal>> > future_proposal_;
  std::map<int, std::map<std::string, std::unique_ptr<Certificate> > > future_cert_;

  Stats* global_stats_;
};

}  // namespace tusk
}  // namespace resdb

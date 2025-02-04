#pragma once

#include <deque>
#include <map>
#include <queue>
#include <thread>

#include "common/crypto/signature_verifier.h"
#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/common/protocol/protocol_base.h"
#include "platform/consensus/ordering/tusk/proto/proposal.pb.h"
#include "platform/consensus/ordering/tusk/protocol/proposal_manager.h"

namespace resdb {
namespace tusk {

class Tusk : public protocol::ProtocolBase {
 public:
  Tusk(int id, int f, int total_num, SignatureVerifier* verifier);
  ~Tusk();

  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  bool ReceiveBlock(std::unique_ptr<Proposal> proposal);
  void ReceiveBlockACK(std::unique_ptr<Metadata> metadata);
  void ReceiveBlockCert(std::unique_ptr<Certificate> cert);

  void SetVerifyFunc(std::function<bool(const Transaction& txn)> func);

  int DagID() { return dag_id_; }
  void StopDone();

  bool WaitForNext(bool);

 private:
  // bool ProcessBlock(std::unique_ptr<Proposal> proposal);
  void CommitProposal(int round, int proposer);
  void CommitRound(int round);
  void AsyncCommit();
  void AsyncSend();
  void AsyncExecute();
  void SwitchDAG();
  // void AsyncBlock();

  bool VerifyRWS(const Proposal& proposal);
  bool VerifyCert(const Certificate& cert);
  void RunFutureBlock();
  void RunFutureCert();
  void AddBackTxn(const Transaction& tx);

  bool WaitReady();
  void WaitBatchReady();
  void AddCrossTxn(int dag_id, int round, int proposer);
  void RelaseCrossTxn(int dag_id, int round, int proposer);
  void SendTransactions(const std::vector<std::unique_ptr<Transaction>>& txns);

 private:
  int GetLeader(int64_t r);

  void CreateManager(int dag);
  ProposalManager* GetManager(int dag);

  void CheckCross(const Proposal& proposal);
  void AddIfCrossTxn(const Proposal& p);
  void SetCheck(int round, int proposer);
  bool HasCheck(int round, int proposer);
  void CheckCrossRound(int round);
  void NotifyInput();

 private:
  std::atomic<int> local_txn_id_;
  LockFreeQueue<Proposal> pending_block_, pending_;
  LockFreeQueue<const Proposal*> execute_queue_;
  LockFreeQueue<int> commit_queue_;
  LockFreeQueue<Transaction> txns_;

  std::unique_ptr<ProposalManager> proposal_manager_;
  LockFreeQueue<Certificate> future_cert_;
  // std::map<int, std::unique_ptr<ProposalManager>> proposal_manager_;
  SignatureVerifier* verifier_;

  std::thread send_thread_, future_thread_, future_cert_thread_;
  std::thread commit_thread_, execute_thread_, block_thread_;
  std::mutex txn_mutex_, mutex_, switch_mutex_, p_mutex_, m_mutex_, send_mutex_;
  int limit_count_;
  std::map<int, std::map<std::string, std::map<int, std::unique_ptr<Metadata>>>>
      received_num_;
  std::condition_variable vote_cv_, send_cv_;
  int start_ = 0;
  int batch_size_ = 0;
  int execute_id_ = 1;
  std::vector<std::unique_ptr<Transaction>> pending_txn_;
  std::atomic<int> txn_pending_num_;

  std::function<bool(const Transaction& txn)> verify_func_;
  std::set<int> received_stop_;

  std::mutex cert_mutex_, block_mutex_;
  std::condition_variable cert_cv_, block_cv_;
  std::map<int, std::vector<std::unique_ptr<Certificate>>> future_cert_map_;
  std::map<int, std::vector<std::unique_ptr<Proposal>>> future_block_map_;
  std::atomic<int> dag_id_;
  bool stop_commit_ = false;
  int previous_round_ = -2;

  std::mutex cross_mutex_;
  std::condition_variable cross_cv_, txn_cv_;
  std::map<int, std::set<std::pair<int, int>>> cross_txn_;
  std::map<int, std::set<std::pair<int, int>>> future_cross_txn_;

  std::map<int, std::map<std::string, int>> received_flag_;
  std::vector<std::unique_ptr<Transaction>> vec_txns_;
  std::map<int, int> txn_num_;

  std::set<std::pair<int, int>> has_check_;
  int last_id_;
  int64_t last_time_;
};

}  // namespace tusk
}  // namespace resdb

#pragma once

#include <deque>
#include <map>
#include <queue>
#include <thread>

#include "common/crypto/signature_verifier.h"
#include "platform/statistic/stats.h"
#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/fairdag_rl/algorithm/proposal_manager.h"
#include "platform/consensus/ordering/fairdag_rl/proto/proposal.pb.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "platform/consensus/ordering/common/fairness/order_manager.h"

namespace resdb {
namespace fairdag_rl {

class Tusk {
 public:
  Tusk(int id, int f, int total_num, SignatureVerifier* verifier,
    common::ProtocolBase::SingleCallFuncType single_call,  
    common::ProtocolBase :: BroadcastCallFuncType broadcast_call,
    std::function<void(std::unique_ptr<std::vector<std::unique_ptr<Transaction>>>)> commit
  );
  ~Tusk();

  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  bool ReceiveBlock(std::unique_ptr<Proposal> proposal);
  void ReceiveBlockACK(std::unique_ptr<Metadata> metadata);
  void ReceiveBlockCert(std::unique_ptr<Certificate> cert);

  bool IsStop();
  void Stop();

  std::unique_ptr<Transaction> FetchTxn(const std::string& hash);

private:
  void CommitLeaderVertex(int round, int proposer);
  void CommitRound(int round);
  void AsyncCommitLeaderVertex();
  void AsyncSend();
  void AsyncCommitCausalHistory();

private:
  int GetLeader(int64_t r);
  bool VerifyCert(const Certificate& cert);
  bool PathExist(const Proposal *req1, const Proposal *req2);

  bool CheckBlock(const Proposal& p);
  void CheckFutureBlock(int round);
  void CheckFutureCert(const Proposal& proposal);
  void CheckFutureCert(int round, const std::string& hash, int proposer);
  bool CheckCert(const Certificate& cert);
  bool SendBlockAck(std::unique_ptr<Proposal> proposal);

  void AsyncProcessCert();

  void AddTxnFromProposer(const Transaction& txn);
  std::string GddTxnFromProposer();

 private:
  int id_;
  int f_;
  std::atomic<int> local_txn_id_;
  LockFreeQueue<Proposal> leader_vertex_queue_;
  LockFreeQueue<int> commit_queue_;
  //LockFreeQueue<std::string> txns_[2];
  LockFreeQueue<std::string> txns_;

  std::unique_ptr<ProposalManager> proposal_manager_;
  SignatureVerifier* verifier_;

  std::thread send_thread_; 
  std::thread commit_thread_, execute_thread_;
  std::mutex txn_mutex_, mutex_, prio_mutex_, lo_mutex_;
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
  std::function<void(std::unique_ptr<std::vector<std::unique_ptr<Transaction>>>)> commit_;


  Stats* global_stats_;

  std::thread cert_thread_, process_execute_thread_;
  LockFreeQueue<Certificate> cert_queue_;
  std::mutex future_block_mutex_, future_cert_mutex_, check_block_mutex_;
  std::map<int, std::map<std::string, std::unique_ptr<Proposal>>> future_block_;
  std::map<int, std::map<std::string, std::unique_ptr<Certificate>>> future_cert_;
  int64_t  last_commit_time_ = 0;

  struct Node {
    public:
      int64_t seq;
      int proposer;
      std::string hash;
      Node(int64_t seq, int proposer, const std::string& hash) 
      : seq(seq), proposer(proposer), hash(hash){}

  };
  
  class prio {
  public:
      bool operator()(const Node &a, const Node &b) {
        return a.seq > b.seq || (a.seq == b.seq && a.proposer > b.proposer);
      }
  };
  std::priority_queue<Node, std::vector<Node>, prio> prio_txns_;

  std::map<int, int > proposer_seq_;
  int proposer_num_ = 2;

  bool faulty_test_ = false;
  uint64_t faulty_replica_num_ = 1;

  std::unique_ptr<OrderManager> order_manager_;
};

}  // namespace tusk
}  // namespace resdb

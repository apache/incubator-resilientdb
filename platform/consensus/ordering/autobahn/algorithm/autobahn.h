#pragma once

#include <deque>
#include <map>
#include <queue>
#include <thread>

#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "platform/consensus/ordering/autobahn/algorithm/proposal_manager.h"
#include "platform/consensus/ordering/autobahn/proto/proposal.pb.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace autobahn {

class AutoBahn: public common::ProtocolBase {
 public:
  AutoBahn(int id, int f, int total_num, int block_size, SignatureVerifier* verifier);
  ~AutoBahn();

  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  void ReceiveBlock(std::unique_ptr<Block> block);
  void ReceiveBlockACK(std::unique_ptr<BlockACK> block);
  bool ReceiveVote(std::unique_ptr<Proposal>);
  bool ReceiveProposal(std::unique_ptr<Proposal> proposal);
  bool ReceiveCommit(std::unique_ptr<Proposal> proposal);
  bool ReceivePrepare(std::unique_ptr<Proposal> proposal);

 private:
  bool IsStop();
  void BroadcastTxn();
  void GenerateBlocks();
  void AsyncDissemination();
  void AsyncConsensus();
  void AsyncPrepare();
  void AsyncCommit();

  bool WaitForResponse(int64_t block_id);
  void BlockDone();
  void PrepareDone(std::unique_ptr<Proposal> vote);
  void CommitDone(std::unique_ptr<Proposal> proposal);
  
  void NotifyView();
  bool WaitForNextView(int view);

  void Prepare(std::unique_ptr<Proposal> vote);
  void Commit(std::unique_ptr<Proposal> proposal);

  bool IsFastCommit(const Proposal& proposal);

  bool WaitForNextLeader();
  void StartNextLeader(int slot);


 private:
  std::condition_variable bc_block_cv_, view_cv_, leader_cv_;
  LockFreeQueue<Transaction> txns_;
  LockFreeQueue<Proposal> prepare_queue_, commit_queue_;
  std::unique_ptr<ProposalManager> proposal_manager_;
  SignatureVerifier* verifier_;
  int execute_id_;

  int id_, total_num_, f_, batch_size_;
  std::atomic<int> is_stop_;
  int timeout_ms_;

  std::thread block_thread_, dissemi_thread_, consensus_thread_, prepare_thread_, commit_thread_;

  std::mutex block_mutex_, bc_mutex_, view_mutex_, vote_mutex_, commit_mutex_, leader_mutex_;
  std::map<int, std::map<int, SignInfo>> block_ack_;
  std::map<int, std::map<int, std::unique_ptr<Proposal>>> vote_ack_ ;
  std::map<int, std::set<int>>  commit_ack_;
  Stats* global_stats_;
  std::map<int, int64_t> commit_block_;

  bool is_leader_;
  int cur_slot_;
  bool use_hs_;
};

}  // namespace autobahn
}  // namespace resdb

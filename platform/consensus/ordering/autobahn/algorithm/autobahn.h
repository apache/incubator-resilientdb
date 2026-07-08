#pragma once

#include <deque>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <thread>
#include <tuple>

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
  void ReceiveNewLeader(std::unique_ptr<VoteMsg> msg);
  bool ReceiveVote(std::unique_ptr<Proposal>);
  bool ReceiveProposal(std::unique_ptr<Proposal> proposal);
  bool ReceiveCommit(std::unique_ptr<Proposal> proposal);
  bool ReceivePrepare(std::unique_ptr<Proposal> proposal);
  void RecvAskBlock(std::unique_ptr<BlockACK> ask_block);
  void RecvAskBlockAck(std::unique_ptr<Block> block);
  void RecvAskBlockBatch(std::unique_ptr<BlockBatch> block_batch);
  void RecvAskBlockBatchAck(std::unique_ptr<BlockBatch> block_batch);
  void RecvAskProposal(std::unique_ptr<ProposalQuery> proposal_query);
  void RecvAskProposalAck(std::unique_ptr<ProposalQueryResp> proposals);
  void RecvStateRequest(std::unique_ptr<RecoveryState> recovery_state);
  void RecvStateResponse(std::unique_ptr<RecoveryState> recovery_state);

 private:
  bool IsStop();
  void BroadcastTxn();
  void GenerateBlocks();
  void AsyncDissemination();
  void AsyncConsensus();
  void AsyncPrepare();
  void AsyncCommit();
  void AsyncRecovery();

  bool WaitForResponse(int64_t block_id);
  void BlockDone();
  void PrepareDone(std::unique_ptr<Proposal> vote);
  void CommitDone(std::unique_ptr<Proposal> proposal);
  
  void NotifyView();
  bool WaitForNextView(int view);

  void Prepare(std::unique_ptr<Proposal> vote);
  bool Commit(std::unique_ptr<Proposal> proposal);

  bool IsFastCommit(const Proposal& proposal);

  bool WaitForNextLeader();
  void StartNextLeader(int slot);
  void SendNewLeaderRequest(int slot);
  int NextLeader(int slot);

  void AskBlock(int sender, int block_id);
  void AskBlockBatch(int owner, int start_id, int end_id, int target);
  void AskProposal(int sender, const std::string& hash);
  // Records a (owner, block_id) we committed-by-id but lack the payload
  // for, and triggers a non-owner-first asynchronous pull. Owner may be
  // inside the current partition; commit must keep moving regardless.
  void QueueMissingPayload(int owner, int64_t block_id, int64_t target_block_id);
  void DrainCommittedBlocks(int owner);
  void GarbageCollectAcks();
  RecoveryState BuildRecoveryState();
  void BroadcastRecoveryState();
  bool RecoverProposal(const Proposal& proposal);
  void RequestMissingBlocks(const RecoveryState& recovery_state);
  void MarkProgress();


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
  std::set<std::pair<int,int>> send_, recv_;
  std::map<std::pair<int, int>, int64_t> last_block_req_time_;
  std::map<std::pair<int, int>, int> block_req_round_;
  std::map<std::tuple<int, int, int>, int64_t> last_block_resp_time_;
  // Key: (owner, target, start_id, end_id). Used to coalesce duplicate
  // batch requests issued by overlapping recovery rounds.
  std::map<std::tuple<int, int, int, int>, int64_t> last_batch_req_time_;
  // Set of (owner, block_id) pairs that were committed by ID but whose
  // payload has not yet arrived locally. Filled by Commit() and drained
  // when ReceiveBlock adds the data into pending_blocks_.
  std::mutex missing_mutex_;
  std::set<std::pair<int, int64_t>> missing_payloads_;
  std::map<std::pair<int, int64_t>, int64_t> last_missing_payload_req_time_;

  std::thread block_thread_, dissemi_thread_, consensus_thread_, prepare_thread_,
      commit_thread_, recovery_thread_;

  std::mutex block_mutex_, bc_mutex_, view_mutex_, vote_mutex_, commit_mutex_, leader_mutex_,
      recv_slot_mutex_, b_mutex_;
  std::map<int, std::map<int, SignInfo>> block_ack_;
  std::map<int, std::set<int>> new_leader_ack_;
  std::map<int, std::map<int, std::unique_ptr<Proposal>>> vote_ack_ ;
  std::map<int, std::map<std::string, std::map<int, std::unique_ptr<Proposal>>>> chain_vote_ack_;
  std::set<int> proposed_slots_;
  std::set<std::pair<int, std::string>> formed_qc_;
  std::map<int, std::set<int>>  commit_ack_;
  Stats* global_stats_;
  std::mutex execute_mutex_;
  // Highest block id per owner referenced by committed HotStuff cuts.
  std::map<int, int64_t> committed_cut_block_;
  // Highest block id per owner whose payload has been executed/stat-counted.
  std::map<int, int64_t> commit_block_;
  std::atomic<int> last_committed_slot_ = 0;
  std::atomic<int64_t> last_commit_time_ = 0;
  std::atomic<int64_t> last_progress_time_ = 0;
  std::atomic<int> consecutive_recovery_timeout_count_ = 0;
  std::atomic<int> recovery_target_slot_ = 1;

  bool is_leader_;
  int cur_slot_;
  int max_recv_slot_ = 0;
  bool use_hs_;
  int leader_slot_ = 0;
};

}  // namespace autobahn
}  // namespace resdb

#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <map>
#include <set>
#include <thread>
#include <vector>

#include "platform/common/queue/lock_free_queue.h"
#include "platform/config/resdb_config.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "platform/consensus/ordering/zzy/proto/proposal.pb.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace zzy {

class ZZY : public common::ProtocolBase {
 public:
  ZZY(const ResDBConfig& config, int id, int f, int total_num,
  int batch_size,
      SignatureVerifier* verifier);
  ~ZZY();

  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  bool ReceivePropose(std::unique_ptr<Transaction> txn);
  bool ReceiveProposeBatch(std::unique_ptr<TransactionBatch> batch);
  bool ReceivePrepare(std::unique_ptr<Proposal> proposal);
  bool ReceiveCommit(std::unique_ptr<Proposal> proposal);
  bool ReceiveOrderReply(std::unique_ptr<Proposal> reply);
  bool ReceiveClientCommitACK(std::unique_ptr<Proposal> ack);

  void SetFailFunc(std::function<void(const Transaction& txn)> func);
  void SetSpeculativeExecuteFunc(std::function<int(const Transaction& txn)> func);
  void SetClientCompleteFunc(std::function<void(int64_t local_id)> func);
  void AsyncCommit();

 private:
  bool IsStop();
  int PrimaryId() const;
  int ClientId() const;
  bool IsPrimary() const;
  bool IsClient() const;
  int64_t ParseLocalId(const Transaction& txn) const;

  bool VerifyPrePrepare(const Transaction& txn);
  bool VerifyPrepare(const Proposal& proposal);
  bool VerifyCommitQC(const Proposal& proposal);
  bool VerifyOrderReply(const Proposal& reply);

  bool HasPrePrepare(int bucket, int64_t seq, const std::string& hash);
  bool MarkPrePrepare(int bucket, int64_t seq, const std::string& hash);
  bool TryFastCommit(int64_t seq, const std::string& hash);
  bool TrySlowCommit(int64_t seq, const std::string& hash);
  bool TryPendingCommit(int64_t seq, const std::string& hash);
  void SendPrepareToReplicas(int64_t seq, const std::string& hash, int view);
  void SendOrderReplyToClient(const Transaction& txn);
  void SpeculativeExecute(const Transaction& txn);
  void RebroadcastPendingTransaction(int64_t local_id);
  void SendClientCommit(int64_t local_id);
  bool TryClientCommitLocal(int64_t local_id);
  bool TryClientFastComplete(int64_t local_id);
  bool TryClientSlowComplete(int64_t local_id);
  bool CompleteClientRequest(int64_t local_id);
  void MarkClientPending(int64_t local_id, int64_t create_time);
  void AsyncClientCheck();
  void AsyncSend();

 private:
  std::mutex mutex_[1000];
  std::mutex commit_mutex_[1000];
  std::map<int64_t, std::string> pre_prepare_[1000];
  std::map<int64_t, std::set<int32_t>> received_[1000];
  std::map<std::string, Proposal> pending_commit_[1000];
  std::map<std::string, std::unique_ptr<Transaction>> data_[1000];
  std::set<std::string> committed_[1000];
  std::set<std::string> enqueued_[1000];
  std::set<std::string> speculated_[1000];

  std::mutex client_mutex_;
  std::map<int64_t, std::set<int32_t>> client_reply_senders_;
  std::map<int64_t, std::vector<Proposal>> client_replies_;
  std::map<int64_t, std::set<int32_t>> client_commit_ack_senders_;
  std::map<int64_t, int64_t> client_start_time_;
  std::map<int64_t, int64_t> client_last_retry_time_;
  std::map<int64_t, Transaction> client_pending_txn_;
  std::set<int64_t> client_commit_sent_;
  std::set<int64_t> client_done_;
  std::condition_variable client_cv_;

  std::thread commit_thread_;
  std::thread client_thread_;
  std::thread send_thread_;

  int batch_size_;
  int view_;
  std::atomic<int64_t> seq_;
  bool is_stop_;
  int64_t client_time_limit_;
  const ResDBConfig& config_;
  SignatureVerifier* verifier_;
  Stats* global_stats_;
  std::atomic<int> commit_seq_;

  LockFreeQueue<Transaction> txns_;
  LockFreeQueue<Proposal> commit_q_;
  std::function<void(const Transaction& txn)> fail_func_;
  std::function<int(const Transaction& txn)> speculative_execute_func_;
  std::function<void(int64_t local_id)> client_complete_func_;
};

}  // namespace zzy
}  // namespace resdb

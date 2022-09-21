#pragma once

#include <stdint.h>

#include <map>
#include <memory>
#include <queue>
#include <set>

#include "common/queue/lock_free_queue.h"
#include "config/resdb_config.h"
#include "database/txn_memory_db.h"
#include "execution/transaction_executor_impl.h"
#include "ordering/pbft/lock_free_collector_pool.h"
#include "ordering/pbft/transaction_collector.h"
#include "ordering/pbft/transaction_utils.h"
#include "ordering/status/checkpoint/check_point_info.h"
#include "proto/checkpoint_info.pb.h"
#include "proto/resdb.pb.h"
#include "server/server_comm.h"
#include "statistic/stats.h"

namespace resdb {

class TransactionManager {
 public:
  TransactionManager(const ResDBConfig& config,
                     std::unique_ptr<TransactionExecutorImpl> data_impl,
                     CheckPointInfo* checkpoint_info, SystemInfo* system_info);

  absl::StatusOr<uint64_t> AssignNextSeq();

  int64_t GetCurrentPrimary() const;
  uint64_t GetCurrentView() const;
  uint64_t GetMinExecutCandidateSeq();

  // Add commit messages and return the number of messages have been received.
  // The commit messages only include post(pre-prepare), prepare and commit
  // messages. Messages are handled by state (PREPARE,COMMIT,READY_EXECUTE).

  // If there are enough messages and the state is changed after adding the
  // message, return 1, otherwise return 0. Return -2 if the request is not
  // valid.
  CollectorResultCode AddConsensusMsg(const SignatureInfo& signature,
                                      std::unique_ptr<Request> request);

  // Obtain the request that has been executed from Executor.
  // The messages that have been executed from Executor will save inside
  // Message Manager. Consensus Service can obtain the message then send back
  // to the client proxy.
  std::unique_ptr<BatchClientResponse> GetResponseMsg();

  // Get committed messages with 2f+1 proof in [min_seq, max_seq].
  RequestSet GetRequestSet(uint64_t min_seq, uint64_t max_seq);

  // Get the transactions that have been execuited.
  Request* GetRequest(uint64_t seq);

  // =============  Recovery ==================
  // Commit the request which contains 2f+1 proofs to recover the commit
  // message gap.
  int CommittedRequestWithProof(const RequestWithProof& request);

  // =============  System information ========
  // Obtain the current replica list.
  std::vector<ReplicaInfo> GetReplicas();

  // ============= Checkpoint infomation ======
  // Get the max sequence of the stable checkpoint received from other replicas.
  uint64_t GetStableCheckPointSeq();

  // Get the max sequence of the checkpoint. The max sequence is the largest
  // sequence with which the request has been executed. This sequence may be
  // larger than the sequence in checkpoint data which contains a consistant
  // sequence window of executed requests.
  uint64_t GetMaxCheckPointRequestSeq();

  // Get the checkpoint data which contains the
  CheckPointData GetCheckPointData();

  // Replica State
  int GetReplicaState(ReplicaState* state);
  std::unique_ptr<Context> FetchClientContext(uint64_t seq);

 private:
  bool IsValidMsg(const Request& request);

  bool MayConsensusChangeStatus(int type, int received_count,
                                std::atomic<TransactionStatue>* status);
  void SaveCommittedRequest(const Request& request,
                            TransactionCollector::CollectorDataType* data);
  int SaveCommittedRequest(const RequestWithProof& proof_data);

 private:
  ResDBConfig config_;
  uint64_t next_seq_ = 1;

  LockFreeQueue<BatchClientResponse> queue_;
  std::unique_ptr<TxnMemoryDB> txn_db_;
  SystemInfo* system_info_;
  CheckPointInfo* checkpoint_info_;
  std::unique_ptr<TransactionExecutor> transaction_executor_;
  std::map<uint64_t, std::vector<std::unique_ptr<RequestInfo>>>
      committed_proof_;
  std::map<uint64_t, Request> committed_data_;

  std::mutex data_mutex_, seq_mutex_;
  std::unique_ptr<LockFreeCollectorPool> collector_pool_;

  Stats* global_stats_;
};

}  // namespace resdb

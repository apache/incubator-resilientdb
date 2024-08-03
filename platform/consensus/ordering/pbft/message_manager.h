/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#pragma once

#include <stdint.h>

#include <map>
#include <memory>
#include <queue>
#include <set>

#include "chain/state/chain_state.h"
#include "executor/common/transaction_manager.h"
#include "platform/common/queue/lock_free_queue.h"
#include "platform/config/resdb_config.h"
#include "platform/consensus/ordering/pbft/checkpoint_manager.h"
#include "platform/consensus/ordering/pbft/lock_free_collector_pool.h"
#include "platform/consensus/ordering/pbft/transaction_collector.h"
#include "platform/consensus/ordering/pbft/transaction_utils.h"
#include "platform/networkstrate/server_comm.h"
#include "platform/proto/checkpoint_info.pb.h"
#include "platform/proto/resdb.pb.h"
#include "platform/statistic/stats.h"

namespace resdb {

class MessageManager {
 public:
  MessageManager(const ResDBConfig& config,
                 std::unique_ptr<TransactionManager> transaction_manager,
                 CheckPointManager* checkpoint_manager,
                 SystemInfo* system_info);
  ~MessageManager();

  absl::StatusOr<uint64_t> AssignNextSeq();

  int64_t GetCurrentPrimary() const;
  uint64_t GetMinExecutCandidateSeq();
  void SetNextSeq(uint64_t seq);
  int64_t GetNextSeq();

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
  std::unique_ptr<BatchUserResponse> GetResponseMsg();

  // Get committed messages with 2f+1 proof in [min_seq, max_seq].
  RequestSet GetRequestSet(uint64_t min_seq, uint64_t max_seq);

  // Get the transactions that have been execuited.
  Request* GetRequest(uint64_t seq);

  // Get the proof info containing the request and signatures
  // if the request has been prepared, having received 2f+1
  // pre-prepare messages.
  std::vector<RequestInfo> GetPreparedProof(uint64_t seq);
  TransactionStatue GetTransactionState(uint64_t seq);

  // =============  System information ========
  // Obtain the current replica list.
  std::vector<ReplicaInfo> GetReplicas();

  uint64_t GetCurrentView() const;

  // Replica State
  int GetReplicaState(ReplicaState* state);
  std::unique_ptr<Context> FetchClientContext(uint64_t seq);

  Storage* GetStorage();

  void SetLastCommittedTime(uint64_t proxy_id);

  uint64_t GetLastCommittedTime(uint64_t proxy_id);

  bool IsPreapared(uint64_t seq);

  uint64_t GetHighestPreparedSeq();

  void SetHighestPreparedSeq(uint64_t seq);

  void SetDuplicateManager(DuplicateManager* manager);

  void SendResponse(std::unique_ptr<Request> request);

  LockFreeCollectorPool* GetCollectorPool();

 private:
  bool IsValidMsg(const Request& request);

  bool MayConsensusChangeStatus(int type, int received_count,
                                std::atomic<TransactionStatue>* status,
                                bool force);

 private:
  ResDBConfig config_;
  uint64_t next_seq_ = 1;

  LockFreeQueue<BatchUserResponse> queue_;
  ChainState* txn_db_;
  SystemInfo* system_info_;
  CheckPointManager* checkpoint_manager_;
  std::map<uint64_t, std::vector<std::unique_ptr<RequestInfo>>>
      committed_proof_;
  std::map<uint64_t, Request> committed_data_;

  std::mutex data_mutex_, seq_mutex_;
  std::unique_ptr<TransactionExecutor> transaction_executor_;
  std::unique_ptr<LockFreeCollectorPool> collector_pool_;

  Stats* global_stats_;

  std::mutex lct_lock_;
  std::map<uint64_t, uint64_t> last_committed_time_;
};

}  // namespace resdb

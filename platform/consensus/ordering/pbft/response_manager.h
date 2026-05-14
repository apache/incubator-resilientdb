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
#include <semaphore.h>

#include "platform/config/resdb_config.h"
#include "platform/consensus/ordering/pbft/lock_free_collector_pool.h"
#include "platform/consensus/ordering/pbft/transaction_utils.h"
#include "platform/networkstrate/replica_communicator.h"
#include "platform/statistic/stats.h"

namespace resdb {

class ResponseClientTimeout {
 public:
  ResponseClientTimeout(std::string hash_, uint64_t time_);
  ResponseClientTimeout(const ResponseClientTimeout& other);
  bool operator<(const ResponseClientTimeout& other) const;

  std::string hash;
  uint64_t timeout_time;
};

class ResponseManager {
 public:

  // ShardedResponseManager extends ResponseManager to provide shard-aware response handling for a sharded system.
  // It uses the ShardRouter to determine the appropriate shard for each request and manages routing information for local nodes.
  // Routing rotates through shard leaders and mom-leader shard metadata is limited to local replicas for pbft.
  ResponseManager(const ResDBConfig& config,
                  ReplicaCommunicator* replica_communicator,
                  SystemInfo* system_info, SignatureVerifier* verifier,
                  bool defer_user_req_thread = false);

  // Virtual destructor to allow for proper cleanup in derived classes like ShardedResponseManager.
  virtual ~ResponseManager();

  int AddContextList(std::vector<std::unique_ptr<Context>> context,
                     uint64_t id);
  std::vector<std::unique_ptr<Context>> FetchContextList(uint64_t id);

  int NewUserRequest(std::unique_ptr<Context> context,
                     std::unique_ptr<Request> user_request);

  int ProcessResponseMsg(std::unique_ptr<Context> context,
                         std::unique_ptr<Request> request);

 protected:
  // Add response messages which will be sent back to the caller
  // if there are f+1 same messages.
  CollectorResultCode AddResponseMsg(
      const SignatureInfo& signature, std::unique_ptr<Request> request,
      std::function<void(const Request&,
                         const TransactionCollector::CollectorDataType*)>
          call_back);
  void SendResponseToClient(const BatchUserResponse& batch_response);

  struct QueueItem {
    std::unique_ptr<Context> context;
    std::unique_ptr<Request> user_request;
  };

  // Check if the consensus status can be changed based on the type of message, local_id, number of messages received, and current status.
  // Used in sharded response manager to determine if consensus can progress based on responses received from the relevant shard.
  virtual bool MayConsensusChangeStatus(
      int type, uint64_t local_id, int received_count,
      std::atomic<TransactionStatue>* status);
  // ShardedResponseManager override to determine the target replica for a request based on its local_id and shard metadata.
  virtual int DoBatch(const std::vector<std::unique_ptr<QueueItem>>& batch_req);
  int BatchProposeMsg();
  void StartUserRequestThread();
  // Check if the user request thread should be running, which is true if the response manager is initialized and not in deferred initialization mode (e.g., global 3pc mode is enabled for sharded systems).
  bool ShouldRunUserRequestThread() const;
  // ShardedResponseManager override to determine the target replica for a request based on its local_id and shard metadata.
  virtual int64_t GetRequestTarget(Request* request, uint64_t local_id);
  // ShardedResponseManager override to resend requests to the appropriate shard leader on timeout.
  virtual void ResendRequestOnTimeout(const Request& request);
  int GetPrimary();

  void AddWaitingResponseRequest(std::unique_ptr<Request> request);
  void RemoveWaitingResponseRequest(const std::string& hash);
  bool CheckTimeOut(std::string hash);
  void ResponseTimer(std::string hash);
  void MonitoringClientTimeOut();
  std::unique_ptr<Request> GetTimeOutRequest(std::string hash);

 protected:
  ResDBConfig config_;
  ReplicaCommunicator* replica_communicator_;
  std::unique_ptr<LockFreeCollectorPool> collector_pool_, context_pool_;
  LockFreeQueue<QueueItem> batch_queue_;
  std::thread user_req_thread_;
  std::atomic<bool> stop_;
  std::atomic<uint64_t> local_id_;
  Stats* global_stats_;
  SystemInfo* system_info_;
  std::atomic<int> send_num_;
  SignatureVerifier* verifier_;

  std::thread checking_timeout_thread_;
  std::map<std::string, std::unique_ptr<Request>> waiting_response_batches_;
  std::priority_queue<ResponseClientTimeout> client_timeout_min_heap_;
  std::mutex pm_lock_;
  uint64_t timeout_length_;
  sem_t request_sent_signal_;
  uint64_t highest_seq_;
  uint64_t highest_seq_primary_id_;
};

}  // namespace resdb

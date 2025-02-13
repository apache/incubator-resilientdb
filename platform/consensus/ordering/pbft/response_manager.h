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
  ResponseManager(const ResDBConfig& config,
                  ReplicaCommunicator* replica_communicator,
                  SystemInfo* system_info, SignatureVerifier* verifier);

  ~ResponseManager();

  int AddContextList(std::vector<std::unique_ptr<Context>> context,
                     uint64_t id);
  std::vector<std::unique_ptr<Context>> FetchContextList(uint64_t id);

  int NewUserRequest(std::unique_ptr<Context> context,
                     std::unique_ptr<Request> user_request);

  int ProcessResponseMsg(std::unique_ptr<Context> context,
                         std::unique_ptr<Request> request);

 private:
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
  bool MayConsensusChangeStatus(int type, int received_count,
                                std::atomic<TransactionStatue>* status);
  int DoBatch(const std::vector<std::unique_ptr<QueueItem>>& batch_req);
  int BatchProposeMsg();
  int GetPrimary();

  void AddWaitingResponseRequest(std::unique_ptr<Request> request);
  void RemoveWaitingResponseRequest(const std::string& hash);
  bool CheckTimeOut(std::string hash);
  void ResponseTimer(std::string hash);
  void MonitoringClientTimeOut();
  std::unique_ptr<Request> GetTimeOutRequest(std::string hash);

 private:
  ResDBConfig config_;
  ReplicaCommunicator* replica_communicator_;
  std::unique_ptr<LockFreeCollectorPool> collector_pool_, context_pool_;
  LockFreeQueue<QueueItem> batch_queue_;
  std::thread user_req_thread_;
  std::atomic<bool> stop_;
  uint64_t local_id_ = 0;
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

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

#include "platform/config/resdb_config.h"
#include "platform/consensus/ordering/common/framework/transaction_utils.h"
#include "platform/networkstrate/replica_communicator.h"
#include "platform/networkstrate/server_comm.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace common {

class ResponseManager {
 public:
  ResponseManager(const ResDBConfig& config,
                  ReplicaCommunicator* replica_communicator,
                  SignatureVerifier* verifier);

  ~ResponseManager();

  std::vector<std::unique_ptr<Context>> FetchContextList(uint64_t id);

  int NewUserRequest(std::unique_ptr<Context> context,
                     std::unique_ptr<Request> user_request);

  int ProcessResponseMsg(std::unique_ptr<Context> context,
                         std::unique_ptr<Request> request);

 private:
  // Add response messages which will be sent back to the caller
  // if there are f+1 same messages.
  comm::CollectorResultCode AddResponseMsg(
      std::unique_ptr<Request> request,
      std::function<void(const Request&)> call_back);
  void SendResponseToClient(const BatchUserResponse& batch_response);

  struct QueueItem {
    std::unique_ptr<Context> context;
    std::unique_ptr<Request> user_request;
  };
  int DoBatch(const std::vector<std::unique_ptr<QueueItem>>& batch_req);
  int BatchProposeMsg();
  int GetPrimary();

 private:
  ResDBConfig config_;
  ReplicaCommunicator* replica_communicator_;
  LockFreeQueue<QueueItem> batch_queue_;
  std::thread user_req_thread_;
  std::atomic<bool> stop_;
  uint64_t local_id_ = 0;
  Stats* global_stats_;
  std::atomic<int> send_num_;
  SignatureVerifier* verifier_;
  static const int response_set_size_ = 6000000;
  std::map<int64_t, int> response_[response_set_size_];
  std::mutex response_lock_[response_set_size_];
};

}  // namespace common
}  // namespace resdb

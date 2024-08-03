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
#include "executor/common/transaction_manager.h"
#include "platform/config/resdb_config.h"
#include "platform/consensus/execution/system_info.h"
#include "platform/networkstrate/replica_communicator.h"

namespace resdb {

class GeoTransactionExecutor : public TransactionManager {
 public:
  GeoTransactionExecutor(
      const ResDBConfig& config, std::unique_ptr<SystemInfo> system_info,
      std::unique_ptr<ReplicaCommunicator> replica_communicator,
      std::unique_ptr<TransactionManager> local_transaction_manager);

  virtual ~GeoTransactionExecutor();
  virtual std::unique_ptr<BatchUserResponse> ExecuteBatch(
      const BatchUserRequest& request) override;
  bool IsStop();

 private:
  void SendGeoMessages();
  void SendBatchGeoMessage(
      const std::vector<std::unique_ptr<Request>>& requests);

 protected:
  ResDBConfig config_;
  std::unique_ptr<SystemInfo> system_info_;
  std::unique_ptr<ReplicaCommunicator> replica_communicator_;
  std::unique_ptr<TransactionManager> local_transaction_manager_ = nullptr;
  std::thread geo_thread_;
  size_t batch_size_ = 50;
  LockFreeQueue<Request> queue_;
  std::atomic<bool> is_stop_;

  std::vector<std::unique_ptr<BatchUserRequest>> messages_;
};

}  // namespace resdb

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
#include <mutex>
#include <queue>
#include <unordered_map>

#include "executor/common/transaction_manager.h"
#include "platform/common/queue/lock_free_queue.h"
#include "platform/config/resdb_config.h"
#include "platform/statistic/stats.h"

namespace resdb {

class GeoGlobalExecutor {
 public:
  GeoGlobalExecutor(
      std::unique_ptr<TransactionManager> global_transaction_manager,
      const ResDBConfig& config);
  virtual ~GeoGlobalExecutor();
  virtual void Execute(std::unique_ptr<Request> request);
  virtual int OrderGeoRequest(std::unique_ptr<Request> request);

  void Stop();

  std::unique_ptr<BatchUserResponse> GetResponseMsg();

 private:
  using ExecuteMap =
      std::map<std::pair<uint64_t, int>, std::unique_ptr<Request>>;

  bool IsStop();
  void OrderRound();
  std::unique_ptr<Request> GetNextMap();
  void AddData();

 protected:
  std::unique_ptr<TransactionManager> global_transaction_manager_;
  Stats* global_stats_;
  std::thread execute_round_thread_, order_thread_;
  ExecuteMap execute_map_;
  uint64_t next_seq_ = 1;
  int next_region_ = 1;
  size_t region_size_;
  ResDBConfig config_;
  std::atomic<bool> is_stop_;
  std::mutex mutex_;
  std::condition_variable cv_;
  LockFreeQueue<Request> order_queue_;
  LockFreeQueue<BatchUserResponse> resp_queue_;
  int my_region_;
};
}  // namespace resdb

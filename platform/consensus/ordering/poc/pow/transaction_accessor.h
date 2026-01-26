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

#include <thread>

#include "interface/common/resdb_txn_accessor.h"
#include "platform/common/queue/lock_free_queue.h"
#include "platform/config/resdb_poc_config.h"
#include "platform/consensus/ordering/poc/proto/pow.pb.h"
#include "platform/statistic/stats.h"

namespace resdb {

// TransactionAccessor obtains the transaction from BFT cluster.
// It broadcasts the request to all the replicas in BFT cluster
// and waits for 2f+1 same response, but only return one to the
// caller.
class TransactionAccessor {
 public:
  // For test, it is started by the tester.
  TransactionAccessor(const ResDBPoCConfig& config, bool auto_start = true);
  virtual ~TransactionAccessor();

  // consume the transaction between [seq, seq+batch_num-1]
  virtual std::unique_ptr<BatchClientTransactions> ConsumeTransactions(
      uint64_t seq);

  // For test.
  void Start();

 protected:
  void TransactionFetching();
  virtual std::unique_ptr<ResDBTxnAccessor> GetResDBTxnAccessor();

 private:
  ResDBPoCConfig config_;
  std::atomic<bool> stop_;
  std::thread fetching_thread_;
  std::atomic<uint64_t> max_received_seq_;
  LockFreeQueue<ClientTransactions> queue_;
  uint64_t next_consume_ = 0;

  std::condition_variable cv_;
  std::mutex mutex_;
  uint64_t last_time_ = 0;

  PrometheusHandler* prometheus_handler_;
};

}  // namespace resdb

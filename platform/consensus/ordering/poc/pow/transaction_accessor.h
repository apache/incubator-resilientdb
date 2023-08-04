/*
 * Copyright (c) 2019-2022 ExpoLab, UC Davis
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include <thread>

#include "interface/common/resdb_txn_accessor.h"
#include "platform/common/queue/lock_free_queue.h"
#include "platform/config/resdb_poc_config.h"
#include "platform/consensus/ordering/poc/proto/pow.pb.h"

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
};

}  // namespace resdb

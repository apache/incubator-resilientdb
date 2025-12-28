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
  virtual std::unique_ptr<BatchClientTransactions> ConsumeTransactions(uint64_t seq);

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

  PrometheusHandler * prometheus_handler_;
};

}  // namespace resdb

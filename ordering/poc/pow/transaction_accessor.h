#pragma once

#include <thread>

#include "client/resdb_txn_client.h"
#include "common/queue/lock_free_queue.h"
#include "config/resdb_poc_config.h"
#include "ordering/poc/proto/pow.pb.h"

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
  std::unique_ptr<BatchClientTransactions> ConsumeTransactions(uint64_t seq);

  // For test.
  void Start();

 protected:
  void TransactionFetching();
  virtual std::unique_ptr<ResDBTxnClient> GetResDBTxnClient();

 private:
  ResDBPoCConfig config_;
  std::atomic<bool> stop_;
  std::thread fetching_thread_;
  std::atomic<uint64_t> max_received_seq_;
  LockFreeQueue<ClientTransactions> queue_;
  uint64_t next_consume_ = 0;
};

}  // namespace resdb

#pragma once

#include "common/queue/batch_queue.h"
#include "config/resdb_config.h"
#include "ordering/pbft/response_manager.h"
#include "ordering/pbft/transaction_manager.h"
#include "server/resdb_replica_client.h"
#include "statistic/stats.h"

namespace resdb {

class Commitment {
 public:
  Commitment(const ResDBConfig& config, TransactionManager* transaction_manager,
             ResDBReplicaClient* replica_client, SignatureVerifier* verifier);
  virtual ~Commitment();

  virtual int ProcessNewRequest(std::unique_ptr<Context> context,
                                std::unique_ptr<Request> client_request);

  virtual int ProcessProposeMsg(std::unique_ptr<Context> context,
                                std::unique_ptr<Request> request);
  virtual int ProcessPrepareMsg(std::unique_ptr<Context> context,
                                std::unique_ptr<Request> request);
  virtual int ProcessCommitMsg(std::unique_ptr<Context> context,
                               std::unique_ptr<Request> request);

 protected:
  virtual int PostProcessExecutedMsg();

 protected:
  ResDBConfig config_;
  TransactionManager* transaction_manager_;
  std::thread executed_thread_;
  std::atomic<bool> stop_;
  ResDBReplicaClient* replica_client_;

  SignatureVerifier* verifier_;
  Stats* global_stats_;
};

}  // namespace resdb

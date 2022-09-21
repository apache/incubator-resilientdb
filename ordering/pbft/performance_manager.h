#pragma once

#include <future>

#include "config/resdb_config.h"
#include "ordering/pbft/lock_free_collector_pool.h"
#include "ordering/pbft/transaction_utils.h"
#include "server/resdb_replica_client.h"
#include "statistic/stats.h"

namespace resdb {

class PerformanceManager {
 public:
  PerformanceManager(const ResDBConfig& config,
                     ResDBReplicaClient* replica_client,
                     SignatureVerifier* verifier);

  ~PerformanceManager();

  int StartEval();

  int ProcessResponseMsg(std::unique_ptr<Context> context,
                         std::unique_ptr<Request> request);
  void SetDataFunc(std::function<std::string()> func);

 private:
  // Add response messages which will be sent back to the client
  // if there are f+1 same messages.
  CollectorResultCode AddResponseMsg(
      const SignatureInfo& signature, std::unique_ptr<Request> request,
      std::function<void(const Request&,
                         const TransactionCollector::CollectorDataType*)>
          call_back);
  void SendResponseToClient(const BatchClientResponse& batch_response);

  struct QueueItem {
    std::unique_ptr<Context> context;
    std::unique_ptr<Request> client_request;
  };
  bool MayConsensusChangeStatus(int type, int received_count,
                                std::atomic<TransactionStatue>* status);
  int DoBatch(const std::vector<std::unique_ptr<QueueItem>>& batch_req);
  int BatchProposeMsg();
  int GetPrimary();
  std::unique_ptr<Request> GenerateClientRequest();

 private:
  ResDBConfig config_;
  ResDBReplicaClient* replica_client_;
  std::unique_ptr<LockFreeCollectorPool> collector_pool_, context_pool_;
  LockFreeQueue<QueueItem> batch_queue_;
  std::thread client_req_thread_[16];
  std::atomic<bool> stop_;
  uint64_t local_id_ = 0;
  Stats* global_stats_;
  std::atomic<int> send_num_;
  std::mutex mutex_;
  std::atomic<int> total_num_;
  SignatureVerifier* verifier_;
  SignatureInfo sig_;
  std::function<std::string()> data_func_;
  std::future<bool> eval_ready_future_;
  std::promise<bool> eval_ready_promise_;
  std::atomic<bool> eval_started_;
};

}  // namespace resdb

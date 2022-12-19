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

#include "config/resdb_config.h"
#include "ordering/pbft/lock_free_collector_pool.h"
#include "ordering/pbft/transaction_utils.h"
#include "server/resdb_replica_client.h"
#include "statistic/stats.h"

namespace resdb {

class ResponseManager {
 public:
  ResponseManager(const ResDBConfig& config, ResDBReplicaClient* replica_client,
                  SystemInfo* system_info, SignatureVerifier* verifier);

  ~ResponseManager();

  int AddContextList(std::vector<std::unique_ptr<Context>> context,
                     uint64_t id);
  std::vector<std::unique_ptr<Context>> FetchContextList(uint64_t id);

  int NewClientRequest(std::unique_ptr<Context> context,
                       std::unique_ptr<Request> client_request);

  int ProcessResponseMsg(std::unique_ptr<Context> context,
                         std::unique_ptr<Request> request);

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

 private:
  ResDBConfig config_;
  ResDBReplicaClient* replica_client_;
  std::unique_ptr<LockFreeCollectorPool> collector_pool_, context_pool_;
  LockFreeQueue<QueueItem> batch_queue_;
  std::thread client_req_thread_;
  std::atomic<bool> stop_;
  uint64_t local_id_ = 0;
  Stats* global_stats_;
  SystemInfo* system_info_;
  std::atomic<int> send_num_;
  SignatureVerifier* verifier_;
};

}  // namespace resdb

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

#include "common/data_comm/data_comm.h"
#include "config/resdb_config.h"
#include "execution/system_info.h"
#include "ordering/pbft/transaction_utils.h"
#include "server/resdb_replica_client.h"
#include "server/server_comm.h"
#include "statistic/stats.h"

namespace resdb {

class PerformanceManager {
 public:
  PerformanceManager(const ResDBConfig& config,
                     ResDBReplicaClient* replica_client,
                     SystemInfo* system_info, SignatureVerifier* verifier);

  ~PerformanceManager();

  int StartEval();

  int ProcessResponseMsg(std::unique_ptr<Context> context,
                         std::unique_ptr<Request> request);
  void SetDataFunc(std::function<std::string()> func);

 private:
  // Add response messages which will be sent back to the client
  // if there are f+1 same messages.
  CollectorResultCode AddResponseMsg(
      std::unique_ptr<Request> request,
      std::function<void(const Request&)> call_back);
  void SendResponseToClient(const BatchClientResponse& batch_response);

  struct QueueItem {
    std::unique_ptr<Context> context;
    std::unique_ptr<Request> client_request;
  };
  int DoBatch(const std::vector<std::unique_ptr<QueueItem>>& batch_req);
  int BatchProposeMsg();
  int GetPrimary();
  std::unique_ptr<Request> GenerateClientRequest();

 private:
  ResDBConfig config_;
  ResDBReplicaClient* replica_client_;
  LockFreeQueue<QueueItem> batch_queue_;
  std::thread client_req_thread_[16];
  std::atomic<bool> stop_;
  uint64_t local_id_ = 0;
  Stats* global_stats_;
  std::atomic<int> send_num_;
  std::mutex mutex_;
  std::atomic<int> total_num_;
  SystemInfo* system_info_;
  SignatureVerifier* verifier_;
  SignatureInfo sig_;
  std::function<std::string()> data_func_;
  std::future<bool> eval_ready_future_;
  std::promise<bool> eval_ready_promise_;
  std::atomic<bool> eval_started_;
  std::atomic<int> fail_num_;
  static const int response_set_size_ = 6000000;
  std::map<int64_t, int> response_[response_set_size_];
  std::mutex response_lock_[response_set_size_];
};

}  // namespace resdb

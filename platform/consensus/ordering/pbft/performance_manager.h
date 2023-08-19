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

#include <future>
#include <queue>
#include <semaphore.h>

#include "platform/config/resdb_config.h"
#include "platform/consensus/ordering/pbft/lock_free_collector_pool.h"
#include "platform/consensus/ordering/pbft/transaction_utils.h"
#include "platform/networkstrate/replica_communicator.h"
#include "platform/statistic/stats.h"

namespace resdb {

 class PerformanceClientTimeout{
 public:
     std::string hash;
     uint64_t timeout_time;

     PerformanceClientTimeout(std::string hash_, uint64_t time_){
         this->hash = hash_;
         this->timeout_time = time_;
     }

     PerformanceClientTimeout(const PerformanceClientTimeout& other){
         this->hash = other.hash;
         this->timeout_time = other.timeout_time;
     }

     bool operator<(const PerformanceClientTimeout& other) const{
         return timeout_time > other.timeout_time;
     }
 };


class PerformanceManager {
 public:
  PerformanceManager(const ResDBConfig& config,
                     ReplicaCommunicator* replica_communicator,
                     SystemInfo* system_info, SignatureVerifier* verifier);

  ~PerformanceManager();

  int StartEval();

  int ProcessResponseMsg(std::unique_ptr<Context> context,
                         std::unique_ptr<Request> request);
  void SetDataFunc(std::function<std::string()> func);

 private:
  // Add response messages which will be sent back to the caller
  // if there are f+1 same messages.
  CollectorResultCode AddResponseMsg(
      const SignatureInfo& signature, std::unique_ptr<Request> request,
      std::function<void(const Request&,
                         const TransactionCollector::CollectorDataType*)>
          call_back);
  void SendResponseToClient(const BatchUserResponse& batch_response);

  struct QueueItem {
    std::unique_ptr<Context> context;
    std::unique_ptr<Request> user_request;
  };
  bool MayConsensusChangeStatus(int type, int received_count,
                                std::atomic<TransactionStatue>* status);
  int DoBatch(const std::vector<std::unique_ptr<QueueItem>>& batch_req);
  int BatchProposeMsg();
  int GetPrimary();
  std::unique_ptr<Request> GenerateUserRequest();

  void AddWaitingResponseRequest(std::unique_ptr<Request> request);
  void RemoveWaitingResponseRequest(std::string hash);
  bool CheckTimeOut(std::string hash);
  void ResponseTimer(std::string hash);
  void MonitoringClientTimeOut();
  std::unique_ptr<Request> GetTimeOutRequest(std::string hash);

 private:
  ResDBConfig config_;
  ReplicaCommunicator* replica_communicator_;
  std::unique_ptr<LockFreeCollectorPool> collector_pool_, context_pool_;
  LockFreeQueue<QueueItem> batch_queue_;
  std::thread user_req_thread_[16];
  std::atomic<bool> stop_;
  uint64_t local_id_ = 0;
  Stats* global_stats_;
  std::vector<int> send_num_;
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

  std::thread checking_timeout_thread;
  std::map<std::string, std::unique_ptr<Request>> waiting_response_batches;
  std::priority_queue<PerformanceClientTimeout> client_timeout_min_heap;
  std::mutex pm_lock;
  uint64_t timeout_length;
  sem_t request_sent_signal;
  uint64_t highest_seq;
  uint64_t highest_seq_primary_id;
  std::vector<std::string> hashes;
};

}  // namespace resdb

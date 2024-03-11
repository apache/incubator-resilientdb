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

#include "platform/config/resdb_config.h"
#include "platform/consensus/ordering/common/framework/transaction_utils.h"
#include "platform/networkstrate/replica_communicator.h"
#include "platform/networkstrate/server_comm.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace common {

class PerformanceManager {
 public:
  PerformanceManager(const ResDBConfig& config,
                     ReplicaCommunicator* replica_communicator,
                     SignatureVerifier* verifier);

  virtual ~PerformanceManager();

  int StartEval();

  int ProcessResponseMsg(std::unique_ptr<Context> context,
                         std::unique_ptr<Request> request);
  void SetDataFunc(std::function<std::string()> func);

  protected:
  virtual void SendMessage(const Request& request);

 private:
  // Add response messages which will be sent back to the caller
  // if there are f+1 same messages.
  comm::CollectorResultCode AddResponseMsg(
      std::unique_ptr<Request> request,
      std::function<void(std::unique_ptr<BatchUserResponse>)> call_back);
  void SendResponseToClient(const BatchUserResponse& batch_response);

  struct QueueItem {
    std::unique_ptr<Context> context;
    std::unique_ptr<Request> user_request;
  };
  int DoBatch(const std::vector<std::unique_ptr<QueueItem>>& batch_req);
  int BatchProposeMsg();
  int GetPrimary();
  std::unique_ptr<Request> GenerateUserRequest();

 protected:
  ResDBConfig config_;
  ReplicaCommunicator* replica_communicator_;

 private:
  LockFreeQueue<QueueItem> batch_queue_;
  std::thread user_req_thread_[16];
  std::atomic<bool> stop_;
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
  std::atomic<int> fail_num_;
  static const int response_set_size_ = 6000000;
  std::map<int64_t, int> response_[response_set_size_];
  std::mutex response_lock_[response_set_size_];
  int replica_num_;
  int id_;
  int primary_;
  std::atomic<int> local_id_;
  std::atomic<int> sum_;
};

}  // namespace common
}  // namespace resdb

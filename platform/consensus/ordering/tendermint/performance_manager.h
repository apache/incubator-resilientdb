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
#include "platform/consensus/ordering/tendermint/proto/tendermint.pb.h"
#include "platform/networkstrate/replica_communicator.h"
#include "platform/networkstrate/server_comm.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace tendermint {

class PerformanceManager {
 public:
  PerformanceManager(const ResDBConfig& config,
                     ReplicaCommunicator* replica_communicator);

  ~PerformanceManager();

  int StartEval();

  int NewUserRequest(std::unique_ptr<Context> context,
                     std::unique_ptr<Request> user_request);

  int ProcessResponseMsg(std::unique_ptr<Context> context,
                         std::unique_ptr<Request> request);

  void SetDataFunc(std::function<std::string()> func);

 private:
  void SendResponseToClient(const BatchUserResponse& batch_response);
  struct QueueItem {
    std::unique_ptr<Context> context;
    std::unique_ptr<Request> user_request;
  };
  int DoBatch(const std::vector<std::unique_ptr<QueueItem>>& batch_req);
  int BatchProposeMsg();
  int GetPrimary();

  std::vector<std::unique_ptr<Context>> FetchContextList(uint64_t id);

  std::unique_ptr<Request> NewRequest(const TendermintRequest& request);
  int SendMessage(const TendermintRequest& tendermint_request);

  int AddContextList(std::vector<std::unique_ptr<Context>> context_list,
                     uint64_t id);
  void ClearContextList(uint64_t id);
  bool IsExistContextList(uint64_t id);

  std::unique_ptr<Request> GenerateUserRequest();

 private:
  ResDBConfig config_;
  ReplicaCommunicator* replica_communicator_;
  LockFreeQueue<QueueItem> batch_queue_;
  std::atomic<bool> stop_;
  std::atomic<uint64_t> local_id_ = 0;
  std::atomic<int> send_num_;
  uint32_t primary_id_;
  std::map<uint64_t, std::vector<std::unique_ptr<Context>>> client_list_
      GUARDED_BY(client_list_mutex_);
  std::map<uint64_t, std::set<uint32_t>> response_nodes_list_;
  std::mutex client_list_mutex_, response_mutex_;
  std::function<std::string()> data_func_;
  std::future<bool> eval_ready_future_;
  std::promise<bool> eval_ready_promise_;
  std::atomic<bool> eval_started_;
  std::atomic<int> total_num_;
  std::thread user_req_thread_[16];
  Stats* global_stats_;
};

}  // namespace tendermint
}  // namespace resdb

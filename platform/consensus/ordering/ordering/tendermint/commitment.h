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

#include "platform/config/resdb_config.h"
#include "platform/consensus/ordering/tendermint/message_manager.h"
#include "platform/networkstrate/replica_communicator.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace tendermint {

class MonitorTimer;
class Commitment {
 public:
  Commitment(const ResDBConfig& config, MessageManager* message_manager,
             ReplicaCommunicator* replica_client, SignatureVerifier* verifier);
  virtual ~Commitment();

  int Process(std::unique_ptr<TendermintRequest> request);
  void Init();
  void SetNodeId(const int32_t id);
  int ProcessNewRequest(std::unique_ptr<TendermintRequest> request);

 protected:
  int StartNewRound();

  virtual int PostProcessExecutedMsg();
  std::unique_ptr<Request> NewRequest(
      const TendermintRequest& request,
      TendermintRequest::Type type = TendermintRequest::TYPE_NONE);
  int ProcessNewView(int64_t view_number);
  std::unique_ptr<TendermintRequest> GetClientRequest();
  int32_t PrimaryId(int64_t view_num);
  void Reset();

  void StartTimer();

  int ProcessRequest(std::unique_ptr<TendermintRequest> request);
  int CheckStatus(int64_t h, int r, const std::string& hash);
  void CheckNextComplete();

  std::unique_ptr<TendermintRequest> FindPropose(int64_t h, int r);
  int FindPrevote(int64_t h, int r, const std::string& hash, bool all = false);
  int FindPrecommit(int64_t h, int r, const std::string& hash,
                    bool all = false);

  void AddTimer(const std::string& name, TendermintRequest::Type next_type);

 protected:
  ResDBConfig config_;
  MessageManager* message_manager_;
  std::thread executed_thread_, round_thread_;
  std::atomic<bool> stop_;
  ReplicaCommunicator* replica_client_;
  SignatureVerifier* verifier_;
  std::atomic<int> current_round_;
  std::atomic<int64_t> current_height_;
  std::atomic<int> valid_round_;
  std::unique_ptr<TendermintRequest> valid_request_;
  std::atomic<TendermintRequest::Type> current_step_;

  std::atomic<int> lock_round_;
  std::unique_ptr<TendermintRequest> lock_request_;

  int32_t id_;

  LockFreeQueue<TendermintRequest> request_list_;
  std::mutex mutex_;
  Stats* global_stats_ = nullptr;

  std::map<std::pair<int64_t, int>, std::unique_ptr<TendermintRequest>>
      proposal_;
  // hash -> vector<request>
  typedef std::map<std::string, std::vector<std::unique_ptr<TendermintRequest>>>
      DataType;
  std::map<std::pair<int64_t, int>, DataType> prevote_, precommit_;
  std::map<std::pair<int64_t, int>,
           std::map<std::pair<int, std::string>, std::set<int>>>
      sender_;
  std::map<std::pair<int64_t, int>, std::set<int>> outdated_;

  std::vector<std::unique_ptr<MonitorTimer>> monitor_timers_;
};

}  // namespace tendermint
}  // namespace resdb

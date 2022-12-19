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

#include <chrono>
#include <future>

#include "statistic/prometheus_handler.h"

namespace resdb {

class Stats {
 public:
  static Stats* GetGlobalStats(int sleep_seconds = 5);

  void Stop();

  void AddLatency(uint64_t run_time);

  void Monitor();
  void MonitorGlobal();

  void IncSocketRecv();

  void IncClientCall();

  void IncClientRequest();
  void IncPropose();
  void IncPrepare();
  void IncCommit();
  void IncPendingExecute();
  void IncExecute();
  void IncExecuteDone();

  void BroadCastMsg();
  void SendBroadCastMsg(uint32_t num);
  void SendBroadCastMsgPerRep();
  void SeqFail();
  void IncTotalRequest(uint32_t num);
  void IncTotalGeoRequest(uint32_t num);
  void IncGeoRequest();

  void SeqGap(uint64_t seq_gap);
  // Network in->worker
  void ServerCall();
  void ServerProcess();
  void SetPrometheus(const std::string& prometheus_address);

 protected:
  Stats(int sleep_time = 5);
  ~Stats();

 private:
  std::string monitor_port_ = "default";
  std::string name_;
  std::atomic<int> num_call_, run_call_;
  std::atomic<uint64_t> last_time_, run_time_, run_call_time_;
  std::thread thread_;
  std::atomic<bool> begin_;
  std::atomic<bool> stop_;
  std::condition_variable cv_;
  std::mutex mutex_;

  std::thread global_thread_;
  std::atomic<uint64_t> num_client_req_, num_propose_, num_prepare_,
      num_commit_, pending_execute_, execute_, execute_done_;
  std::atomic<uint64_t> client_call_, socket_recv_;
  std::atomic<uint64_t> broad_cast_msg_, send_broad_cast_msg_,
      send_broad_cast_msg_per_rep_;
  std::atomic<uint64_t> seq_fail_;
  std::atomic<uint64_t> server_call_, server_process_;
  std::atomic<uint64_t> run_req_num_;
  std::atomic<uint64_t> run_req_run_time_;
  std::atomic<uint64_t> seq_gap_;
  std::atomic<uint64_t> total_request_, total_geo_request_, geo_request_;
  int monitor_sleep_time_ = 5;  // default 5s.

  std::unique_ptr<PrometheusHandler> prometheus_;
};

}  // namespace resdb

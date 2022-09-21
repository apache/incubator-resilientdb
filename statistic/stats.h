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
  std::atomic<uint64_t> total_request_;
  int monitor_sleep_time_ = 5;  // default 5s.

  std::unique_ptr<PrometheusHandler> prometheus_;
};

}  // namespace resdb

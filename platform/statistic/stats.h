/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#pragma once

#include <crow.h>

#include <chrono>
#include <future>
#include <nlohmann/json.hpp>

#include "boost/asio.hpp"
#include "boost/beast.hpp"
#include "platform/common/network/tcp_socket.h"
#include "platform/proto/resdb.pb.h"
#include "platform/statistic/prometheus_handler.h"
#include "proto/kv/kv.pb.h"

namespace asio = boost::asio;
namespace beast = boost::beast;
using tcp = asio::ip::tcp;

namespace resdb {

struct VisualData {
  // Set when initializing
  int replica_id;
  int primary_id;
  std::string ip;
  int port;

  // Set when new txn is received
  int txn_number;
  std::vector<std::string> txn_command;
  std::vector<std::string> txn_key;
  std::vector<std::string> txn_value;

  // Request state if primary_id==replica_id, pre_prepare state otherwise
  std::chrono::system_clock::time_point request_pre_prepare_state_time;
  std::chrono::system_clock::time_point prepare_state_time;
  std::vector<std::chrono::system_clock::time_point>
      prepare_message_count_times_list;
  std::chrono::system_clock::time_point commit_state_time;
  std::vector<std::chrono::system_clock::time_point>
      commit_message_count_times_list;
  std::chrono::system_clock::time_point execution_time;
};

class Stats {
 public:
  static Stats* GetGlobalStats(int sleep_seconds = 5);

  void Stop();

  void RetrieveProgress();
  void SetProps(int replica_id, std::string ip, int port, bool resview_flag,
                bool faulty_flag);
  void SetPrimaryId(int primary_id);
  void RecordStateTime(std::string state);
  void GetTransactionDetails(BatchUserRequest batch_request);
  void SendSummary();
  void CrowRoute();
  bool IsFaulty();
  void ChangePrimary(int primary_id);

  void AddLatency(uint64_t run_time);
  void AddQueuingLatency(uint64_t run_time);
  void AddRoundLatency(uint64_t run_time);
  void AddCommitLatency(uint64_t run_time);
  void AddCommitQueuingLatency(uint64_t run_time);
  void AddVerifyLatency(uint64_t run_time);
  void AddExecuteQueuingLatency(uint64_t run_time);
  void AddExecuteLatency(uint64_t run_time);
  void AddCommitRuntime(uint64_t run_time);
  void AddCommitRoundLatency(uint64_t run_time);
  void AddCommitWaitingLatency(uint64_t run_time);
  void AddCommitDelay(uint64_t run_time);
  void AddExecutePrepareDelay(uint64_t run_time);
  void AddCommitInterval(uint64_t run_time);
  void AddCommitTxn(int num);
  void AddCommitBlock(int num);
  void AddBlockSize(int size);
  void AddCommitRatio(uint64_t num);
  void AddExecuteDelay(uint64_t run_time);

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

  void AddNewTransactions(int num);
  void ConsumeTransactions(int num);

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
  std::atomic<uint64_t> num_transactions_, num_transactions_time_, num_consumed_transactions_, num_consumed_transactions_time_;
  std::atomic<uint64_t> queuing_num_, queuing_time_, round_num_, round_time_, commit_num_, commit_time_;
  std::atomic<uint64_t> execute_queuing_num_, execute_queuing_time_, verify_num_, verify_time_;
  std::atomic<uint64_t> execute_num_, execute_time_;
  std::atomic<uint64_t> commit_running_num_, commit_running_time_;
  std::atomic<uint64_t> commit_queuing_num_, commit_queuing_time_;
  std::atomic<uint64_t> commit_round_num_, commit_round_time_;
  std::atomic<uint64_t> commit_txn_num_, commit_txn_time_;
  std::atomic<uint64_t> commit_block_num_, commit_block_time_;
  std::atomic<uint64_t> commit_delay_num_, commit_delay_time_;
  std::atomic<uint64_t> commit_waiting_num_, commit_waiting_time_;
  std::atomic<uint64_t> execute_prepare_num_, execute_prepare_time_;
  std::atomic<uint64_t> commit_interval_num_, commit_interval_time_;
  std::atomic<uint64_t> block_size_num_, block_size_;
  std::atomic<uint64_t> commit_ratio_num_, commit_ratio_time_;
  std::atomic<uint64_t> execute_delay_num_, execute_delay_time_;
  int monitor_sleep_time_ = 5;  // default 5s.

  std::thread crow_thread_;
  bool enable_resview;
  bool enable_faulty_switch_;
  VisualData transaction_summary_;
  std::atomic<bool> make_faulty_;
  std::atomic<uint64_t> prev_num_prepare_;
  std::atomic<uint64_t> prev_num_commit_;
  nlohmann::json summary_json_;
  nlohmann::json consensus_history_;

  std::unique_ptr<PrometheusHandler> prometheus_;

};

}  // namespace resdb

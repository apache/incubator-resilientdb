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
#include <mutex>
#include <nlohmann/json.hpp>
#include <atomic>
#include <unordered_map>

#include "boost/asio.hpp"
#include "boost/beast.hpp"
#include "platform/common/network/tcp_socket.h"
#include "platform/proto/resdb.pb.h"
#include "platform/statistic/prometheus_handler.h"
#include "proto/kv/kv.pb.h"
#include "sys/resource.h"

namespace asio = boost::asio;
namespace beast = boost::beast;
using tcp = asio::ip::tcp;

namespace resdb {

// Per-transaction telemetry data with automatic JSON serialization
struct TransactionTelemetry {
  // Static info (set once, copied to each transaction)
  int replica_id = 0;
  int primary_id = 0;
  std::string ip;
  int port = -1;

  // Transaction-specific data
  uint64_t txn_number = 0;
  std::vector<std::string> txn_command;
  std::vector<std::string> txn_key;
  std::vector<std::string> txn_value;

  // Timestamps
  std::chrono::system_clock::time_point request_pre_prepare_state_time;
  std::chrono::system_clock::time_point prepare_state_time;
  std::vector<std::chrono::system_clock::time_point>
      prepare_message_count_times_list;
  std::chrono::system_clock::time_point commit_state_time;
  std::vector<std::chrono::system_clock::time_point>
      commit_message_count_times_list;
  std::chrono::system_clock::time_point execution_time;

  // Timeline events for detailed tracking
  struct TimelineEvent {
    std::chrono::system_clock::time_point timestamp;
    std::string phase;
    int sender_id = -1;
  };
  std::vector<TimelineEvent> timeline_events;

  // Storage Engine Stats
  double ext_cache_hit_ratio_ = 0.0;
  std::string level_db_stats_;
  std::string level_db_approx_mem_size_;

  // Default constructor with min timestamps
  TransactionTelemetry()
      : request_pre_prepare_state_time(
            std::chrono::system_clock::time_point::min()),
        prepare_state_time(std::chrono::system_clock::time_point::min()),
        commit_state_time(std::chrono::system_clock::time_point::min()),
        execution_time(std::chrono::system_clock::time_point::min()) {}

  // Convert to JSON
  nlohmann::json to_json() const {
    nlohmann::json json;
    json["replica_id"] = replica_id;
    json["ip"] = ip;
    json["port"] = port;
    json["primary_id"] = primary_id;
    json["propose_pre_prepare_time"] =
        request_pre_prepare_state_time.time_since_epoch().count();
    json["prepare_time"] = prepare_state_time.time_since_epoch().count();
    json["commit_time"] = commit_state_time.time_since_epoch().count();
    json["execution_time"] = execution_time.time_since_epoch().count();

    json["prepare_message_timestamps"] = nlohmann::json::array();
    for (const auto& ts : prepare_message_count_times_list) {
      json["prepare_message_timestamps"].push_back(
          ts.time_since_epoch().count());
    }

    json["commit_message_timestamps"] = nlohmann::json::array();
    for (const auto& ts : commit_message_count_times_list) {
      json["commit_message_timestamps"].push_back(
          ts.time_since_epoch().count());
    }

    json["txn_number"] = txn_number;
    json["txn_commands"] = txn_command;
    json["txn_keys"] = txn_key;
    json["txn_values"] = txn_value;
    json["ext_cache_hit_ratio"] = ext_cache_hit_ratio_;
    json["level_db_stats"] = level_db_stats_;
    json["level_db_approx_mem_size"] = level_db_approx_mem_size_;

    // Add timeline events
    json["timeline_events"] = nlohmann::json::array();
    for (const auto& event : timeline_events) {
      nlohmann::json event_json;
      event_json["timestamp"] = event.timestamp.time_since_epoch().count();
      event_json["phase"] = event.phase;
      if (event.sender_id >= 0) {
        event_json["sender_id"] = event.sender_id;
      }
      json["timeline_events"].push_back(event_json);
    }

    return json;
  }
};

// Keep old VisualData for backwards compatibility if needed
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

  // Storage Engine Stats
  double ext_cache_hit_ratio_;
  std::string level_db_stats_;
  std::string level_db_approx_mem_size_;

  // process stats
  struct rusage process_stats_;
};

class Stats {
 public:
  static Stats* GetGlobalStats(int sleep_seconds = 5);

  void Stop();

  void RetrieveProgress();
  void SetProps(int replica_id, std::string ip, int port, bool resview_flag,
                bool faulty_flag);
  void SetPrimaryId(int primary_id);
  void SetStorageEngineMetrics(double ext_cache_hit_ratio,
                               std::string level_db_stats,
                               std::string level_db_approx_mem_size);
  void RecordStateTime(std::string state);
  void RecordStateTime(uint64_t seq, std::string state);
  void RecordPrepareRecv(uint64_t seq, int sender_id);
  void RecordCommitRecv(uint64_t seq, int sender_id);
  void RecordExecuteStart(uint64_t seq);
  void RecordExecuteEnd(uint64_t seq);
  void GetTransactionDetails(BatchUserRequest batch_request);
  void SendSummary();
  void SendSummary(uint64_t seq);
  void CrowRoute();
  bool IsFaulty();
  void ChangePrimary(int primary_id);

  void AddLatency(uint64_t run_time);

  void Monitor();
  void MonitorGlobal();

  void IncSocketRecv();

  void IncClientCall();

  void IncClientRequest();
  void IncPropose();
  void IncPrepare();
  void IncPrepare(uint64_t seq);
  void IncCommit();
  void IncCommit(uint64_t seq);
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

  // Latest executed consensus sequence observed by this replica.
  // This is updated when execution completes (and also when summaries are sent).
  uint64_t GetLastExecutedSeq() const { return last_executed_seq_.load(); }

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

  std::thread crow_thread_;
  bool enable_resview;
  bool enable_faulty_switch_;

  // Old single transaction_summary_ for backwards compatibility
  // TODO: Remove after full migration to sharded map
  VisualData transaction_summary_;

  // Sharded map solution for per-transaction telemetry
  static const int kTelemetryShards = 256;
  std::unordered_map<uint64_t, TransactionTelemetry>
      transaction_telemetry_map_[kTelemetryShards];
  std::mutex telemetry_mutex_[kTelemetryShards];
  TransactionTelemetry
      static_telemetry_info_;  // Static info to copy to new transactions
  mutable std::unordered_set<uint64_t>
      finalized_sequences_;  // Sequences that have been sent (don't create new
                             // entries)
  mutable std::mutex finalized_mutex_;  // Protect finalized_sequences_

  // Helper to get shard index
  size_t GetShardIndex(uint64_t seq) const;
  TransactionTelemetry& GetOrCreateTelemetry(uint64_t seq);
  bool IsFinalized(uint64_t seq) const;
  void MarkFinalized(uint64_t seq);

  std::atomic<bool> make_faulty_;
  std::atomic<uint64_t> prev_num_prepare_;
  std::atomic<uint64_t> prev_num_commit_;
  nlohmann::json summary_json_;

  // Tracks the most recently executed sequence number.
  std::atomic<uint64_t> last_executed_seq_{0};

  int previous_primary_id_ = -1;  // Track for view change detection

  std::unique_ptr<PrometheusHandler> prometheus_;
};

}  // namespace resdb
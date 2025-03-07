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

#include "platform/statistic/stats.h"

#include <glog/logging.h>

#include <ctime>

#include "common/utils/utils.h"
#include "proto/kv/kv.pb.h"

namespace asio = boost::asio;
namespace beast = boost::beast;
using tcp = asio::ip::tcp;

namespace resdb {

std::mutex g_mutex;
Stats* Stats::GetGlobalStats(int seconds) {
  std::unique_lock<std::mutex> lk(g_mutex);
  static Stats stats(seconds);
  return &stats;
}  // gets a singelton instance of Stats Class

Stats::Stats(int sleep_time) {
  monitor_sleep_time_ = sleep_time;
#ifdef TEST_MODE
  monitor_sleep_time_ = 1;
#endif
  num_call_ = 0;
  num_commit_ = 0;
  run_time_ = 0;
  run_call_ = 0;
  run_call_time_ = 0;
  server_call_ = 0;
  server_process_ = 0;
  run_req_num_ = 0;
  run_req_run_time_ = 0;
  seq_gap_ = 0;
  total_request_ = 0;
  total_geo_request_ = 0;
  geo_request_ = 0;

  stop_ = false;
  begin_ = false;

  socket_recv_ = 0;
  broad_cast_msg_ = 0;
  send_broad_cast_msg_ = 0;

  prometheus_ = nullptr;
  global_thread_ =
      std::thread(&Stats::MonitorGlobal, this);  // pass by reference

  transaction_summary_.port = -1;

  // Setup websocket here
  make_faulty_.store(false);
  transaction_summary_.request_pre_prepare_state_time =
      std::chrono::system_clock::time_point::min();
  transaction_summary_.prepare_state_time =
      std::chrono::system_clock::time_point::min();
  transaction_summary_.commit_state_time =
      std::chrono::system_clock::time_point::min();
  transaction_summary_.execution_time =
      std::chrono::system_clock::time_point::min();
  transaction_summary_.txn_number = 0;
}

void Stats::Stop() { stop_ = true; }

Stats::~Stats() {
  stop_ = true;
  if (global_thread_.joinable()) {
    global_thread_.join();
  }
  if (enable_resview && crow_thread_.joinable()) {
    crow_thread_.join();
  }
}

int64_t GetRSS() {
  int64_t rss = 0;
  FILE* fp = NULL;
  if ((fp = fopen("/proc/self/statm", "r")) == NULL) {
    return 0;
  }

  uint64_t size, resident, share, text, lib, data, dt;
  if (fscanf(fp, "%lu %lu %lu %lu %lu %lu %lu", &size, &resident, &share, &text,
             &lib, &data, &dt) != 7) {
    fclose(fp);
    return 0;
  }
  fclose(fp);

  int64_t page_size = sysconf(_SC_PAGESIZE);
  rss = resident * page_size;

  // Convert to MB
  rss = rss / (1024 * 1024);

  return rss;
}

void Stats::CrowRoute() {
  crow::SimpleApp app;
  while (!stop_) {
    try {
      CROW_ROUTE(app, "/consensus_data")
          .methods("GET"_method)([this](const crow::request& req,
                                        crow::response& res) {
            LOG(ERROR) << "API 1";
            res.set_header("Access-Control-Allow-Origin",
                           "*");  // Allow requests from any origin
            res.set_header("Access-Control-Allow-Methods",
                           "GET, POST, OPTIONS");  // Specify allowed methods
            res.set_header(
                "Access-Control-Allow-Headers",
                "Content-Type, Authorization");  // Specify allowed headers

            // Send your response
            res.body = consensus_history_.dump();
            res.end();
          });
      CROW_ROUTE(app, "/get_status")
          .methods("GET"_method)([this](const crow::request& req,
                                        crow::response& res) {
            LOG(ERROR) << "API 2";
            res.set_header("Access-Control-Allow-Origin",
                           "*");  // Allow requests from any origin
            res.set_header("Access-Control-Allow-Methods",
                           "GET, POST, OPTIONS");  // Specify allowed methods
            res.set_header(
                "Access-Control-Allow-Headers",
                "Content-Type, Authorization");  // Specify allowed headers

            // Send your response
            res.body = IsFaulty() ? "Faulty" : "Not Faulty";
            res.end();
          });
      CROW_ROUTE(app, "/make_faulty")
          .methods("GET"_method)([this](const crow::request& req,
                                        crow::response& res) {
            LOG(ERROR) << "API 3";
            res.set_header("Access-Control-Allow-Origin",
                           "*");  // Allow requests from any origin
            res.set_header("Access-Control-Allow-Methods",
                           "GET, POST, OPTIONS");  // Specify allowed methods
            res.set_header(
                "Access-Control-Allow-Headers",
                "Content-Type, Authorization");  // Specify allowed headers

            // Send your response
            if (enable_faulty_switch_) {
              make_faulty_.store(!make_faulty_.load());
            }
            res.body = "Success";
            res.end();
          });
      CROW_ROUTE(app, "/transaction_data")
          .methods("GET"_method)([this](const crow::request& req,
                                        crow::response& res) {
            LOG(ERROR) << "API 4";
            res.set_header("Access-Control-Allow-Origin",
                           "*");  // Allow requests from any origin
            res.set_header("Access-Control-Allow-Methods",
                           "GET, POST, OPTIONS");  // Specify allowed methods
            res.set_header(
                "Access-Control-Allow-Headers",
                "Content-Type, Authorization");  // Specify allowed headers

            nlohmann::json mem_view_json;
            int status =
                getrusage(RUSAGE_SELF, &transaction_summary_.process_stats_);
            if (status == 0) {
              mem_view_json["resident_set_size"] = GetRSS();
              mem_view_json["max_resident_set_size"] =
                  transaction_summary_.process_stats_.ru_maxrss;
              mem_view_json["num_reads"] =
                  transaction_summary_.process_stats_.ru_inblock;
              mem_view_json["num_writes"] =
                  transaction_summary_.process_stats_.ru_oublock;
            }

            mem_view_json["ext_cache_hit_ratio"] =
                transaction_summary_.ext_cache_hit_ratio_;
            mem_view_json["level_db_stats"] =
                transaction_summary_.level_db_stats_;
            mem_view_json["level_db_approx_mem_size"] =
                transaction_summary_.level_db_approx_mem_size_;
            res.body = mem_view_json.dump();
            mem_view_json.clear();
            res.end();
          });
      app.port(8500 + transaction_summary_.port).multithreaded().run();
      sleep(1);
    } catch (const std::exception& e) {
    }
  }
  app.stop();
}

bool Stats::IsFaulty() { return make_faulty_.load(); }

void Stats::ChangePrimary(int primary_id) {
  transaction_summary_.primary_id = primary_id;
  make_faulty_.store(false);
}

void Stats::SetProps(int replica_id, std::string ip, int port,
                     bool resview_flag, bool faulty_flag) {
  transaction_summary_.replica_id = replica_id;
  transaction_summary_.ip = ip;
  transaction_summary_.port = port;
  enable_resview = resview_flag;
  enable_faulty_switch_ = faulty_flag;
  if (resview_flag) {
    crow_thread_ = std::thread(&Stats::CrowRoute, this);
  }
}

void Stats::SetPrimaryId(int primary_id) {
  transaction_summary_.primary_id = primary_id;
}

void Stats::SetStorageEngineMetrics(double ext_cache_hit_ratio,
                                    std::string level_db_stats,
                                    std::string level_db_approx_mem_size) {
  transaction_summary_.ext_cache_hit_ratio_ = ext_cache_hit_ratio;
  transaction_summary_.level_db_stats_ = level_db_stats;
  transaction_summary_.level_db_approx_mem_size_ = level_db_approx_mem_size;
}

void Stats::RecordStateTime(std::string state) {
  if (!enable_resview) {
    return;
  }
  if (state == "request" || state == "pre-prepare") {
    transaction_summary_.request_pre_prepare_state_time =
        std::chrono::system_clock::now();
  } else if (state == "prepare") {
    transaction_summary_.prepare_state_time = std::chrono::system_clock::now();
  } else if (state == "commit") {
    transaction_summary_.commit_state_time = std::chrono::system_clock::now();
  }
}

void Stats::GetTransactionDetails(BatchUserRequest batch_request) {
  if (!enable_resview) {
    return;
  }
  transaction_summary_.txn_number = batch_request.seq();
  transaction_summary_.txn_command.clear();
  transaction_summary_.txn_key.clear();
  transaction_summary_.txn_value.clear();
  for (auto& sub_request : batch_request.user_requests()) {
    KVRequest kv_request;
    if (!kv_request.ParseFromString(sub_request.request().data())) {
      break;
    }
    if (kv_request.cmd() == KVRequest::SET) {
      transaction_summary_.txn_command.push_back("SET");
      transaction_summary_.txn_key.push_back(kv_request.key());
      transaction_summary_.txn_value.push_back(kv_request.value());
    } else if (kv_request.cmd() == KVRequest::GET) {
      transaction_summary_.txn_command.push_back("GET");
      transaction_summary_.txn_key.push_back(kv_request.key());
      transaction_summary_.txn_value.push_back("");
    } else if (kv_request.cmd() == KVRequest::GETALLVALUES) {
      transaction_summary_.txn_command.push_back("GETALLVALUES");
      transaction_summary_.txn_key.push_back(kv_request.key());
      transaction_summary_.txn_value.push_back("");
    } else if (kv_request.cmd() == KVRequest::GETRANGE) {
      transaction_summary_.txn_command.push_back("GETRANGE");
      transaction_summary_.txn_key.push_back(kv_request.key());
      transaction_summary_.txn_value.push_back(kv_request.value());
    }
  }
}

void Stats::SendSummary() {
  if (!enable_resview) {
    return;
  }
  transaction_summary_.execution_time = std::chrono::system_clock::now();

  // Convert Transaction Summary to JSON
  summary_json_["replica_id"] = transaction_summary_.replica_id;
  summary_json_["ip"] = transaction_summary_.ip;
  summary_json_["port"] = transaction_summary_.port;
  summary_json_["primary_id"] = transaction_summary_.primary_id;
  summary_json_["propose_pre_prepare_time"] =
      transaction_summary_.request_pre_prepare_state_time.time_since_epoch()
          .count();
  summary_json_["prepare_time"] =
      transaction_summary_.prepare_state_time.time_since_epoch().count();
  summary_json_["commit_time"] =
      transaction_summary_.commit_state_time.time_since_epoch().count();
  summary_json_["execution_time"] =
      transaction_summary_.execution_time.time_since_epoch().count();
  for (size_t i = 0;
       i < transaction_summary_.prepare_message_count_times_list.size(); i++) {
    summary_json_["prepare_message_timestamps"].push_back(
        transaction_summary_.prepare_message_count_times_list[i]
            .time_since_epoch()
            .count());
  }
  for (size_t i = 0;
       i < transaction_summary_.commit_message_count_times_list.size(); i++) {
    summary_json_["commit_message_timestamps"].push_back(
        transaction_summary_.commit_message_count_times_list[i]
            .time_since_epoch()
            .count());
  }
  summary_json_["txn_number"] = transaction_summary_.txn_number;
  for (size_t i = 0; i < transaction_summary_.txn_command.size(); i++) {
    summary_json_["txn_commands"].push_back(
        transaction_summary_.txn_command[i]);
  }
  for (size_t i = 0; i < transaction_summary_.txn_key.size(); i++) {
    summary_json_["txn_keys"].push_back(transaction_summary_.txn_key[i]);
  }
  for (size_t i = 0; i < transaction_summary_.txn_value.size(); i++) {
    summary_json_["txn_values"].push_back(transaction_summary_.txn_value[i]);
  }

  summary_json_["ext_cache_hit_ratio"] =
      transaction_summary_.ext_cache_hit_ratio_;
  consensus_history_[std::to_string(transaction_summary_.txn_number)] =
      summary_json_;

  LOG(ERROR) << summary_json_.dump();

  // Reset Transaction Summary Parameters
  transaction_summary_.request_pre_prepare_state_time =
      std::chrono::system_clock::time_point::min();
  transaction_summary_.prepare_state_time =
      std::chrono::system_clock::time_point::min();
  transaction_summary_.commit_state_time =
      std::chrono::system_clock::time_point::min();
  transaction_summary_.execution_time =
      std::chrono::system_clock::time_point::min();
  transaction_summary_.prepare_message_count_times_list.clear();
  transaction_summary_.commit_message_count_times_list.clear();

  summary_json_.clear();
}

void Stats::MonitorGlobal() {
  LOG(ERROR) << "monitor:" << name_ << " sleep time:" << monitor_sleep_time_;

  uint64_t seq_fail = 0;
  uint64_t client_call = 0, socket_recv = 0;
  uint64_t num_client_req = 0, num_propose = 0, num_prepare = 0, num_commit = 0,
           pending_execute = 0, execute = 0, execute_done = 0;
  uint64_t broad_cast_msg = 0, send_broad_cast_msg = 0;
  uint64_t send_broad_cast_msg_per_rep = 0;
  uint64_t server_call = 0, server_process = 0;
  uint64_t seq_gap = 0;
  uint64_t total_request = 0, total_geo_request = 0, geo_request = 0;

  // ====== for client proxy ======
  uint64_t run_req_num = 0, run_req_run_time = 0;

  uint64_t last_run_req_num = 0, last_run_req_run_time = 0;
  // =============================

  uint64_t last_seq_fail = 0;
  uint64_t last_num_client_req = 0, last_num_propose = 0, last_num_prepare = 0,
           last_num_commit = 0;
  uint64_t last_pending_execute = 0, last_execute = 0, last_execute_done = 0;
  uint64_t last_client_call = 0, last_socket_recv = 0;
  uint64_t last_broad_cast_msg = 0, last_send_broad_cast_msg = 0;
  uint64_t last_send_broad_cast_msg_per_rep = 0;
  uint64_t last_server_call = 0, last_server_process = 0;
  uint64_t last_total_request = 0, last_total_geo_request = 0,
           last_geo_request = 0;
  uint64_t time = 0;

  while (!stop_) {
    sleep(monitor_sleep_time_);
    time += monitor_sleep_time_;
    seq_fail = seq_fail_;
    socket_recv = socket_recv_;
    client_call = client_call_;
    num_client_req = num_client_req_;
    num_propose = num_propose_;
    num_prepare = num_prepare_;
    num_commit = num_commit_;
    pending_execute = pending_execute_;
    execute = execute_;
    execute_done = execute_done_;
    broad_cast_msg = broad_cast_msg_;
    send_broad_cast_msg = send_broad_cast_msg_;
    send_broad_cast_msg_per_rep = send_broad_cast_msg_per_rep_;
    server_call = server_call_;
    server_process = server_process_;
    seq_gap = seq_gap_;
    total_request = total_request_;
    total_geo_request = total_geo_request_;
    geo_request = geo_request_;

    run_req_num = run_req_num_;
    run_req_run_time = run_req_run_time_;

    LOG(ERROR) << "=========== monitor =========\n"
               << "server call:" << server_call - last_server_call
               << " server process:" << server_process - last_server_process
               << " socket recv:" << socket_recv - last_socket_recv
               << " "
                  "client call:"
               << client_call - last_client_call
               << " "
                  "client req:"
               << num_client_req - last_num_client_req
               << " "
                  "broad_cast:"
               << broad_cast_msg - last_broad_cast_msg
               << " "
                  "send broad_cast:"
               << send_broad_cast_msg - last_send_broad_cast_msg
               << " "
                  "per send broad_cast:"
               << send_broad_cast_msg_per_rep - last_send_broad_cast_msg_per_rep
               << " "
                  "propose:"
               << num_propose - last_num_propose
               << " "
                  "prepare:"
               << (num_prepare - last_num_prepare)
               << " "
                  "commit:"
               << (num_commit - last_num_commit)
               << " "
                  "pending execute:"
               << pending_execute - last_pending_execute
               << " "
                  "execute:"
               << execute - last_execute
               << " "
                  "execute done:"
               << execute_done - last_execute_done << " seq gap:" << seq_gap
               << " total request:" << total_request - last_total_request
               << " txn:" << (total_request - last_total_request) / 5
               << " total geo request:"
               << total_geo_request - last_total_geo_request
               << " total geo request per:"
               << (total_geo_request - last_total_geo_request) / 5
               << " geo request:" << (geo_request - last_geo_request)
               << " "
                  "seq fail:"
               << seq_fail - last_seq_fail << " time:" << time
               << " "
                  "\n--------------- monitor ------------";
    if (run_req_num - last_run_req_num > 0) {
      LOG(ERROR) << "  req client latency:"
                 << static_cast<double>(run_req_run_time -
                                        last_run_req_run_time) /
                        (run_req_num - last_run_req_num) / 1000000000.0;
    }

    last_seq_fail = seq_fail;
    last_socket_recv = socket_recv;
    last_client_call = client_call;
    last_num_client_req = num_client_req;
    last_num_propose = num_propose;
    last_num_prepare = num_prepare;
    last_num_commit = num_commit;
    last_pending_execute = pending_execute;
    last_execute = execute;
    last_execute_done = execute_done;

    last_broad_cast_msg = broad_cast_msg;
    last_send_broad_cast_msg = send_broad_cast_msg;
    last_send_broad_cast_msg_per_rep = send_broad_cast_msg_per_rep;

    last_server_call = server_call;
    last_server_process = server_process;

    last_run_req_num = run_req_num;
    last_run_req_run_time = run_req_run_time;
    last_total_request = total_request;
    last_total_geo_request = total_geo_request;
    last_geo_request = geo_request;
  }
}

void Stats::IncClientCall() {
  if (prometheus_) {
    prometheus_->Inc(CLIENT_CALL, 1);
  }
  client_call_++;
}

void Stats::IncClientRequest() {
  if (prometheus_) {
    prometheus_->Inc(CLIENT_REQ, 1);
  }
  num_client_req_++;
}

void Stats::IncPropose() {
  if (prometheus_) {
    prometheus_->Inc(PROPOSE, 1);
  }
  num_propose_++;
}

void Stats::IncPrepare() {
  if (prometheus_) {
    prometheus_->Inc(PREPARE, 1);
  }
  num_prepare_++;
  transaction_summary_.prepare_message_count_times_list.push_back(
      std::chrono::system_clock::now());
}

void Stats::IncCommit() {
  if (prometheus_) {
    prometheus_->Inc(COMMIT, 1);
  }
  num_commit_++;
  transaction_summary_.commit_message_count_times_list.push_back(
      std::chrono::system_clock::now());
}

void Stats::IncPendingExecute() { pending_execute_++; }

void Stats::IncExecute() { execute_++; }

void Stats::IncExecuteDone() {
  if (prometheus_) {
    prometheus_->Inc(EXECUTE, 1);
  }
  execute_done_++;
}

void Stats::BroadCastMsg() {
  if (prometheus_) {
    prometheus_->Inc(BROAD_CAST, 1);
  }
  broad_cast_msg_++;
}

void Stats::SendBroadCastMsg(uint32_t num) { send_broad_cast_msg_ += num; }

void Stats::SendBroadCastMsgPerRep() { send_broad_cast_msg_per_rep_++; }

void Stats::SeqFail() { seq_fail_++; }

void Stats::IncTotalRequest(uint32_t num) {
  if (prometheus_) {
    prometheus_->Inc(NUM_EXECUTE_TX, num);
  }
  total_request_ += num;
}

void Stats::IncTotalGeoRequest(uint32_t num) { total_geo_request_ += num; }

void Stats::IncGeoRequest() { geo_request_++; }

void Stats::ServerCall() {
  if (prometheus_) {
    prometheus_->Inc(SERVER_CALL_NAME, 1);
  }
  server_call_++;
}

void Stats::ServerProcess() {
  if (prometheus_) {
    prometheus_->Inc(SERVER_PROCESS, 1);
  }
  server_process_++;
}

void Stats::SeqGap(uint64_t seq_gap) { seq_gap_ = seq_gap; }

void Stats::AddLatency(uint64_t run_time) {
  run_req_num_++;
  run_req_run_time_ += run_time;
}

void Stats::SetPrometheus(const std::string& prometheus_address) {
  prometheus_ = std::make_unique<PrometheusHandler>(prometheus_address);
}

}  // namespace resdb

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

#include "common/utils/utils.h"

namespace resdb {

std::mutex g_mutex;
Stats* Stats::GetGlobalStats(int seconds) {
  std::unique_lock<std::mutex> lk(g_mutex);
  static Stats stats(seconds);
  return &stats;
}

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
}

void Stats::Stop() { stop_ = true; }

Stats::~Stats() {
  stop_ = true;
  if (global_thread_.joinable()) {
    global_thread_.join();
  }
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
  

  uint64_t num_transactions = 0, num_consumed_transactions = 0;
  uint64_t num_transactions_time = 0, num_consumed_transactions_time = 0;
  uint64_t last_num_transactions = 0, last_num_consumed_transactions = 0;
  uint64_t last_num_transactions_time = 0, last_num_consumed_transactions_time = 0;

  uint64_t queuing_num = 0, queuing_time = 0;
  uint64_t  last_queuing_num = 0, last_queuing_time = 0;
  uint64_t round_num = 0, round_time = 0;
  uint64_t last_round_num = 0, last_round_time = 0;

  uint64_t commit_num = 0, commit_time = 0;
  uint64_t last_commit_num = 0, last_commit_time = 0;

  uint64_t verify_num = 0, verify_time = 0;
  uint64_t last_verify_num = 0, last_verify_time = 0;

  uint64_t execute_queuing_num = 0, execute_queuing_time = 0;
  uint64_t last_execute_queuing_num = 0, last_execute_queuing_time = 0;

  uint64_t execute_num = 0, execute_time = 0;
  uint64_t last_execute_num = 0, last_execute_time = 0;

  uint64_t commit_running_num = 0, commit_running_time = 0;
  uint64_t last_commit_running_num = 0, last_commit_running_time = 0;

  uint64_t commit_delay_num = 0, commit_delay_time = 0;
  uint64_t last_commit_delay_num = 0, last_commit_delay_time = 0;

  uint64_t commit_waiting_num = 0, commit_waiting_time = 0;
  uint64_t last_commit_waiting_num = 0, last_commit_waiting_time = 0;

  uint64_t execute_prepare_num = 0, execute_prepare_time = 0;
  uint64_t last_execute_prepare_num = 0, last_execute_prepare_time = 0;

  uint64_t commit_interval_num = 0, commit_interval_time = 0;
  uint64_t last_commit_interval_num = 0, last_commit_interval_time = 0;

  uint64_t commit_queuing_num = 0, commit_queuing_time = 0;
  uint64_t last_commit_queuing_num = 0, last_commit_queuing_time = 0;

  uint64_t commit_round_num = 0, commit_round_time = 0;
  uint64_t last_commit_round_num = 0, last_commit_round_time = 0;

  uint64_t commit_txn_num = 0, commit_txn_time = 0;
  uint64_t last_commit_txn_num = 0, last_commit_txn_time = 0;

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

    queuing_num = queuing_num_;
    queuing_time = queuing_time_;

    round_num = round_num_;
    round_time = round_time_;

    commit_num = commit_num_;
    commit_time = commit_time_;

    execute_queuing_num = execute_queuing_num_;
    execute_queuing_time = execute_queuing_time_;

    execute_num = execute_num_;
    execute_time = execute_time_;

    commit_running_num = commit_running_num_;
    commit_running_time = commit_running_time_;

    commit_delay_num = commit_delay_num_;
    commit_delay_time = commit_delay_time_;

    commit_waiting_num = commit_waiting_num_;
    commit_waiting_time = commit_waiting_time_;

    execute_prepare_num = execute_prepare_num_;
    execute_prepare_time = execute_prepare_time_;

    commit_interval_num = commit_interval_num_;
    commit_interval_time = commit_interval_time_;

    commit_queuing_num = commit_queuing_num_;
    commit_queuing_time = commit_queuing_time_;

    commit_round_num = commit_round_num_;
    commit_round_time = commit_round_time_;

    commit_txn_num = commit_txn_num_;
    commit_txn_time = commit_txn_time_;

    verify_num = verify_num_;
    verify_time = verify_time_;

    num_transactions = num_transactions_;
    num_consumed_transactions = num_consumed_transactions_;

    num_transactions_time = num_transactions_time_;
    num_consumed_transactions_time = num_consumed_transactions_time_;

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
                  "new transactions:"
               << (num_transactions - last_num_transactions )
               << " "
                  "consumed transactions:"
               << (num_consumed_transactions - last_num_consumed_transactions)
               << " queuing latency :"
                 << static_cast<double>(queuing_time -
                                        last_queuing_time) /
                        (queuing_num - last_queuing_num) / 1000000.0
               << " round latency :"
                 << static_cast<double>(round_time -
                                        last_round_time) /
                        (round_num - last_round_num) / 1000000.0
               << " commit latency :"
                 << static_cast<double>(commit_time -
                                        last_commit_time) /
                        (commit_num - last_commit_num) / 1000000.0

              << " verify latency :"
                 << static_cast<double>(verify_time -
                                        last_verify_time) /
                        (verify_num - last_verify_num) / 1000000.0

              << " execute_queuing latency :"
                 << static_cast<double>(execute_queuing_time -
                                        last_execute_queuing_time) /
                        (execute_queuing_num - last_execute_queuing_num) / 1000000.0

              << " execute latency :"
                 << static_cast<double>(execute_time -
                                        last_execute_time) /
                        (execute_num - last_execute_num) / 1000000.0

              << " commit_queuing latency :"
                 << static_cast<double>(commit_queuing_time -
                                        last_commit_queuing_time) /
                        (commit_queuing_num - last_commit_queuing_num) / 1000000.0

              << " commit_running latency :"
                 << static_cast<double>(commit_running_time -
                                        last_commit_running_time) /
                        (commit_running_num - last_commit_running_num) / 1000000.0

            << " commit_delay latency :"
                 << static_cast<double>(commit_delay_time -
                                        last_commit_delay_time) /
                        (commit_delay_num - last_commit_delay_num) / 1000000.0

            << " commit_waiting latency :"
                 << static_cast<double>(commit_waiting_time -
                                        last_commit_waiting_time) /
                        (commit_waiting_num - last_commit_waiting_num) / 1000000.0

            << " execute_prepare latency :"
                 << static_cast<double>(execute_prepare_time -
                                        last_execute_prepare_time) /
                        (execute_prepare_num - last_execute_prepare_num) / 1000000.0

              << " commit_round latency :"
                 << static_cast<double>(commit_round_time -
                                        last_commit_round_time) /
                        (commit_round_num - last_commit_round_num) 

            << " commit_interval latency :"
                 << static_cast<double>(commit_interval_time -
                                        last_commit_interval_time) /
                        (commit_interval_num - last_commit_interval_num) / 1000000.0

              << " commit_txn latency :"
                 << static_cast<double>(commit_txn_time -
                                        last_commit_txn_time) /
                        (commit_txn_num - last_commit_txn_num) 

               << " "
                  "\n--------------- monitor ------------";
    if (run_req_num - last_run_req_num > 0) {
      LOG(ERROR) << "  req client latency:"
                 << static_cast<double>(run_req_run_time -
                                        last_run_req_run_time) /
                        (run_req_num - last_run_req_num) / 1000000.0;
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

    last_num_transactions = num_transactions;
    last_num_consumed_transactions = num_consumed_transactions;

    last_num_transactions_time = num_transactions_time;
    last_num_consumed_transactions_time = num_consumed_transactions_time;

    last_queuing_num = queuing_num;
    last_queuing_time = queuing_time;

    last_round_num = round_num;
    last_round_time = round_time;

    last_commit_num = commit_num;
    last_commit_time = commit_time;

    last_execute_queuing_num = execute_queuing_num;
    last_execute_queuing_time = execute_queuing_time;

    last_execute_num = execute_num;
    last_execute_time = execute_time;

    last_commit_running_num = commit_running_num;
    last_commit_running_time = commit_running_time;

    last_commit_delay_num = commit_delay_num;
    last_commit_delay_time = commit_delay_time;

    last_commit_waiting_num = commit_waiting_num;
    last_commit_waiting_time = commit_waiting_time;

    last_execute_prepare_num = execute_prepare_num;
    last_execute_prepare_time = execute_prepare_time;

    last_commit_interval_num = commit_interval_num;
    last_commit_interval_time = commit_interval_time;

    last_commit_queuing_num = commit_queuing_num;
    last_commit_queuing_time = commit_queuing_time;

    last_commit_round_num = commit_round_num;
    last_commit_round_time = commit_round_time;

    last_verify_num = verify_num;
    last_verify_time = verify_time;
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
}

void Stats::IncCommit() {
  if (prometheus_) {
    prometheus_->Inc(COMMIT, 1);
  }
  num_commit_++;
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

void Stats::AddNewTransactions(int num) {
  num_transactions_++;
}

void Stats::ConsumeTransactions(int num) {
  num_consumed_transactions_++;
}

void Stats::SeqGap(uint64_t seq_gap) { seq_gap_ = seq_gap; }

void Stats::AddLatency(uint64_t run_time) {
  run_req_num_++;
  run_req_run_time_ += run_time;
}

void Stats::AddQueuingLatency(uint64_t run_time) {
  queuing_num_++;
  queuing_time_ += run_time;
}

void Stats::AddRoundLatency(uint64_t run_time) {
  round_num_++;
  round_time_ += run_time;
}

void Stats::AddCommitLatency(uint64_t run_time) {
  commit_num_++;
  commit_time_ += run_time;
}

void Stats::AddVerifyLatency(uint64_t run_time) {
  verify_num_++;
  verify_time_ += run_time;
}

void Stats::AddExecuteQueuingLatency(uint64_t run_time) {
  execute_queuing_num_++;
  execute_queuing_time_ += run_time;
}

void Stats::AddExecuteLatency(uint64_t run_time) {
  execute_num_++;
  execute_time_ += run_time;
}

void Stats::AddCommitQueuingLatency(uint64_t run_time) {
  commit_queuing_num_++;
  commit_queuing_time_ += run_time;
}

void Stats::AddCommitRuntime(uint64_t run_time) {
  commit_running_num_++;
  commit_running_time_ += run_time;
}

void Stats::AddCommitWaitingLatency(uint64_t run_time) {
  commit_waiting_num_++;
  commit_waiting_time_ += run_time;
}

void Stats::AddCommitDelay(uint64_t run_time) {
  commit_delay_num_++;
  commit_delay_time_ += run_time;
}

void Stats::AddExecutePrepareDelay(uint64_t run_time) {
  execute_prepare_num_++;
  execute_prepare_time_ += run_time;
}

void Stats::AddCommitRoundLatency(uint64_t run_time) {
  //LOG(ERROR)<<"commit round:"<<run_time;
  commit_round_num_++;
  commit_round_time_ += run_time;
}

void Stats::AddCommitInterval(uint64_t run_time) {
  commit_interval_num_++;
  commit_interval_time_ += run_time;
}

void Stats::AddCommitTxn(int num) {
  commit_txn_num_++;
  commit_txn_time_ += num;
}

void Stats::SetPrometheus(const std::string& prometheus_address) {
  prometheus_ = std::make_unique<PrometheusHandler>(prometheus_address);
}

}  // namespace resdb

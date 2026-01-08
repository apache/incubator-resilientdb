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

#include "platform/statistic/prometheus_handler.h"

#include <glog/logging.h>

namespace resdb {

std::map<TableName, std::string> table_names = {
    {GENERAL, "general"},
    {WORKER_THREAD, "worker_thread"},
    {IO_THREAD, "io_thread"},
    {CLIENT, "client"},
    {CONFIGURATION, "configuration"},
    {SERVER, "server"},
    {CONSENSUS, "consensus"}};

std::map<MetricName, std::pair<TableName, std::string>> metric_names = {
    {SERVER_CALL_NAME, {SERVER, "server_call"}},
    {SERVER_PROCESS, {SERVER, "server_process"}},
    {CLIENT_CALL, {CLIENT, "client_call"}},
    {CLIENT_REQ, {CLIENT, "client_req"}},
    {SOCKET_RECV, {IO_THREAD, "socket_recv"}},
    {BROAD_CAST, {IO_THREAD, "broad_cast"}},
    {PROPOSE, {CONSENSUS, "propose"}},
    {PREPARE, {CONSENSUS, "prepare"}},
    {COMMIT, {CONSENSUS, "commit"}},
    {EXECUTE, {CONSENSUS, "execute"}},
    {NUM_EXECUTE_TX, {CONSENSUS, "num_execute_tx"}},
    {CLIENT_LATENCY, {CLIENT, "client_latency"}},
    {SEQ_FAIL, {CONSENSUS, "seq_fail"}},
    {SEQ_GAP, {CONSENSUS, "seq_gap"}},
    {PENDING_EXECUTE, {CONSENSUS, "pending_execute"}},
    {EXECUTE_DONE, {CONSENSUS, "execute_done"}},
    {SEND_BROADCAST_MSG, {IO_THREAD, "send_broadcast_msg"}},
    {TOTAL_REQUEST, {CONSENSUS, "total_request"}},
    {PREPARE_PHASE_LATENCY, {CONSENSUS, "prepare_phase_latency"}},
    {COMMIT_PHASE_LATENCY, {CONSENSUS, "commit_phase_latency"}},
    {EXECUTION_DURATION, {CONSENSUS, "execution_duration"}},
    {CACHE_HIT_RATIO, {GENERAL, "cache_hit_ratio"}},
    {LEVELDB_MEM_SIZE, {GENERAL, "leveldb_mem_size"}},
    {SEND_BROADCAST_PER_REP, {IO_THREAD, "send_broadcast_per_rep"}},
    {GEO_REQUEST, {CLIENT, "geo_request"}},
    {TOTAL_GEO_REQUEST, {CLIENT, "total_geo_request"}},
    {VIEW_CHANGE, {CONSENSUS, "view_change"}}};

PrometheusHandler::PrometheusHandler(const std::string& server_address) {
  exposer_ =
      prometheus::detail::make_unique<prometheus::Exposer>(server_address);

  registry_ = std::make_shared<prometheus::Registry>();
  exposer_->RegisterCollectable(registry_);

  Register();
}

PrometheusHandler::~PrometheusHandler() {
  if (exposer_) {
    exposer_->RemoveCollectable(registry_);
  }
}

void PrometheusHandler::Register() {
  for (auto& table : table_names) {
    RegisterTable(table.second);
  }

  for (auto& metric_pair : metric_names) {
    // Register Summary metrics (latencies)
    if (metric_pair.first == CLIENT_LATENCY ||
        metric_pair.first == PREPARE_PHASE_LATENCY ||
        metric_pair.first == COMMIT_PHASE_LATENCY ||
        metric_pair.first == EXECUTION_DURATION) {
      RegisterSummaryMetric(table_names[metric_pair.second.first],
                            metric_pair.second.second);
    } else {
      RegisterMetric(table_names[metric_pair.second.first],
                     metric_pair.second.second);
    }
  }
}

void PrometheusHandler::RegisterTable(const std::string& name) {
  gbuilder* gauge = &prometheus::BuildGauge()
                         .Name(name)
                         .Help(name + " metrics")
                         .Register(*registry_);
  gauge_[name] = gauge;
}

void PrometheusHandler::RegisterMetric(const std::string& table_name,
                                       const std::string& metric_name) {
  if (gauge_.find(table_name) == gauge_.end()) {
    return;
  }
  gmetric* metric = &gauge_[table_name]->Add({{"metrics", metric_name}});
  metric_[metric_name] = metric;
}

void PrometheusHandler::Set(MetricName name, double value) {
  std::string metric_name_str = metric_names[name].second;
  if (metric_.find(metric_name_str) == metric_.end()) {
    return;
  }
  metric_[metric_name_str]->Set(value);
}

void PrometheusHandler::Inc(MetricName name, double value) {
  std::string metric_name_str = metric_names[name].second;
  if (metric_.find(metric_name_str) == metric_.end()) {
    return;
  }
  metric_[metric_name_str]->Increment(value);
}

void PrometheusHandler::RegisterSummaryMetric(const std::string& table_name,
                                               const std::string& metric_name) {
  // Create summary family if it doesn't exist
  if (summary_.find(table_name) == summary_.end()) {
    sbuilder* summary_family = &prometheus::BuildSummary()
                                   .Name(table_name + "_summary")
                                   .Help(table_name + " summary metrics")
                                   .Register(*registry_);
    summary_[table_name] = summary_family;
  }
  
  // Create summary metric
  smetric* summary_metric = &summary_[table_name]->Add(
      {{"metrics", metric_name}},
      prometheus::Summary::Quantiles{
          {0.5, 0.05},   // p50 with 5% error
          {0.9, 0.01},   // p90 with 1% error
          {0.95, 0.01},  // p95 with 1% error
          {0.99, 0.001}  // p99 with 0.1% error
      });
  summary_metric_[metric_name] = summary_metric;
}

void PrometheusHandler::Observe(MetricName name, double value) {
  std::string metric_name_str = metric_names[name].second;
  if (summary_metric_.find(metric_name_str) == summary_metric_.end()) {
    return;
  }
  summary_metric_[metric_name_str]->Observe(value);
}

}  // namespace resdb

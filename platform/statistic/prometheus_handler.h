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

#include <glog/logging.h>
#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <prometheus/summary.h>

namespace resdb {

enum TableName {
  GENERAL,
  WORKER_THREAD,
  IO_THREAD,
  CLIENT,
  CONFIGURATION,
  SERVER,
  CONSENSUS,
};

enum MetricName {
  SERVER_CALL_NAME,
  SERVER_PROCESS,
  CLIENT_CALL,
  CLIENT_REQ,
  SOCKET_RECV,
  BROAD_CAST,
  PROPOSE,
  PREPARE,
  COMMIT,
  EXECUTE,
  NUM_EXECUTE_TX,
  CLIENT_LATENCY,
  // New metrics
  SEQ_FAIL,
  SEQ_GAP,
  PENDING_EXECUTE,
  EXECUTE_DONE,
  SEND_BROADCAST_MSG,
  TOTAL_REQUEST,
  PREPARE_PHASE_LATENCY,
  COMMIT_PHASE_LATENCY,
  EXECUTION_DURATION,
  CACHE_HIT_RATIO,
  LEVELDB_MEM_SIZE,
  SEND_BROADCAST_PER_REP,
  GEO_REQUEST,
  TOTAL_GEO_REQUEST,
  VIEW_CHANGE,
};

class PrometheusHandler {
 public:
  PrometheusHandler(const std::string& server_address);
  ~PrometheusHandler();

  void Set(MetricName name, double value);
  void Inc(MetricName name, double value);
  void Observe(MetricName name, double value);

 protected:
  void Register();
  void RegisterTable(const std::string& name);
  void RegisterMetric(const std::string& table_name,
                      const std::string& metric_name);

 private:
  typedef prometheus::Family<prometheus::Gauge> gbuilder;
  typedef prometheus::Gauge gmetric;
  typedef prometheus::Family<prometheus::Summary> sbuilder;
  typedef prometheus::Summary smetric;

  std::unique_ptr<prometheus::Exposer, std::default_delete<prometheus::Exposer>>
      exposer_;
  std::shared_ptr<prometheus::Registry> registry_;

  std::map<std::string, gbuilder*> gauge_;
  std::map<std::string, gmetric*> metric_;
  std::map<std::string, sbuilder*> summary_;
  std::map<std::string, smetric*> summary_metric_;
  
  void RegisterSummaryMetric(const std::string& table_name,
                              const std::string& metric_name);
};

}  // namespace resdb

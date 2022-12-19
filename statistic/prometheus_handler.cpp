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

#include "statistic/prometheus_handler.h"

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
    {NUM_EXECUTE_TX, {CONSENSUS, "num_execute_tx"}}};

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
    RegisterMetric(table_names[metric_pair.second.first],
                   metric_pair.second.second);
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

}  // namespace resdb

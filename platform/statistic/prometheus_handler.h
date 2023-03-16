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

#include <glog/logging.h>
#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

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
};

class PrometheusHandler {
 public:
  PrometheusHandler(const std::string& server_address);
  ~PrometheusHandler();

  void Set(MetricName name, double value);
  void Inc(MetricName name, double value);

 protected:
  void Register();
  void RegisterTable(const std::string& name);
  void RegisterMetric(const std::string& table_name,
                      const std::string& metric_name);

 private:
  typedef prometheus::Family<prometheus::Gauge> gbuilder;
  typedef prometheus::Gauge gmetric;

  std::unique_ptr<prometheus::Exposer, std::default_delete<prometheus::Exposer>>
      exposer_;
  std::shared_ptr<prometheus::Registry> registry_;

  std::map<std::string, gbuilder*> gauge_;
  std::map<std::string, gmetric*> metric_;
};

}  // namespace resdb

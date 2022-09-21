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

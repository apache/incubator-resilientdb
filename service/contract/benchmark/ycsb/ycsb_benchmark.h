#pragma once

#include "service/contract/benchmark/ycsb/db/contract_db.h"

namespace resdb {
namespace contract {
namespace benchmark {
namespace ycsb {

struct BenchmarkResult {
  int total_ops;
  int ktps;
  double latency_s;
  int latency_size;
  double average_retry;
  int max_retry;
  std::string db_name;
  std::string theta;
  std::string batch_size;
  std::string thread_count;
};

std::string ParseCommandLine(int argc, const char *argv[],
                             utils::Properties &props);
BenchmarkResult Process(utils::Properties props, std::unique_ptr<ycsbc::DB> db);

void PrintResult(const std::vector<BenchmarkResult> &results);

}  // namespace ycsb
}  // namespace benchmark
}  // namespace contract
}  // namespace resdb

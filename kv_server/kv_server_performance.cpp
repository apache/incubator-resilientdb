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

#include <glog/logging.h>

#include "config/resdb_config_utils.h"
#include "kv_server/kv_server_executor.h"
#include "ordering/pbft/consensus_service_pbft.h"
#include "proto/kv_server.pb.h"
#include "server/resdb_server.h"
#include "statistic/stats.h"

using resdb::ConsensusServicePBFT;
using resdb::GenerateResDBConfig;
using resdb::KVRequest;
using resdb::KVServerExecutor;
using resdb::ResDBConfig;
using resdb::ResDBServer;
using resdb::Stats;

void ShowUsage() {
  printf("<config> <private_key> <cert_file> [logging_dir]\n");
}

std::string GetRandomKey() {
  int num1 = rand() % 10;
  int num2 = rand() % 10;
  return std::to_string(num1) + std::to_string(num2);
}

int main(int argc, char** argv) {
  if (argc < 3) {
    ShowUsage();
    exit(0);
  }

  // google::InitGoogleLogging(argv[0]);
  // FLAGS_minloglevel = google::GLOG_WARNING;

  char* config_file = argv[1];
  char* private_key_file = argv[2];
  char* cert_file = argv[3];

  if (argc >= 5) {
    auto monitor_port = Stats::GetGlobalStats(5);
    monitor_port->SetPrometheus(argv[4]);
  }

  std::unique_ptr<ResDBConfig> config =
      GenerateResDBConfig(config_file, private_key_file, cert_file);

  config->RunningPerformance(true);

  auto performance_consens = std::make_unique<ConsensusServicePBFT>(
      *config,
      std::make_unique<KVServerExecutor>((*config).GetConfigData(), cert_file));
  performance_consens->SetupPerformanceDataFunc([]() {
    KVRequest request;
    request.set_cmd(KVRequest::SET);
    request.set_key(GetRandomKey());
    request.set_value("helloworld");
    std::string request_data;
    request.SerializeToString(&request_data);
    return request_data;
  });

  auto server =
      std::make_unique<ResDBServer>(*config, std::move(performance_consens));
  server->Run();
}

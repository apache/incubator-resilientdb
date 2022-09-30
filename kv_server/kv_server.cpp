#include <glog/logging.h>

#include "application/utils/server_factory.h"
#include "config/resdb_config_utils.h"
#include "kv_server/kv_server_executor.h"
#include "statistic/stats.h"

using resdb::GenerateResDBConfig;
using resdb::KVServerExecutor;
using resdb::ResConfigData;
using resdb::ResDBConfig;
using resdb::ResDBServer;
using resdb::Stats;

void ShowUsage() {
  printf(
      "<config> <private_key> <cert_file> [logging_dir]\n");
}

int main(int argc, char** argv) {
  if (argc < 4) {
    ShowUsage();
    exit(0);
  }

  char* config_file = argv[1];
  char* private_key_file = argv[2];
  char* cert_file = argv[3];
  char* logging_dir = nullptr;

  if (argc >= 6) {
    logging_dir = argv[5];
  }

  if (argc >= 5) {
    auto monitor_port = Stats::GetGlobalStats(5);
    monitor_port->SetPrometheus(argv[4]);
  }

  std::unique_ptr<ResDBConfig> config =
      GenerateResDBConfig(config_file, private_key_file, cert_file);
  ResConfigData config_data = config->GetConfigData();

  auto server = GenerateResDBServer(
      config_file, private_key_file, cert_file,
      std::make_unique<KVServerExecutor>(config_data, cert_file), logging_dir);
  server->Run();
}

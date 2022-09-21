#include "config/resdb_config_utils.h"
#include "kv_client/resdb_kv_performance_client.h"

using resdb::GenerateReplicaInfo;
using resdb::GenerateResDBConfig;
using resdb::ResDBConfig;
using resdb::ResDBKVPerformanceClient;

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("<config path>\n");
    return 0;
  }
  std::string client_config_file = argv[1];

  ResDBConfig config = GenerateResDBConfig(client_config_file);

  config.SetClientTimeoutMs(100000);

  ResDBKVPerformanceClient client(config);
  int ret = client.Start();
  printf("performance start ret %d\n", ret);
}

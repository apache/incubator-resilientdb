#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <fstream>

#include "common/proto/signature_info.pb.h"
#include "interface/kv/kv_client.h"
#include "platform/config/resdb_config_utils.h"

using resdb::GenerateReplicaInfo;
using resdb::GenerateResDBConfig;
using resdb::KVClient;
using resdb::ReplicaInfo;
using resdb::ResDBConfig;

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("<config path>\n");
    return 0;
  }
  std::string client_config_file = argv[1];
  ResDBConfig config = GenerateResDBConfig(client_config_file);

  config.SetClientTimeoutMs(100000);

  KVClient client(config);

  client.Set("start", "value");
  printf("start benchmark\n");
}
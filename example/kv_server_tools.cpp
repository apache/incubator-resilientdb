#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <fstream>

#include "config/resdb_config_utils.h"
#include "kv_client/resdb_kv_client.h"
#include "proto/signature_info.pb.h"

using resdb::GenerateReplicaInfo;
using resdb::GenerateResDBConfig;
using resdb::ReplicaInfo;
using resdb::ResDBConfig;
using resdb::ResDBKVClient;

int main(int argc, char** argv) {
  if (argc < 4) {
    printf("<config path> <cmd>(set/get), key [value]\n");
    return 0;
  }
  std::string client_config_file = argv[1];
  std::string cmd = argv[2];
  std::string key = argv[3];
  std::string value;
  if (cmd == "set") {
    value = argv[4];
  }

  ResDBConfig config = GenerateResDBConfig(client_config_file);

  config.SetClientTimeoutMs(100000);

  ResDBKVClient client(config);

  if (cmd == "set") {
    int ret = client.Set(key, value);
    printf("client set ret = %d\n", ret);
  } else {
    auto res = client.Get(key);
    if (res != nullptr) {
      printf("client get value = %s\n", res->c_str());
    } else {
      printf("client get value fail\n");
    }
  }
}

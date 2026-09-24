#include <glog/logging.h>

#include "chain/storage/memory_db.h"
#include "executor/kv/kv_executor.h"
#include "platform/config/resdb_config_utils.h"
#include "platform/consensus/ordering/2PC/framework/consensus.h"
#include "platform/networkstrate/service_network.h"
#include "proto/kv/kv.pb.h"

using namespace resdb;
using namespace resdb::storage;
using namespace resdb::twopc;
using resdb::KVRequest;

void ShowUsage() { printf("<config> <private_key> <cert_file> [logging_dir]\n"); }

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

  char* config_file = argv[1];
  char* private_key_file = argv[2];
  char* cert_file = argv[3];

  std::unique_ptr<ResDBConfig> config =
      GenerateResDBConfig(config_file, private_key_file, cert_file);

  config->RunningPerformance(true);

  auto consensus = std::make_unique<Consensus>(
      *config, std::make_unique<KVExecutor>(std::make_unique<MemoryDB>()));

  consensus->SetupPerformanceDataFunc([]() {
    KVRequest request;
    request.set_cmd(KVRequest::SET);
    request.set_key(GetRandomKey());
    request.set_value("helloworld");
    std::string request_data;
    request.SerializeToString(&request_data);
    return request_data;
  });

  auto server = std::make_unique<ServiceNetwork>(*config, std::move(consensus));
  server->Run();
}
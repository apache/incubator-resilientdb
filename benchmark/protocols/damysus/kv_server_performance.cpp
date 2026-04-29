#include <glog/logging.h>

#include "chain/storage/memory_db.h"
#include "enclave/sgx_cpp_u.h"
#include "executor/kv/kv_executor.h"
#include "platform/config/resdb_config_utils.h"
#include "platform/consensus/ordering/damysus/framework/consensus.h"
#include "platform/networkstrate/service_network.h"
#include "platform/statistic/stats.h"
#include "proto/kv/kv.pb.h"

using namespace resdb;
using namespace resdb::damysus;
using namespace resdb::storage;

oe_enclave_t* enclave = NULL;
oe_result_t result;
int ret = 0;
uint32_t flags = OE_ENCLAVE_FLAG_DEBUG;

bool check_simulate_opt(int* argc, char* argv[]) {
  for (int i = 0; i < *argc; i++) {
    if (strcmp(argv[i], "--simulate") == 0) {
      std::cout << "Running in simulation mode" << std::endl;
      memmove(&argv[i], &argv[i + 1], (*argc - i) * sizeof(char*));
      (*argc)--;
      return true;
    }
  }
  return false;
}

std::string GetRandomKey() {
  int num1 = rand() % 10;
  int num2 = rand() % 10;
  return std::to_string(num1) + std::to_string(num2);
}

int main(int argc, char** argv) {
  if (argc < 4) {
    printf("<config> <private_key> <cert_file> <enclave_file>\n");
    exit(0);
  }

  char* config_file = argv[1];
  char* private_key_file = argv[2];
  char* cert_file = argv[3];
  char* enclave_file = argv[4];

  if (check_simulate_opt(&argc, argv)) {
    flags |= OE_ENCLAVE_FLAG_SIMULATE;
  }

  result = oe_create_sgx_cpp_enclave(enclave_file, OE_ENCLAVE_TYPE_SGX, flags,
                                     NULL, 0, &enclave);
  if (result != OE_OK) {
    std::cerr << "oe_create_sgx_cpp_enclave() failed: " << result << std::endl;
    ret = 1;
  }

  if (argc >= 6) {
    auto monitor_port = Stats::GetGlobalStats(5);
    monitor_port->SetPrometheus(argv[5]);
  }

  std::unique_ptr<ResDBConfig> config =
      GenerateResDBConfig(config_file, private_key_file, cert_file);
  config->RunningPerformance(true);

  // Initialize enclave components (skip if enclave creation failed)
  if (enclave) {
    uint32_t node_id = config->GetSelfInfo().id();
    result = checker_init(enclave, &ret, &node_id);
    if (result != OE_OK || ret != 0) {
      std::cerr << "checker_init failed" << std::endl;
    }

    uint32_t seed = 123456789;
    uint32_t totalNum = config->GetReplicaNum();
    result = reset_prng(enclave, &ret, &seed, &totalNum);
  }

  auto performance_consens = std::make_unique<DamysusConsensus>(
      *config, std::make_unique<KVExecutor>(std::make_unique<MemoryDB>()), enclave);

  performance_consens->SetupPerformanceDataFunc([]() {
    KVRequest request;
    request.set_cmd(KVRequest::SET);
    request.set_key(GetRandomKey());
    request.set_value("helloworld");
    std::string request_data;
    request.SerializeToString(&request_data);
    return request_data;
  });

  auto server = std::make_unique<ServiceNetwork>(*config,
                                                  std::move(performance_consens));
  server->Run();
}

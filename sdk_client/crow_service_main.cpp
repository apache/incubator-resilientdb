#include "sdk_client/crow_service.h"

using resdb::CrowService;
using resdb::GenerateResDBConfig;
using resdb::ResDBConfig;

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("<config path>\n");
    return 0;
  }
  std::string client_config_file = argv[1];
  ResDBConfig config = GenerateResDBConfig(client_config_file);
  config.SetClientTimeoutMs(100000);

  CrowService service(config);
  service.run();
}

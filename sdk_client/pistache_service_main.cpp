#include "pistache_service.h"

using resdb::GenerateResDBConfig;
using resdb::PistacheService;
using resdb::ResDBConfig;

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("<config path>\n");
    return 0;
  }
  std::string client_config_file = argv[1];
  ResDBConfig config = GenerateResDBConfig(client_config_file);
  config.SetClientTimeoutMs(100000);

  try {
    PistacheService service(config);
    service.run();
  } catch (const std::exception& bang) {
    std::cerr << bang.what() << '\n';
    return 1;
  } catch (...) {
    return 1;
  }

  return 0;
}

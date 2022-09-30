#include <fcntl.h>
#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/mime.h>
#include <pistache/net.h>
#include <pistache/router.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config/resdb_config_utils.h"
#include "kv_client/resdb_kv_client.h"
#include "proto/signature_info.pb.h"
#include "sdk_client/sdk_transaction.h"

namespace resdb {

class PistacheService {
 public:
  PistacheService(ResDBConfig config, uint16_t portNum = 8000,
                  unsigned int numThreads = std::thread::hardware_concurrency())
      : config_(config),
        portNum_(portNum),
        numThreads_(numThreads),
        address_("localhost", portNum),
        endpoint_(std::make_shared<Pistache::Http::Endpoint>(address_)) {}

  void run();

 private:
  void configureRoutes();

  using Request = Pistache::Rest::Request;
  using Response = Pistache::Http::ResponseWriter;
  void commit(const Request& request, Response response);
  void getAssets(const Request& request, Response response);

  ResDBConfig config_;
  uint16_t portNum_;
  unsigned int numThreads_;
  Pistache::Address address_;
  std::shared_ptr<Pistache::Http::Endpoint> endpoint_;
  Pistache::Rest::Router router_;
};

}  // namespace resdb

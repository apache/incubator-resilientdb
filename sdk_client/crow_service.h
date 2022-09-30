#pragma once

#include "config/resdb_config_utils.h"
#include "crow.h"
#include "kv_client/resdb_kv_client.h"
#include "proto/signature_info.pb.h"
#include "sdk_client/sdk_transaction.h"

namespace resdb {

class CrowService {
 public:
  CrowService(ResDBConfig config, uint16_t port_num = 18000);
  void run();

 private:
  ResDBConfig config_;
  uint16_t port_num_;
};

}  // namespace resdb

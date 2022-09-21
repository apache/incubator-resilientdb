#pragma once

#include "kv_client/resdb_kv_client.h"

namespace resdb {

class ResDBKVPerformanceClient : public ResDBKVClient {
 public:
  ResDBKVPerformanceClient(const ResDBConfig& config);
  int Start();
};

}  // namespace resdb

#include "kv_client/resdb_kv_performance_client.h"

#include "proto/kv_server.pb.h"

namespace resdb {

ResDBKVPerformanceClient::ResDBKVPerformanceClient(const ResDBConfig& config)
    : ResDBKVClient(config) {}

int ResDBKVPerformanceClient::Start() {
  KVRequest request;
  return SendRequest(request);
}

}  // namespace resdb

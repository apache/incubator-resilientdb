#pragma once

#include "client/resdb_user_client.h"

namespace resdb {

// ResDBKVClient to send data to the kv server.
class ResDBKVClient : public ResDBUserClient {
 public:
  ResDBKVClient(const ResDBConfig& config);

  int Set(const std::string& key, const std::string& data);
  std::unique_ptr<std::string> Get(const std::string& key);
};

}  // namespace resdb

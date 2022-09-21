#pragma once

#include "client/resdb_client.h"
#include "proto/resdb.pb.h"

namespace resdb {

struct Context {
  std::unique_ptr<ResDBClient> client;
  SignatureInfo signature;
};

}  // namespace resdb

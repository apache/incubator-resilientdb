#pragma once
#include "proto/replica_info.pb.h"
#include "proto/resdb.pb.h"

namespace resdb {

enum CollectorResultCode {
  INVALID = -2,
  OK = 0,
  STATE_CHANGED = 1,
};

std::unique_ptr<Request> NewRequest(Request::Type type, const Request& request,
                                    int sender_id);

std::unique_ptr<Request> NewRequest(Request::Type type, const Request& request,
                                    int sender_id, int region_info);

}  // namespace resdb

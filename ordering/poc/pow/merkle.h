#pragma once

#include "ordering/poc/proto/pow.pb.h"

namespace resdb {

class Merkle {
 public:
  static HashValue MakeHash(const BatchClientTransactions& transaction);
};

}  // namespace resdb

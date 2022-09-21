#pragma once

#include "proto/signature_info.pb.h"

namespace resdb {

class KeyGenerator {
 public:
  static SecretKey GeneratorKeys(SignatureInfo::HashType type);
};

}  // namespace resdb

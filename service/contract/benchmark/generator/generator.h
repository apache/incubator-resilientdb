#pragma once

#include <stdint.h>

namespace resdb {

class Generator {
 public:
  Generator() = default;
  virtual ~Generator() = default;

  virtual uint64_t Next() = 0;
};

}  // namespace resdb

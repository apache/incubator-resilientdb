#pragma once

#include <stdint.h>

#include "service/contract/benchmark/generator/generator.h"

namespace resdb {

class ZipfianGenerator : public Generator {
 public:
  ZipfianGenerator(uint64_t num_items, double alpha);

  uint64_t Next() override;

 private:
  uint64_t total_;
  double alpha_;
  double zeta_;
};

}  // namespace resdb

#pragma once

#include <stdint.h>

#include <random>

#include "service/contract/benchmark/generator/generator.h"

namespace resdb {

class UniformGenerator : public Generator {
 public:
  UniformGenerator(uint64_t num_items);

  uint64_t Next() override;

 private:
  std::mt19937_64 generator_;
  std::uniform_int_distribution<uint64_t> dist_;
};

}  // namespace resdb

#include "service/contract/benchmark/uniform_generator.h"

namespace resdb {
UniformGenerator::UniformGenerator(uint64_t num_items) : dist_(1, num_items) {}

uint64_t UniformGenerator::Next() {
  return dist_(generator_);
  ;
}

}  // namespace resdb

#include "service/contract/benchmark/generator/zipfian_generator.h"

#include <assert.h>
#include <math.h>

#include <random>

namespace resdb {

double RandomDouble(double min = 0.0, double max = 1.0) {
  static std::default_random_engine generator;
  static std::uniform_real_distribution<double> uniform(min, max);
  return uniform(generator);
}

ZipfianGenerator::ZipfianGenerator(uint64_t num_items, double alpha)
    : total_(num_items), alpha_(alpha) {
  zeta_ = 0;
  for (int i = 1; i <= total_; i++)
    zeta_ = zeta_ + (1.0 / pow((double)i, alpha_));
  zeta_ = 1.0 / zeta_;
}

uint64_t ZipfianGenerator::Next() {
  double u = RandomDouble();

  double sum_prob = 0;
  // Map z to the value
  for (int i = 1; i <= total_; i++) {
    sum_prob = sum_prob + zeta_ / pow((double)i, alpha_);
    if (sum_prob >= u) {
      return i;
    }
  }
  return 1;
}

}  // namespace resdb

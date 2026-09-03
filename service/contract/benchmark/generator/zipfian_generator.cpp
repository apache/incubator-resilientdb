/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
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

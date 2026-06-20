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

// Microbenchmark: prefix-scan via composite keys vs full scan + app-side
// filter. Both paths run against the same LevelDB instance.

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "chain/storage/composite_key_codec.h"
#include "chain/storage/leveldb.h"

using ::resdb::storage::EncodeCompositeKey;
using ::resdb::storage::EncodeCompositeKeyPrefix;
using ::resdb::storage::NewResLevelDB;
using ::resdb::Storage;
using Clock = std::chrono::high_resolution_clock;

namespace {

constexpr const char* kIndexName = "byCity";
constexpr const char* kTargetCity = "Davis";
constexpr const char* kOtherCity = "Other";

struct Row {
  int n_records;
  int selectivity_pct;
  int matching;
  double old_way_ms;
  double new_way_ms;
};

// `matching` records get city="Davis"; the rest get city="Other". Every
// record also gets a composite-key index entry.
void SeedDatabase(Storage* storage, int n, int matching) {
  for (int i = 0; i < n; ++i) {
    std::string key = "user:" + std::to_string(i);
    std::string city = (i < matching) ? kTargetCity : kOtherCity;
    storage->SetValueWithVersion(key, city, 0);
    storage->CreateCompositeKey(EncodeCompositeKey(kIndexName, {city}, key));
  }
}

// OLD path: scan everything, filter in code.
int CountByScan(Storage* storage) {
  auto all = storage->GetAllItems();
  int count = 0;
  for (const auto& kv : all) {
    if (kv.second.first == kTargetCity) ++count;
  }
  return count;
}

// NEW path: prefix scan the byCity:Davis index.
int CountByPrefixScan(Storage* storage) {
  return storage
      ->GetByCompositeKeyPrefix(
          EncodeCompositeKeyPrefix(kIndexName, {kTargetCity}))
      .size();
}

template <typename F>
double TimeMs(F&& f) {
  auto start = Clock::now();
  f();
  auto end = Clock::now();
  return std::chrono::duration<double, std::milli>(end - start).count();
}

Row RunOne(int n, int selectivity_pct, int iters) {
  int matching = (n * selectivity_pct) / 100;

  std::string path = "/tmp/composite_key_bench_n" + std::to_string(n) + "_s" +
                     std::to_string(selectivity_pct);
  std::filesystem::remove_all(path);
  auto storage = NewResLevelDB(path);

  SeedDatabase(storage.get(), n, matching);

  // Warmup pass.
  int warm_a = CountByScan(storage.get());
  int warm_b = CountByPrefixScan(storage.get());
  if (warm_a != matching || warm_b != matching) {
    std::cerr << "WARN: warmup counts disagree: scan=" << warm_a
              << " prefix=" << warm_b << " expected=" << matching << "\n";
  }

  double old_total = 0, new_total = 0;
  for (int i = 0; i < iters; ++i) {
    old_total += TimeMs([&] { CountByScan(storage.get()); });
    new_total += TimeMs([&] { CountByPrefixScan(storage.get()); });
  }

  return Row{n, selectivity_pct, matching, old_total / iters,
             new_total / iters};
}

void PrintTable(const std::vector<Row>& rows) {
  std::cout
      << "\n============================================================"
      << "================\n";
  std::cout << "Composite Key Benchmark -- ResilientDB Storage Layer\n";
  std::cout << "  OLD: GetAllItems() + filter in app code     (expected O(N))\n";
  std::cout << "  NEW: GetByCompositeKeyPrefix()              (expected O(log N + M))\n";
  std::cout
      << "============================================================"
      << "================\n";
  std::cout << std::left << std::setw(10) << "Records" << std::setw(13)
            << "Selectivity" << std::setw(11) << "Matching" << std::setw(13)
            << "OLD (ms)" << std::setw(13) << "NEW (ms)" << "Speedup\n";
  std::cout
      << "------------------------------------------------------------"
      << "----------------\n";
  for (const auto& r : rows) {
    double speedup = (r.new_way_ms > 0) ? r.old_way_ms / r.new_way_ms : 0;
    std::cout << std::left << std::setw(10) << r.n_records << std::setw(13)
              << (std::to_string(r.selectivity_pct) + "%") << std::setw(11)
              << r.matching << std::setw(13) << std::fixed
              << std::setprecision(2) << r.old_way_ms << std::setw(13)
              << r.new_way_ms << std::fixed << std::setprecision(1) << speedup
              << "x\n";
  }
  std::cout
      << "============================================================"
      << "================\n\n";
}

}  // namespace

int main(int /*argc*/, char** /*argv*/) {
  std::cout << "Running composite key benchmark...\n";
  std::cout << "(Each (N, selectivity) pair is averaged over 5 iterations "
            << "after a warmup.)\n\n";

  const int iters = 5;
  std::vector<Row> rows;
  for (int n : {1000, 10000, 100000}) {
    for (int sel : {1, 10}) {
      std::cout << "  N=" << n << "  selectivity=" << sel << "% ..."
                << std::flush;
      rows.push_back(RunOne(n, sel, iters));
      std::cout << " done.\n";
    }
  }

  PrintTable(rows);
  return 0;
}

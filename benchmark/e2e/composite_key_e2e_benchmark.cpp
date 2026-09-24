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

// End-to-end benchmark for secondary indexes (issue #180), measured against a
// running cluster rather than the storage layer. Four ways to answer the same
// question, "which records have city = Davis?":
//
//   1. scan     every record is fetched and filtered by the application
//   2. index    one query, primary keys only
//   3. index+get  one query, then one get per match
//   4. index+values  one query that returns the records (--with_values)
//
// Usage (from the repository root, with a cluster already running):
//   bazel run //benchmark/e2e:composite_key_e2e_benchmark -- \
//       --config $PWD/service/tools/config/interface/service.config \
//       --records 1000 --selectivity 10 --repeat 3
//
// --config takes any client config, so the same binary measures a local
// cluster or a multi-machine deployment.

#include <getopt.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

#include "interface/kv/kv_client.h"
#include "platform/config/resdb_config_utils.h"

namespace {

using resdb::GenerateResDBConfig;
using resdb::Items;
using resdb::KVClient;
using resdb::ResDBConfig;

// Each run uses its own index name, so entries from earlier runs on the
// same cluster are not counted.
std::string IndexName(const std::string& run_id) {
  return "bench_by_city_" + run_id;
}
constexpr char kTargetCity[] = "Davis";
constexpr char kOtherCity[] = "Other";

double MillisSince(std::chrono::steady_clock::time_point start) {
  auto elapsed = std::chrono::steady_clock::now() - start;
  return std::chrono::duration<double, std::milli>(elapsed).count();
}

std::string RecordKey(const std::string& run_id, int i) {
  return "bench:" + run_id + ":" + std::to_string(i);
}

// Every record carries its city, so the scan has something to filter on.
// `padding` makes the record the size a real document would be: the scan has
// to transfer every record, the index query only the matching ones.
std::string RecordValue(const std::string& city, int i, int value_size) {
  std::string value =
      "{\"city\":\"" + city + "\",\"n\":" + std::to_string(i) + ",\"data\":\"";
  if (static_cast<int>(value.size()) + 2 < value_size) {
    value.append(value_size - value.size() - 2, 'x');
  }
  value.append("\"}");
  return value;
}

// The proxy batches client requests (100 requests or 100ms, whichever comes
// first), so a single client sending one request at a time pays the full wait
// every time. Loading uses several clients at once so the batches fill.
int Load(const ResDBConfig& config, KVClient* client, const std::string& run_id,
         int records, int selectivity, int threads, int value_size) {
  int matching = 0;
  for (int i = 0; i < records; ++i) {
    bool is_match = (i % 100) < selectivity;
    const std::string city = is_match ? kTargetCity : kOtherCity;
    if (is_match) {
      ++matching;
    }
    client->Set(RecordKey(run_id, i), RecordValue(city, i, value_size));
  }
  // Set does not wait for consensus, so confirm the last write landed before
  // indexing: CreateIndexEntry refuses records that don't exist yet.
  for (int attempt = 0; attempt < 100; ++attempt) {
    auto value = client->Get(RecordKey(run_id, records - 1));
    if (value != nullptr && !value->empty()) {
      break;
    }
  }
  std::atomic<int> failed(0);
  std::vector<std::thread> workers;
  for (int t = 0; t < threads; ++t) {
    workers.emplace_back([&, t]() {
      ResDBConfig thread_config = config;
      KVClient thread_client(thread_config);
      for (int i = t; i < records; i += threads) {
        bool is_match = (i % 100) < selectivity;
        const std::string city = is_match ? kTargetCity : kOtherCity;
        if (thread_client.CreateIndexEntry(IndexName(run_id), {city},
                                           RecordKey(run_id, i)) != 0) {
          ++failed;
        }
      }
    });
  }
  for (std::thread& worker : workers) {
    worker.join();
  }
  if (failed > 0) {
    printf("  WARNING: %d index entries could not be created\n", failed.load());
  }
  return matching;
}

// 1. What an application has to do today: fetch everything, filter locally.
double ScanAndFilter(KVClient* client, const std::string& run_id, int* found) {
  auto start = std::chrono::steady_clock::now();
  std::unique_ptr<Items> all =
      client->GetKeyRange("bench:" + run_id + ":", "bench:" + run_id + ";");
  int matches = 0;
  if (all != nullptr) {
    const std::string needle = std::string("\"city\":\"") + kTargetCity + "\"";
    for (const auto& item : all->item()) {
      if (item.value_info().value().find(needle) != std::string::npos) {
        ++matches;
      }
    }
  }
  double ms = MillisSince(start);
  *found = matches;
  return ms;
}

// 2. One query, primary keys only.
double QueryKeysOnly(KVClient* client, const std::string& run_id,
                     int* found) {
  auto start = std::chrono::steady_clock::now();
  std::unique_ptr<Items> keys =
      client->QueryByIndex(IndexName(run_id), {kTargetCity});
  double ms = MillisSince(start);
  *found = keys == nullptr ? -1 : keys->item_size();
  return ms;
}

// 3. One query, then one get per match. Each get is its own consensus round.
double QueryThenGet(KVClient* client, const std::string& run_id,
                    int* found) {
  auto start = std::chrono::steady_clock::now();
  std::unique_ptr<Items> keys =
      client->QueryByIndex(IndexName(run_id), {kTargetCity});
  int fetched = 0;
  if (keys != nullptr) {
    for (const auto& item : keys->item()) {
      auto value = client->Get(item.key());
      if (value != nullptr && !value->empty()) {
        ++fetched;
      }
    }
  }
  double ms = MillisSince(start);
  *found = fetched;
  return ms;
}

// 4. One query that returns the records themselves.
double QueryWithValues(KVClient* client, const std::string& run_id,
                       int* found) {
  auto start = std::chrono::steady_clock::now();
  std::unique_ptr<Items> records =
      client->QueryByIndex(IndexName(run_id), {kTargetCity}, true);
  int with_value = 0;
  if (records != nullptr) {
    for (const auto& item : records->item()) {
      if (!item.value_info().value().empty()) {
        ++with_value;
      }
    }
  }
  double ms = MillisSince(start);
  *found = with_value;
  return ms;
}

void ShowUsage() {
  printf(
      "--config path to the client config (required)\n"
      "--records how many records to load (default 1000)\n"
      "--selectivity percent of records that match, 1-100 (default 10)\n"
      "--repeat how many times to time each approach (default 3)\n"
      "--threads clients used while loading (default 16)\n"
      "--value_size bytes per record, like a real document (default 64)\n");
}

}  // namespace

int main(int argc, char** argv) {
  std::string config_file;
  int records = 1000;
  int selectivity = 10;
  int repeat = 3;
  int threads = 16;
  int value_size = 64;

  static struct option long_options[] = {
      {"config", required_argument, NULL, 'c'},
      {"records", required_argument, NULL, 'r'},
      {"selectivity", required_argument, NULL, 's'},
      {"repeat", required_argument, NULL, 'n'},
      {"threads", required_argument, NULL, 't'},
      {"value_size", required_argument, NULL, 'v'},
      {NULL, 0, NULL, 0},
  };
  int option_index = 0;
  int c;
  while ((c = getopt_long(argc, argv, "", long_options, &option_index)) != -1) {
    switch (c) {
      case 'c':
        config_file = optarg;
        break;
      case 'r':
        records = atoi(optarg);
        break;
      case 's':
        selectivity = atoi(optarg);
        break;
      case 'n':
        repeat = atoi(optarg);
        break;
      case 't':
        threads = atoi(optarg);
        break;
      case 'v':
        value_size = atoi(optarg);
        break;
      default:
        ShowUsage();
        return 1;
    }
  }
  if (config_file.empty() || records <= 0 || selectivity <= 0 ||
      selectivity > 100 || repeat <= 0) {
    ShowUsage();
    return 1;
  }

  ResDBConfig config = GenerateResDBConfig(config_file);
  config.SetClientTimeoutMs(100000);
  KVClient client(config);

  // A fresh key range per run, so repeated runs don't read each other's data.
  const std::string run_id =
      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count() %
                     100000);

  printf("\nrecords=%d selectivity=%d%% value_size=%dB repeat=%d run_id=%s\n",
         records, selectivity, value_size, repeat, run_id.c_str());
  printf("loading...\n");
  auto load_start = std::chrono::steady_clock::now();
  int expected =
      Load(config, &client, run_id, records, selectivity, threads, value_size);
  printf("loaded %d records (%d matching) in %.0f ms\n\n", records, expected,
         MillisSince(load_start));

  struct Result {
    const char* name;
    double best = 1e18;
    int found = 0;
  };
  Result results[4] = {{"scan + filter in the app"},
                       {"index query (keys only)"},
                       {"index query + one get per match"},
                       {"index query with values"}};

  for (int i = 0; i < repeat; ++i) {
    int found = 0;
    double ms = ScanAndFilter(&client, run_id, &found);
    results[0].best = std::min(results[0].best, ms);
    results[0].found = found;

    ms = QueryKeysOnly(&client, run_id, &found);
    results[1].best = std::min(results[1].best, ms);
    results[1].found = found;

    ms = QueryThenGet(&client, run_id, &found);
    results[2].best = std::min(results[2].best, ms);
    results[2].found = found;

    ms = QueryWithValues(&client, run_id, &found);
    results[3].best = std::min(results[3].best, ms);
    results[3].found = found;
  }

  printf("%-34s %10s %10s %8s\n", "approach", "time (ms)", "speedup", "found");
  printf("%-34s %10s %10s %8s\n", "----------------------------------",
         "----------", "----------", "--------");
  for (const Result& r : results) {
    printf("%-34s %10.1f %9.1fx %8d\n", r.name, r.best,
           results[0].best / r.best, r.found);
  }
  printf("\nfastest of %d runs; %d records, %d expected matches\n\n", repeat,
         records, expected);

  bool ok = results[0].found == expected && results[1].found == expected &&
            results[2].found == expected && results[3].found == expected;
  if (!ok) {
    printf("WARNING: the four approaches did not agree on the match count\n");
    return 1;
  }
  return 0;
}

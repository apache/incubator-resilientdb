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

#include <benchmark/benchmark.h>
#include <glog/logging.h>

#include <filesystem>
#include <nlohmann/json.hpp>
#include <random>
#include <string>
#include <vector>

#include "chain/storage/leveldb.h"
#include "chain/storage/rocksdb.h"
#include "chain/storage/storage.h"
#include "executor/kv/kv_executor.h"
#include "proto/kv/kv.pb.h"

namespace resdb {
namespace {

using json = nlohmann::json;

DEFINE_string(storage_type, "leveldb",
              "Storage type to use: mem, leveldb, leveldb_cache, rocksdb");

class LogSuppressor {
 public:
  LogSuppressor() {
    FLAGS_minloglevel = 3;
    FLAGS_logtostderr = false;
  }
};

std::string ExtractFieldValue(const std::string& json_str,
                              const std::string& field) {
  try {
    auto j = json::parse(json_str);
    if (j.find(field) != j.end()) {
      if (j[field].is_string()) {
        return j[field].get<std::string>();
      } else if (j[field].is_number()) {
        return std::to_string(j[field].get<int>());
      } else if (j[field].is_boolean()) {
        return j[field].get<bool>() ? "true" : "false";
      }
    }
  } catch (const std::exception& e) {
    // Return empty string on parsing error
  }
  return "";
}

int SetValue(KVExecutor* executor, const std::string& key,
             const std::string& value) {
  KVRequest request;
  request.set_cmd(KVRequest::SET);
  request.set_key(key);
  request.set_value(value);

  std::string str;
  if (!request.SerializeToString(&str)) {
    return -1;
  }

  executor->ExecuteData(str);
  return 0;
}

std::string GetValue(KVExecutor* executor, const std::string& key) {
  KVRequest request;
  request.set_cmd(KVRequest::GET);
  request.set_key(key);

  std::string str;
  if (!request.SerializeToString(&str)) {
    return "";
  }
  auto resp = executor->ExecuteData(str);
  if (resp == nullptr) {
    return "";
  }
  KVResponse kv_response;
  if (!kv_response.ParseFromString(*resp)) {
    return "";
  }
  return kv_response.value();
}

int CreateCompositeKey(KVExecutor* executor, const std::string& primary_key,
                       const std::string& field_name,
                       const std::string& field_value, int field_type) {
  KVRequest request;
  request.set_cmd(KVRequest::CREATE_COMPOSITE_KEY);
  request.set_key(primary_key);
  request.set_field_name(field_name);
  request.set_value(field_value);
  request.set_field_type(field_type);

  std::string str;
  if (!request.SerializeToString(&str)) {
    return -1;
  }

  executor->ExecuteData(str);
  return 0;
}

std::vector<std::string> GetByCompositeKey(KVExecutor* executor,
                                           const std::string& field_name,
                                           const std::string& field_value,
                                           int field_type) {
  KVRequest request;
  request.set_cmd(KVRequest::GET_BY_COMPOSITE_KEY);
  request.set_field_name(field_name);
  request.set_value(field_value);
  request.set_field_type(field_type);

  std::string str;
  if (!request.SerializeToString(&str)) {
    return std::vector<std::string>();
  }

  auto resp = executor->ExecuteData(str);
  if (resp == nullptr) {
    return std::vector<std::string>();
  }
  KVResponse kv_response;
  if (!kv_response.ParseFromString(*resp)) {
    return std::vector<std::string>();
  }

  std::vector<std::string> results;
  for (const auto& item : kv_response.items().item()) {
    results.push_back(item.value_info().value());
  }
  return results;
}

std::vector<std::string> GetByCompositeKeyRange(KVExecutor* executor,
                                                const std::string& field_name,
                                                const std::string& min_value,
                                                const std::string& max_value,
                                                int field_type) {
  KVRequest request;
  request.set_cmd(KVRequest::GET_COMPOSITE_KEY_RANGE);
  request.set_field_name(field_name);
  request.set_key(min_value);
  request.set_value(max_value);
  request.set_field_type(field_type);

  std::string str;
  if (!request.SerializeToString(&str)) {
    return std::vector<std::string>();
  }

  auto resp = executor->ExecuteData(str);
  if (resp == nullptr) {
    return std::vector<std::string>();
  }
  KVResponse kv_response;
  if (!kv_response.ParseFromString(*resp)) {
    return std::vector<std::string>();
  }

  std::vector<std::string> results;
  for (const auto& item : kv_response.items().item()) {
    results.push_back(item.value_info().value());
  }
  return results;
}

class CompositeKeyBenchmark : public benchmark::Fixture {
 protected:
  void SetUp(const benchmark::State& state) override {
    static LogSuppressor log_suppressor;

    if(std::filesystem::exists("/tmp/nexres-leveldb")) {
      std::filesystem::remove_all("/tmp/nexres-leveldb");
    }
    if(std::filesystem::exists("/tmp/nexres-rocksdb")) {
      std::filesystem::remove_all("/tmp/nexres-rocksdb");
    }

    std::unique_ptr<Storage> storage;
    std::string type = FLAGS_storage_type;
    std::transform(type.begin(), type.end(), type.begin(), ::tolower);

    if (type == "leveldb") {
      storage = storage::NewResLevelDB(std::nullopt);
    } else if (type == "leveldb_cache") {
      storage::LevelDBInfo config;
      config.set_enable_block_cache(true);
      storage = storage::NewResLevelDB(config);
    } else if (type == "rocksdb") {
      storage = storage::NewResRocksDB(std::nullopt);
    } else {
      storage = storage::NewResLevelDB(std::nullopt);
    }

    storage_ptr_ = storage.get();
    executor_ = std::make_unique<KVExecutor>(std::move(storage));
  }

  void TearDown(const benchmark::State& state) override {
    executor_.reset();
    storage_ptr_ = nullptr;
    std::filesystem::remove_all("/tmp/nexres-leveldb");
    std::filesystem::remove_all("/tmp/nexres-rocksdb");
  }

  std::unique_ptr<KVExecutor> executor_;
  Storage* storage_ptr_;
};

BENCHMARK_DEFINE_F(CompositeKeyBenchmark,
                   StringFieldInMemory)(benchmark::State& state) {
  const int num_records = state.range(0);

  std::vector<std::string> string_values = {"Alice", "Bob", "Charlie", "David",
                                            "Eve"};
  std::vector<std::string> target_strings = {"Alice", "Charlie"};

  for (int i = 0; i < num_records; i++) {
    std::string name = string_values[i % string_values.size()];
    std::string json =
        "{\"name\":\"" + name + "\",\"id\":" + std::to_string(i) + "}";
    SetValue(executor_.get(), "record_" + std::to_string(i), json);
  }

  for (auto _ : state) {
    int total_matches = 0;
    for (const auto& target : target_strings) {
      for (int i = 0; i < num_records; i++) {
        std::string json =
            GetValue(executor_.get(), "record_" + std::to_string(i));
        if (!json.empty()) {
          std::string name = ExtractFieldValue(json, "name");
          if (name == target) {
            total_matches++;
          }
        }
      }
    }
    benchmark::DoNotOptimize(total_matches);
  }

  state.SetItemsProcessed(state.iterations() * target_strings.size());
  state.counters["records_scanned_per_query"] = num_records;
  state.counters["total_records_scanned"] =
      state.iterations() * target_strings.size() * num_records;
}

BENCHMARK_DEFINE_F(CompositeKeyBenchmark,
                   StringFieldComposite)(benchmark::State& state) {
  const int num_records = state.range(0);

  std::vector<std::string> string_values = {"Alice", "Bob", "Charlie", "David",
                                            "Eve"};
  std::vector<std::string> target_strings = {"Alice", "Charlie"};

  for (int i = 0; i < num_records; i++) {
    std::string name = string_values[i % string_values.size()];
    std::string json =
        "{\"name\":\"" + name + "\",\"id\":" + std::to_string(i) + "}";
    SetValue(executor_.get(), "record_" + std::to_string(i), json);
    CreateCompositeKey(executor_.get(), "record_" + std::to_string(i), "name",
                       name, 0);  // STRING = 0
  }

  for (auto _ : state) {
    int total_matches = 0;
    for (const auto& target : target_strings) {
      std::vector<std::string> results =
          GetByCompositeKey(executor_.get(), "name", target, 0);
      total_matches += results.size();
    }
    benchmark::DoNotOptimize(total_matches);
  }

  state.SetItemsProcessed(state.iterations() * target_strings.size());
  state.counters["records_scanned_per_query"] = 1;  // Direct lookup
  state.counters["total_records_scanned"] =
      state.iterations() * target_strings.size();
}

BENCHMARK_DEFINE_F(CompositeKeyBenchmark,
                   IntegerFieldInMemory)(benchmark::State& state) {
  const int num_records = state.range(0);

  std::vector<int> age_values = {25, 30, 35, 40, 45};
  std::vector<int> target_ages = {30, 40};

  for (int i = 0; i < num_records; i++) {
    int age = age_values[i % age_values.size()];
    std::string json = "{\"age\":" + std::to_string(age) +
                       ",\"id\":" + std::to_string(i) + "}";
    SetValue(executor_.get(), "record_" + std::to_string(i), json);
  }

  for (auto _ : state) {
    int total_matches = 0;
    for (const auto& target : target_ages) {
      for (int i = 0; i < num_records; i++) {
        std::string json =
            GetValue(executor_.get(), "record_" + std::to_string(i));
        if (!json.empty()) {
          std::string age_str = ExtractFieldValue(json, "age");
          if (!age_str.empty()) {
            int age = std::stoi(age_str);
            if (age == target) {
              total_matches++;
            }
          }
        }
      }
    }
    benchmark::DoNotOptimize(total_matches);
  }

  state.SetItemsProcessed(state.iterations() * target_ages.size());
  state.counters["records_scanned_per_query"] = num_records;
  state.counters["total_records_scanned"] =
      state.iterations() * target_ages.size() * num_records;
}

BENCHMARK_DEFINE_F(CompositeKeyBenchmark,
                   IntegerFieldComposite)(benchmark::State& state) {
  const int num_records = state.range(0);

  std::vector<int> age_values = {25, 30, 35, 40, 45};
  std::vector<int> target_ages = {30, 40};

  for (int i = 0; i < num_records; i++) {
    int age = age_values[i % age_values.size()];
    std::string json = "{\"age\":" + std::to_string(age) +
                       ",\"id\":" + std::to_string(i) + "}";
    SetValue(executor_.get(), "record_" + std::to_string(i), json);
    CreateCompositeKey(executor_.get(), "record_" + std::to_string(i), "age",
                       std::to_string(age), 1);  // INTEGER = 1
  }

  for (auto _ : state) {
    int total_matches = 0;
    for (const auto& target : target_ages) {
      std::vector<std::string> results =
          GetByCompositeKey(executor_.get(), "age", std::to_string(target), 1);
      total_matches += results.size();
    }
    benchmark::DoNotOptimize(total_matches);
  }

  state.SetItemsProcessed(state.iterations() * target_ages.size());
  state.counters["records_scanned_per_query"] = 1;
  state.counters["total_records_scanned"] =
      state.iterations() * target_ages.size();
}

BENCHMARK_DEFINE_F(CompositeKeyBenchmark,
                   RangeQueryInMemory)(benchmark::State& state) {
  const int num_records = state.range(0);

  std::vector<int> age_values = {25, 30, 35, 40, 45};

  for (int i = 0; i < num_records; i++) {
    int age = age_values[i % age_values.size()];
    std::string json = "{\"age\":" + std::to_string(age) +
                       ",\"id\":" + std::to_string(i) + "}";
    SetValue(executor_.get(), "record_" + std::to_string(i), json);
  }

  for (auto _ : state) {
    int total_matches = 0;
    int min_age = 30;
    int max_age = 40;

    for (int i = 0; i < num_records; i++) {
      std::string json =
          GetValue(executor_.get(), "record_" + std::to_string(i));
      if (!json.empty()) {
        std::string age_str = ExtractFieldValue(json, "age");
        if (!age_str.empty()) {
          int age = std::stoi(age_str);
          if (age >= min_age && age <= max_age) {
            total_matches++;
          }
        }
      }
    }
    benchmark::DoNotOptimize(total_matches);
  }

  state.SetItemsProcessed(state.iterations() * num_records);
}

BENCHMARK_DEFINE_F(CompositeKeyBenchmark,
                   RangeQueryComposite)(benchmark::State& state) {
  const int num_records = state.range(0);

  std::vector<int> age_values = {25, 30, 35, 40, 45};

  for (int i = 0; i < num_records; i++) {
    int age = age_values[i % age_values.size()];
    std::string json = "{\"age\":" + std::to_string(age) +
                       ",\"id\":" + std::to_string(i) + "}";
    SetValue(executor_.get(), "record_" + std::to_string(i), json);
    CreateCompositeKey(executor_.get(), "record_" + std::to_string(i), "age",
                       std::to_string(age), 1);  // INTEGER = 1
  }

  for (auto _ : state) {
    std::vector<std::string> results =
        GetByCompositeKeyRange(executor_.get(), "age", "30", "40", 1);
    benchmark::DoNotOptimize(results.size());
  }

  state.SetItemsProcessed(state.iterations());
}

BENCHMARK_DEFINE_F(CompositeKeyBenchmark,
                   PrimaryKeyOnly_Rand)(benchmark::State& state) {
  const int num_records = state.range(0);

  for (int i = 0; i < num_records; i++) {
    std::string json = "{\"id\":" + std::to_string(i) + ",\"data\":\"record_" +
                       std::to_string(i) + "\"}";
    SetValue(executor_.get(), "record_" + std::to_string(i), json);
  }

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dist(0, num_records - 1);

  for (auto _ : state) {
    for (int i = 0; i < num_records; i++) {
      int random_id = dist(gen);
      std::string result =
          GetValue(executor_.get(), "record_" + std::to_string(random_id));
      benchmark::DoNotOptimize(result);
    }
  }

  state.SetItemsProcessed(state.iterations() * num_records);
  state.counters["total_records"] = num_records;
  state.counters["composite_keys"] = 0;

  TearDown(state);
}

BENCHMARK_DEFINE_F(CompositeKeyBenchmark,
                   PrimaryKeyWithCompositeKeys_25AF_Rand)(benchmark::State& state) {
  const int num_records = state.range(0);

  for (int i = 0; i < num_records; i++) {
    std::string json = "{\"id\":" + std::to_string(i) + ",\"data\":\"record_" +
                       std::to_string(i) + "\"}";
    SetValue(executor_.get(), "record_" + std::to_string(i), json);

    if (i % 4 == 0) {
      CreateCompositeKey(executor_.get(), "record_" + std::to_string(i), "id",
                         std::to_string(i), 1);
    }
  }
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dist(0, num_records - 1);

  for (auto _ : state) {
    for (int i = 0; i < num_records; i++) {
      int random_id = dist(gen);
      std::string result =
          GetValue(executor_.get(), "record_" + std::to_string(random_id));
      benchmark::DoNotOptimize(result);
    }
  }

  state.SetItemsProcessed(state.iterations() * num_records);
  state.counters["total_records"] = num_records;
  state.counters["composite_keys"] = num_records;

  TearDown(state);
}

BENCHMARK_DEFINE_F(CompositeKeyBenchmark,
                   PrimaryKeyWithCompositeKeys_50AF_Rand)(benchmark::State& state) {
  const int num_records = state.range(0);

  for (int i = 0; i < num_records; i++) {
    std::string json = "{\"id\":" + std::to_string(i) + ",\"data\":\"record_" +
                       std::to_string(i) + "\"}";
    SetValue(executor_.get(), "record_" + std::to_string(i), json);

    if (i % 2 == 0) {
      CreateCompositeKey(executor_.get(), "record_" + std::to_string(i), "id",
                         std::to_string(i), 1);
    }
  }
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dist(0, num_records - 1);

  for (auto _ : state) {
    for (int i = 0; i < num_records; i++) {
      int random_id = dist(gen);
      std::string result =
          GetValue(executor_.get(), "record_" + std::to_string(random_id));
      benchmark::DoNotOptimize(result);
    }
  }

  state.SetItemsProcessed(state.iterations() * num_records);
  state.counters["total_records"] = num_records;
  state.counters["composite_keys"] = num_records;

  TearDown(state);
}

BENCHMARK_DEFINE_F(CompositeKeyBenchmark,
                   PrimaryKeyWithCompositeKeys_100AF_Rand)(benchmark::State& state) {
  const int num_records = state.range(0);

  for (int i = 0; i < num_records; i++) {
    std::string json = "{\"id\":" + std::to_string(i) + ",\"data\":\"record_" +
                       std::to_string(i) + "\"}";
    SetValue(executor_.get(), "record_" + std::to_string(i), json);

    CreateCompositeKey(executor_.get(), "record_" + std::to_string(i), "id",
                       std::to_string(i), 1);
  }

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dist(0, num_records - 1);

  for (auto _ : state) {
    for (int i = 0; i < num_records; i++) {
      int random_id = dist(gen);
      std::string result =
          GetValue(executor_.get(), "record_" + std::to_string(random_id));
      benchmark::DoNotOptimize(result);
    }
  }

  state.SetItemsProcessed(state.iterations() * num_records);
  state.counters["total_records"] = num_records;
  state.counters["composite_keys"] = num_records;

  TearDown(state);
}

BENCHMARK_REGISTER_F(CompositeKeyBenchmark, StringFieldInMemory)
    ->Args({1000})
    ->Args({10000})
    ->Args({100000})
    ->Unit(benchmark::kMillisecond);

BENCHMARK_REGISTER_F(CompositeKeyBenchmark, StringFieldComposite)
    ->Args({1000})
    ->Args({10000})
    ->Args({100000})
    ->Args({1000000})
    ->Unit(benchmark::kMillisecond);

BENCHMARK_REGISTER_F(CompositeKeyBenchmark, IntegerFieldInMemory)
    ->Args({1000})
    ->Args({10000})
    ->Args({100000})
    ->Unit(benchmark::kMillisecond);

BENCHMARK_REGISTER_F(CompositeKeyBenchmark, IntegerFieldComposite)
    ->Args({1000})
    ->Args({10000})
    ->Args({100000})
    ->Args({1000000})
    // ->Args({10000000})
    ->Unit(benchmark::kMillisecond);

BENCHMARK_REGISTER_F(CompositeKeyBenchmark, PrimaryKeyOnly_Rand)
    ->Args({1000})
    ->Args({10000})
    ->Args({100000})
    // ->Args({1000000})
    ->Unit(benchmark::kMillisecond);

BENCHMARK_REGISTER_F(CompositeKeyBenchmark, PrimaryKeyWithCompositeKeys_25AF_Rand)
    ->Args({1000})
    ->Args({10000})
    ->Args({100000})
    ->Args({1000000})
    ->Unit(benchmark::kMillisecond);

BENCHMARK_REGISTER_F(CompositeKeyBenchmark, PrimaryKeyWithCompositeKeys_50AF_Rand)
    ->Args({1000})
    ->Args({10000})
    ->Args({100000})
    ->Args({1000000})
    ->Unit(benchmark::kMillisecond);

BENCHMARK_REGISTER_F(CompositeKeyBenchmark, PrimaryKeyWithCompositeKeys_100AF_Rand)
    ->Args({1000})
    ->Args({10000})
    ->Args({100000})
    ->Args({1000000})
    ->Unit(benchmark::kMillisecond);

// TODO: Working on range query bench
//  BENCHMARK_REGISTER_F(CompositeKeyBenchmark, RangeQueryInMemory)
//      ->Args({1000})
//      ->Args({10000})
//      ->Args({100000})
//      ->Unit(benchmark::kMillisecond);

// BENCHMARK_REGISTER_F(CompositeKeyBenchmark, RangeQueryComposite)
//     ->Args({1000})
//     ->Args({10000})
//     ->Args({100000})
//     ->Unit(benchmark::kMillisecond);

}  // namespace
}  // namespace resdb

int main(int argc, char** argv) {
  ::benchmark::Initialize(&argc, argv);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
  ::benchmark::RunSpecifiedBenchmarks();
  ::benchmark::Shutdown();
  return 0;
}

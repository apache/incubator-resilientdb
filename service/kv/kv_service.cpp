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

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <algorithm>
#include <cctype>

#include "chain/storage/memory_db.h"
#include "executor/kv/kv_executor.h"
#include "platform/config/resdb_config_utils.h"
#include "platform/statistic/stats.h"
#include "service/utils/server_factory.h"
#ifdef ENABLE_DUCKDB
#include "chain/storage/duckdb.h"
#endif
#ifdef ENABLE_LEVELDB
#include "chain/storage/leveldb.h"
#endif

using namespace resdb;
using namespace resdb::storage;

DEFINE_string(storage_backend, "memory",
              "Storage backend to use: memory|leveldb|duckdb");

void ShowUsage() {
  printf("<config> <private_key> <cert_file> [logging_dir]\n");
}

std::unique_ptr<Storage> NewStorage(const std::string& db_path,
                                    const ResConfigData& config_data) {
  std::string backend = FLAGS_storage_backend;
  std::transform(backend.begin(), backend.end(), backend.begin(),
                 [](unsigned char c) { return std::tolower(c); });
#ifdef ENABLE_DUCKDB
  if (backend == "duckdb") {
    DuckDBInfo duckdb_cfg;
    if (config_data.has_duckdb_info()) {
      duckdb_cfg = config_data.duckdb_info();
    }
    if (!duckdb_cfg.has_path()) {
      duckdb_cfg.set_path(db_path);
    }
    LOG(INFO) << "use duckdb storage at " << duckdb_cfg.path();
    return NewResQL(db_path, duckdb_cfg);
  }
#else
  if (backend == "duckdb") {
    LOG(FATAL) << "DuckDB backend requested but binary is not built with "
                  "DuckDB. Rebuild with --define enable_duckdb=True.";
  }
#endif
#ifdef ENABLE_LEVELDB
  if (backend == "leveldb") {
    LOG(INFO) << "use leveldb storage.";
    return NewResLevelDB(db_path, config_data.leveldb_info());
  }
#else
  if (backend == "leveldb") {
    LOG(FATAL) << "LevelDB backend requested but binary is not built with "
                  "LevelDB. Rebuild with --define enable_leveldb=True.";
  }
#endif
  LOG(INFO) << "use memory storage.";
  return NewMemoryDB();
}

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  if (argc < 4) {
    ShowUsage();
    exit(0);
  }
  google::InitGoogleLogging(argv[0]);
  FLAGS_minloglevel = 1;

  char* config_file = argv[1];
  char* private_key_file = argv[2];
  char* cert_file = argv[3];

  if (argc == 5) {
    std::string grafana_port = argv[4];
    std::string grafana_address = "0.0.0.0:" + grafana_port;

    auto monitor_port = Stats::GetGlobalStats(5);
    monitor_port->SetPrometheus(grafana_address);
    LOG(ERROR) << "monitoring port:" << grafana_address;
  }

  std::unique_ptr<ResDBConfig> config =
      GenerateResDBConfig(config_file, private_key_file, cert_file);
  ResConfigData config_data = config->GetConfigData();

  std::string db_path = std::to_string(config->GetSelfInfo().port()) + "_db/";
  LOG(INFO) << "db path:" << db_path;

  auto server = GenerateResDBServer(
      config_file, private_key_file, cert_file,
      std::make_unique<KVExecutor>(NewStorage(db_path, config_data)), nullptr);
  server->Run();
}

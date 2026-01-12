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

#include <glog/logging.h>

#include <algorithm>
#include <cctype>
#include <string>

#include "chain/storage/memory_db.h"
#include "chain/storage/duckdb.h"
#include "chain/storage/proto/duckdb_config.pb.h"
#include "executor/kv/kv_executor.h"
#include "platform/config/resdb_config_utils.h"
#include "platform/statistic/stats.h"
#include "service/utils/server_factory.h"
#ifdef ENABLE_LEVELDB
#include "chain/storage/leveldb.h"
#endif

using namespace resdb;
using namespace resdb::storage;

void SignalHandler(int sig_num) {
  LOG(ERROR) << " signal:" << sig_num << " call"
             << " ======================";
}

void ShowUsage() {
  printf("<config> <private_key> <cert_file> "
         "[--enable_duckdb] [--duckdb_path=<path>] "
         "[--grafana_port=<port> | <grafana_port>]\n");
}

std::unique_ptr<Storage> NewStorage(const std::string& db_path,
                                    const ResConfigData& config_data,
                                    bool enable_duckdb,
                                    const std::string& duckdb_path) {
  if (enable_duckdb) {
    std::string path = duckdb_path;
    if (path.empty()) {
      path = db_path + "duckdb.db";
    }
    LOG(INFO) << "use duckdb storage, path=" << path;
    resdb::storage::DuckDBInfo duckdb_info;
    duckdb_info.set_path(path);
    return NewResQL(path, duckdb_info);
  }
#ifdef ENABLE_LEVELDB
  LOG(INFO) << "use leveldb storage.";
  return NewResLevelDB(db_path, config_data.leveldb_info());
#endif
  LOG(INFO) << "use memory storage.";
  return NewMemoryDB();
}

int main(int argc, char** argv) {
  if (argc < 4) {
    ShowUsage();
    exit(0);
  }
  google::InitGoogleLogging(argv[0]);
  FLAGS_minloglevel = 0;
  signal(SIGINT, SignalHandler);
  signal(SIGKILL, SignalHandler);

  char* config_file = argv[1];
  char* private_key_file = argv[2];
  char* cert_file = argv[3];

  bool enable_duckdb = false;
  std::string duckdb_path;
  std::string grafana_port;

  for (int i = 4; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--enable_duckdb") {
      enable_duckdb = true;
    } else if (arg.rfind("--duckdb_path=", 0) == 0) {
      duckdb_path = arg.substr(std::string("--duckdb_path=").size());
    } else if (arg.rfind("--grafana_port=", 0) == 0) {
      grafana_port = arg.substr(std::string("--grafana_port=").size());
    } else if (grafana_port.empty() &&
               std::all_of(arg.begin(), arg.end(),
                           [](unsigned char ch) { return std::isdigit(ch); })) {
      // Backward-compatible: allow bare grafana port as 4th arg.
      grafana_port = arg;
    } else {
      ShowUsage();
      exit(1);
    }
  }

  if (!grafana_port.empty()) {
    std::string grafana_address = "0.0.0.0:" + grafana_port;

    auto monitor_port = Stats::GetGlobalStats(5);
    monitor_port->SetPrometheus(grafana_address);
    LOG(ERROR) << "monitoring port:" << grafana_address;
  }

  std::unique_ptr<ResDBConfig> config =
      GenerateResDBConfig(config_file, private_key_file, cert_file);
  ResConfigData config_data = config->GetConfigData();

  std::string db_path = std::to_string(config->GetSelfInfo().port()) + "_db/";
  LOG(ERROR) << "db path:" << db_path;

  auto server = GenerateResDBServer(
      config_file, private_key_file, cert_file,
      std::make_unique<KVExecutor>(
          NewStorage(db_path, config_data, enable_duckdb, duckdb_path)),
      nullptr);
  server->Run();
}

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

#include <sys/stat.h>

#include "chain/storage/memory_db.h"
#include "executor/kv/kv_executor.h"
#include "platform/config/resdb_config_utils.h"
#include "platform/statistic/stats.h"
#include "service/utils/server_factory.h"
#ifdef ENABLE_LEVELDB
#include "chain/storage/leveldb.h"
#include "chain/storage/tiered_storage.h"
#include "chain/storage/ipfs_client.h"
#endif

using namespace resdb;
using namespace resdb::storage;

void SignalHandler(int sig_num) {
  LOG(ERROR) << " signal:" << sig_num << " call"
             << " ======================";
}

void ShowUsage() {
  printf("<config> <private_key> <cert_file> [logging_dir]\n");
}

std::unique_ptr<Storage> NewStorage(const std::string& db_path,
                                    const ResConfigData& config_data) {
  LOG(ERROR) << "NewStorage: has_storage_config=" << config_data.has_storage_config();
#ifdef ENABLE_LEVELDB
  if (!config_data.has_storage_config()) {
    LOG(ERROR) << "NewStorage: no storage config, using MemoryDB";
    return NewMemoryDB();
  }

  const auto& storage_config = config_data.storage_config();
  auto backend = storage_config.backend();

  if (backend == StorageConfig::MEMORYDB) {
    LOG(ERROR) << "NewStorage: using MemoryDB only";
    return NewMemoryDB();
  }

  if (backend == StorageConfig::LEVELDB_ONLY) {
    LOG(ERROR) << "NewStorage: using LevelDB only";
    return NewResLevelDB(db_path, config_data.leveldb_info());
  }

  if (backend == StorageConfig::TIERED) {
    const auto& tiered_info = storage_config.tiered_info();

    mkdir(db_path.c_str(), 0755);

    std::unique_ptr<Storage> hot;
    if (tiered_info.hot_backend() == TieredStorageConfig::LEVELDB) {
      LOG(ERROR) << "NewStorage: using Tiered with LevelDB hot tier";
      hot = NewResLevelDB(db_path, config_data.leveldb_info());
    } else {
      LOG(ERROR) << "NewStorage: using Tiered with MemoryDB hot tier";
      hot = NewMemoryDB();
    }

    std::string warm_path = db_path + "_manifest_db";
    auto warm = NewResLevelDB(warm_path, config_data.leveldb_info());
    auto ipfs = IPFSClient::Create(storage_config.ipfs_info());

    TieredStorageConfig tiered_config = tiered_info;
    tiered_config.set_enabled(true);
    tiered_config.set_auto_migration_enabled(true);

    auto tiered = TieredStorage::Create(
        std::move(hot), std::move(warm),
        storage_config.ipfs_info(), tiered_config);

    LOG(ERROR) << "TieredStorage created: hot_backend="
              << tiered_info.hot_backend()
              << " cold_threshold=" << tiered_config.cold_threshold_checkpoint()
              << " ipfs_endpoint=" << storage_config.ipfs_info().api_endpoint();
    return tiered;
  }

  LOG(ERROR) << "NewStorage: unknown backend, using MemoryDB";
  return NewMemoryDB();
#else
  LOG(ERROR) << "NewStorage: LevelDB disabled, using MemoryDB";
  return NewMemoryDB();
#endif
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
  LOG(ERROR) << "db path:" << db_path;

  auto server = GenerateResDBServer(
      config_file, private_key_file, cert_file,
      std::make_unique<KVExecutor>(NewStorage(db_path, config_data)), nullptr);
  server->Run();
}

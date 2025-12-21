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

#include "platform/config/resdb_config_utils.h"
#include "platform/statistic/stats.h"
#include "service/kv_service/kv_service_transaction_manager.h"
#include "service/utils/server_factory.h"

using sdk::KVServiceTransactionManager;
using namespace resdb;

void ShowUsage() {
  printf("<config> <private_key> <cert_file> [logging_dir]\n");
}

std::unique_ptr<ChainState> NewState(const std::string &cert_file,
                                     const ResConfigData &config_data) {
  std::unique_ptr<Storage> storage = nullptr;

#ifdef ENABLE_ROCKSDB
  storage = NewResRocksDB(cert_file.c_str(), config_data);
  LOG(INFO) << "use rocksdb storage.";
#endif

#ifdef ENABLE_LEVELDB
  storage = NewResLevelDB(cert_file.c_str(), config_data);
  LOG(INFO) << "use leveldb storage.";
#endif
  std::unique_ptr<ChainState> state =
      std::make_unique<ChainState>(std::move(storage));
  return state;
}

int main(int argc, char **argv) {
  if (argc < 4) {
    ShowUsage();
    exit(0);
  }

  char *config_file = argv[1];
  char *private_key_file = argv[2];
  char *cert_file = argv[3];
  char *logging_dir = nullptr;

  if (argc >= 6) {
    logging_dir = argv[5];
  }

  if (argc >= 5) {
    auto monitor_port = Stats::GetGlobalStats(5);
    monitor_port->SetPrometheus(argv[4]);
    LOG(ERROR) << "prot:" << argv[4];
  }

  std::unique_ptr<ResDBConfig> config =
      GenerateResDBConfig(config_file, private_key_file, cert_file);
  ResConfigData config_data = config->GetConfigData();

  auto server =
      GenerateResDBServer(config_file, private_key_file, cert_file,
                          std::make_unique<KVServiceTransactionManager>(
                              NewState(cert_file, config_data)),
                          logging_dir);
  server->Run();
}

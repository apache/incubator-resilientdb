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
#include <google/protobuf/util/json_util.h>

#include <fstream>

#include "executor/utxo/executor/utxo_executor.h"
#include "platform/config/resdb_config_utils.h"
#include "platform/statistic/stats.h"
#include "service/utils/server_factory.h"

using google::protobuf::util::JsonParseOptions;
using resdb::ConsensusManagerPBFT;
using resdb::CustomGenerateResDBServer;
using resdb::GenerateResDBConfig;
using resdb::ResConfigData;
using resdb::ResDBConfig;
using resdb::Stats;
using resdb::utxo::QueryExecutor;
using resdb::utxo::Transaction;
using resdb::utxo::UTXOExecutor;
using resdb::utxo::Wallet;

resdb::utxo::Config ReadConfigFromFile(const std::string& file_name) {
  std::stringstream json_data;
  std::ifstream infile(file_name.c_str());
  json_data << infile.rdbuf();

  resdb::utxo::Config config_data;
  JsonParseOptions options;
  auto status = JsonStringToMessage(json_data.str(), &config_data, options);
  if (!status.ok()) {
    LOG(ERROR) << "parse json :" << file_name << " fail:" << status.message();
  }
  assert(status.ok());
  return config_data;
}

void ShowUsage() {
  printf(
      "<config> <private_key> <cert_file> <durability_option> [logging_dir]\n");
}

int main(int argc, char** argv) {
  if (argc < 4) {
    ShowUsage();
    exit(0);
  }

  char* config_file = argv[1];
  char* private_key_file = argv[2];
  char* cert_file = argv[3];

  char* utxo_config_file = argv[4];

  if (argc >= 6) {
    auto monitor_port = Stats::GetGlobalStats(5);
    monitor_port->SetPrometheus(argv[5]);
  }

  resdb::utxo::Config utxo_config = ReadConfigFromFile(utxo_config_file);

  std::unique_ptr<ResDBConfig> config =
      GenerateResDBConfig(config_file, private_key_file, cert_file);
  ResConfigData config_data = config->GetConfigData();

  std::unique_ptr<Wallet> wallet = std::make_unique<Wallet>();
  std::unique_ptr<Transaction> transaction =
      std::make_unique<Transaction>(utxo_config, wallet.get());

  auto server = CustomGenerateResDBServer<ConsensusManagerPBFT>(
      config_file, private_key_file, cert_file,
      std::make_unique<UTXOExecutor>(utxo_config, transaction.get(),
                                     wallet.get()),
      std::make_unique<QueryExecutor>(transaction.get(), wallet.get()));

  server->Run();
}

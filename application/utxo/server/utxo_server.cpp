/*
 * Copyright (c) 2019-2022 ExpoLab, UC Davis
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include <glog/logging.h>
#include <google/protobuf/util/json_util.h>

#include <fstream>

#include "application/utils/server_factory.h"
#include "application/utxo/service/utxo_executor.h"
#include "config/resdb_config_utils.h"
#include "ordering/pbft/consensus_service_pbft.h"
#include "statistic/stats.h"

using google::protobuf::util::JsonParseOptions;
using resdb::ConsensusServicePBFT;
using resdb::CustomGenerateResDBServer;
using resdb::GenerateResDBConfig;
using resdb::ResConfigData;
using resdb::ResDBConfig;
using resdb::ResDBServer;
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

  auto server = CustomGenerateResDBServer<ConsensusServicePBFT>(
      config_file, private_key_file, cert_file,
      std::make_unique<UTXOExecutor>(utxo_config, transaction.get(),
                                     wallet.get()),
      std::make_unique<QueryExecutor>(transaction.get(), wallet.get()));

  server->Run();
}

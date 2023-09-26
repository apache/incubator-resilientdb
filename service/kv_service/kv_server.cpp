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

#include "platform/config/resdb_config_utils.h"
#include "platform/statistic/stats.h"
#include "service/kv_service/kv_service_transaction_manager.h"
#include "service/utils/server_factory.h"

using sdk::KVServiceTransactionManager;
using namespace resdb;

void ShowUsage() {
  printf("<config> <private_key> <cert_file> [logging_dir]\n");
}

std::unique_ptr<ChainState> NewState(const std::string& cert_file,
                                    const ResConfigData& config_data) {
  std::unique_ptr<Storage> storage = nullptr;

#ifdef ENABLE_ROCKSDB
  storage = NewResRocksDB(cert_file.c_str(), config_data);
  LOG(INFO) << "use rocksdb storage.";
#endif

#ifdef ENABLE_LEVELDB
  storage = NewResLevelDB(cert_file.c_str(), config_data);
  LOG(INFO) << "use leveldb storage.";
#endif
  std::unique_ptr<ChainState> state  = std::make_unique<ChainState>(std::move(storage));
  return state;
}

int main(int argc, char** argv) {
  if (argc < 4) {
    ShowUsage();
    exit(0);
  }

  char* config_file = argv[1];
  char* private_key_file = argv[2];
  char* cert_file = argv[3];
  char* logging_dir = nullptr;

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

  auto server = GenerateResDBServer(
      config_file, private_key_file, cert_file,
      std::make_unique<KVServiceTransactionManager>(NewState(cert_file, config_data)),
      logging_dir);
  server->Run();
}

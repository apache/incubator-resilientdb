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

#include "application/geo_pbft/geo_pbft_executor_impl.h"
#include "config/resdb_config_utils.h"
#include "execution/geo_global_executor.h"
#include "execution/system_info.h"
#include "ordering/geo_pbft/consensus_service_geo_pbft.h"
#include "server/resdb_server.h"

using resdb::ConsensusServiceGeoPBFT;
using resdb::GenerateResDBConfig;
using resdb::GeoGlobalExecutor;
using resdb::GeoPBFTExecutorImpl;
using resdb::GeoTransactionExecutor;
using resdb::ReplicaInfo;
using resdb::ResDBConfig;
using resdb::ResDBReplicaClient;
using resdb::ResDBServer;
using resdb::SystemInfo;

void ShowUsage() { printf("<config> <private_key> <cert_file>\n"); }

int main(int argc, char** argv) {
  if (argc < 3) {
    ShowUsage();
    exit(0);
  }

  char* config_file = argv[1];
  char* private_key_file = argv[2];
  char* cert_file = argv[3];

  std::unique_ptr<ResDBConfig> geo_config =
      GenerateResDBConfig(config_file, private_key_file, cert_file);

  ResDBServer server(
      *geo_config,
      std::make_unique<ConsensusServiceGeoPBFT>(
          *geo_config,
          std::make_unique<GeoTransactionExecutor>(
              *geo_config, std::make_unique<SystemInfo>(*geo_config),
              std::make_unique<ResDBReplicaClient>(std::vector<ReplicaInfo>(),
                                                   nullptr, true),
              std::make_unique<GeoPBFTExecutorImpl>()),
          std::make_unique<GeoGlobalExecutor>(
              std::make_unique<GeoPBFTExecutorImpl>(), *geo_config)));
  server.Run();
}

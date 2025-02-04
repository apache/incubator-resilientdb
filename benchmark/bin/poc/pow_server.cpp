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

#include "platform/communication/resdb_server.h"
#include "platform/config/resdb_config_utils.h"
#include "platform/consensus/ordering/poc/pow/consensus_service_pow.h"

using resdb::CertificateInfo;
using resdb::ConsensusServicePoW;
using resdb::GenerateReplicaInfo;
using resdb::GenerateResDBConfig;
using resdb::KeyInfo;
using resdb::ReadConfig;
using resdb::ReplicaInfo;
using resdb::ResConfigData;
using resdb::ResDBConfig;
using resdb::ResDBPoCConfig;
using resdb::ResDBServer;

void ShowUsage() { printf("<bft config> <pow config>\n"); }

int main(int argc, char** argv) {
  if (argc < 5) {
    ShowUsage();
    exit(0);
  }

  std::string bft_config_file = argv[1];
  std::string pow_config_file = argv[2];
  std::string private_key_file = argv[3];
  std::string cert_file = argv[4];
  LOG(ERROR) << "pow_config:" << pow_config_file;

  ResDBConfig bft_config = GenerateResDBConfig(bft_config_file);

  std::unique_ptr<ResDBConfig> pow_config = GenerateResDBConfig(
      pow_config_file, private_key_file, cert_file, std::nullopt,
      [&](const ResConfigData& config_data, const ReplicaInfo& self_info,
          const KeyInfo& private_key,
          const CertificateInfo& public_key_cert_info) {
        return std::make_unique<ResDBPoCConfig>(bft_config, config_data,
                                                self_info, private_key,
                                                public_key_cert_info);
      });

  ResDBPoCConfig* pow_config_ptr =
      static_cast<ResDBPoCConfig*>(pow_config.get());

  pow_config_ptr->SetMaxNonceBit(42);
  pow_config_ptr->SetDifficulty(32);

  ResDBServer server(*pow_config_ptr,
                     std::make_unique<ConsensusServicePoW>(*pow_config_ptr));
  server.Run();
}

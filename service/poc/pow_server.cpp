#include "platform/config/resdb_config_utils.h"
#include "platform/consensus/ordering/poc/pow/consensus_service_pow.h"
#include "service/utils/server_factory.h"
#include "platform/statistic/stats.h"

using resdb::CertificateInfo;
using resdb::ConsensusServicePoW;
using resdb::GenerateReplicaInfo;
using resdb::GenerateResDBConfig;
using resdb::KeyInfo;
using resdb::ReadConfig;
using resdb::Stats;
using resdb::ReplicaInfo;
using resdb::ResDBConfig;
using resdb::ResConfigData;
using resdb::ResDBPoCConfig;
using resdb::ServiceNetwork;

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
      [&](const ResConfigData& config_data,
          const ReplicaInfo& self_info, const KeyInfo& private_key,
          const CertificateInfo& public_key_cert_info) {
        return std::make_unique<ResDBPoCConfig>(
            bft_config, config_data, self_info, private_key, public_key_cert_info);
      });


  LOG(ERROR)<<"elf ip:"<<pow_config->GetSelfInfo().ip();
  ResDBPoCConfig* pow_config_ptr =
      static_cast<ResDBPoCConfig*>(pow_config.get());

  pow_config_ptr->SetMaxNonceBit(42);
  pow_config_ptr->SetDifficulty(36);
  
  auto server =
      std::make_unique<ServiceNetwork>(*pow_config_ptr, std::make_unique<ConsensusServicePoW>(*pow_config_ptr));
  server->Run();
}


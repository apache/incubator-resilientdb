#include "application/utils/server_factory.h"

#include "config/resdb_config_utils.h"
#include "ordering/pbft/consensus_service_pbft.h"

namespace resdb {

std::unique_ptr<ResDBServer> ServerFactory::CreateResDBServer(
    char* config_file, char* private_key_file, char* cert_file,
    std::unique_ptr<TransactionExecutorImpl> executor, char* logging_dir,
    std::function<void(ResDBConfig* config)> config_handler) {
  std::unique_ptr<ResDBConfig> config =
      GenerateResDBConfig(config_file, private_key_file, cert_file);

  if (config_handler) {
    config_handler(config.get());
  }
  return std::make_unique<ResDBServer>(
      *config,
      std::make_unique<ConsensusServicePBFT>(*config, std::move(executor)));
}

std::unique_ptr<ResDBServer> GenerateResDBServer(
    char* config_file, char* private_key_file, char* cert_file,
    std::unique_ptr<TransactionExecutorImpl> executor, char* logging_dir,
    std::function<void(ResDBConfig* config)> config_handler) {
  return ServerFactory().CreateResDBServer(config_file, private_key_file,
                                           cert_file, std::move(executor),
                                           logging_dir, config_handler);
}

}  // namespace resdb

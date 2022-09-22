#pragma once

#include "execution/transaction_executor_impl.h"
#include "server/resdb_server.h"

namespace resdb {

class ServerFactory {
 public:
  std::unique_ptr<ResDBServer> CreateResDBServer(
      char* config_file, char* private_key_file, char* cert_file,
      std::unique_ptr<TransactionExecutorImpl> executor, char* logging_dir,
      std::function<void(ResDBConfig* config)> config_handler);
};

std::unique_ptr<ResDBServer> GenerateResDBServer(
    char* config_file, char* private_key_file, char* cert_file,
    std::unique_ptr<TransactionExecutorImpl> executor,
    char* logging_dir = nullptr,
    std::function<void(ResDBConfig* config)> config_handler = nullptr);
}  // namespace resdb

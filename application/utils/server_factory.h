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

#pragma once

#include "config/resdb_config_utils.h"
#include "execution/custom_query.h"
#include "execution/transaction_executor_impl.h"
#include "ordering/pbft/consensus_service_pbft.h"
#include "server/resdb_server.h"

namespace resdb {

class ServerFactory {
 public:
  std::unique_ptr<ResDBServer> CreateResDBServer(
      char* config_file, char* private_key_file, char* cert_file,
      std::unique_ptr<TransactionExecutorImpl> executor, char* logging_dir,
      std::function<void(ResDBConfig* config)> config_handler);

  template <typename ConsensusProtocol = ConsensusServicePBFT>
  std::unique_ptr<ResDBServer> CustomCreateResDBServer(
      char* config_file, char* private_key_file, char* cert_file,
      std::unique_ptr<TransactionExecutorImpl> executor, char* logging_dir,
      std::function<void(ResDBConfig* config)> config_handler);

  template <typename ConsensusProtocol = ConsensusServicePBFT>
  std::unique_ptr<ResDBServer> CustomCreateResDBServer(
      char* config_file, char* private_key_file, char* cert_file,
      std::unique_ptr<TransactionExecutorImpl> executor,
      std::unique_ptr<CustomQuery> query_executor,
      std::function<void(ResDBConfig* config)> config_handler);
};

std::unique_ptr<ResDBServer> GenerateResDBServer(
    char* config_file, char* private_key_file, char* cert_file,
    std::unique_ptr<TransactionExecutorImpl> executor,
    char* logging_dir = nullptr,
    std::function<void(ResDBConfig* config)> config_handler = nullptr);

template <typename ConsensusProtocol>
std::unique_ptr<ResDBServer> CustomGenerateResDBServer(
    char* config_file, char* private_key_file, char* cert_file,
    std::unique_ptr<TransactionExecutorImpl> executor,
    char* logging_dir = nullptr,
    std::function<void(ResDBConfig* config)> config_handler = nullptr);

template <typename ConsensusProtocol>
std::unique_ptr<ResDBServer> CustomGenerateResDBServer(
    char* config_file, char* private_key_file, char* cert_file,
    std::unique_ptr<TransactionExecutorImpl> executor,
    std::unique_ptr<CustomQuery> query_executor,
    std::function<void(ResDBConfig* config)> config_handler = nullptr);

// ===================================================================
template <typename ConsensusProtocol>
std::unique_ptr<ResDBServer> ServerFactory::CustomCreateResDBServer(
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
      std::make_unique<ConsensusProtocol>(*config, std::move(executor)));
}

template <typename ConsensusProtocol>
std::unique_ptr<ResDBServer> ServerFactory::CustomCreateResDBServer(
    char* config_file, char* private_key_file, char* cert_file,
    std::unique_ptr<TransactionExecutorImpl> executor,
    std::unique_ptr<CustomQuery> query_executor,
    std::function<void(ResDBConfig* config)> config_handler) {
  std::unique_ptr<ResDBConfig> config =
      GenerateResDBConfig(config_file, private_key_file, cert_file);

  if (config_handler) {
    config_handler(config.get());
  }
  return std::make_unique<ResDBServer>(
      *config, std::make_unique<ConsensusProtocol>(*config, std::move(executor),
                                                   std::move(query_executor)));
}

template <typename ConsensusProtocol>
std::unique_ptr<ResDBServer> CustomGenerateResDBServer(
    char* config_file, char* private_key_file, char* cert_file,
    std::unique_ptr<TransactionExecutorImpl> executor, char* logging_dir,
    std::function<void(ResDBConfig* config)> config_handler) {
  return ServerFactory().CustomCreateResDBServer<ConsensusProtocol>(
      config_file, private_key_file, cert_file, std::move(executor),
      logging_dir, config_handler);
}

template <typename ConsensusProtocol>
std::unique_ptr<ResDBServer> CustomGenerateResDBServer(
    char* config_file, char* private_key_file, char* cert_file,
    std::unique_ptr<TransactionExecutorImpl> executor,
    std::unique_ptr<CustomQuery> query_executor,
    std::function<void(ResDBConfig* config)> config_handler) {
  return ServerFactory().CustomCreateResDBServer<ConsensusProtocol>(
      config_file, private_key_file, cert_file, std::move(executor),
      std::move(query_executor), config_handler);
}

}  // namespace resdb

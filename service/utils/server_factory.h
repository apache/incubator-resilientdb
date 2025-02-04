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

#include "executor/common/custom_query.h"
#include "executor/common/transaction_manager.h"
#include "platform/config/resdb_config_utils.h"
#include "platform/consensus/ordering/pbft/consensus_manager_pbft.h"
#include "platform/networkstrate/service_network.h"

namespace resdb {

class ServerFactory {
 public:
  std::unique_ptr<ServiceNetwork> CreateResDBServer(
      char* config_file, char* private_key_file, char* cert_file,
      std::unique_ptr<TransactionManager> executor, char* logging_dir,
      std::function<void(ResDBConfig* config)> config_handler);

  template <typename ConsensusProtocol = ConsensusManagerPBFT>
  std::unique_ptr<ServiceNetwork> CustomCreateResDBServer(
      char* config_file, char* private_key_file, char* cert_file,
      std::unique_ptr<TransactionManager> executor, char* logging_dir,
      std::function<void(ResDBConfig* config)> config_handler);

  template <typename ConsensusProtocol = ConsensusManagerPBFT>
  std::unique_ptr<ServiceNetwork> CustomCreateResDBServer(
      char* config_file, char* private_key_file, char* cert_file,
      std::unique_ptr<TransactionManager> executor,
      std::unique_ptr<CustomQuery> query_executor,
      std::function<void(ResDBConfig* config)> config_handler);
};

std::unique_ptr<ServiceNetwork> GenerateResDBServer(
    char* config_file, char* private_key_file, char* cert_file,
    std::unique_ptr<TransactionManager> executor, char* logging_dir = nullptr,
    std::function<void(ResDBConfig* config)> config_handler = nullptr);

template <typename ConsensusProtocol>
std::unique_ptr<ServiceNetwork> CustomGenerateResDBServer(
    char* config_file, char* private_key_file, char* cert_file,
    std::unique_ptr<TransactionManager> executor, char* logging_dir = nullptr,
    std::function<void(ResDBConfig* config)> config_handler = nullptr);

template <typename ConsensusProtocol>
std::unique_ptr<ServiceNetwork> CustomGenerateResDBServer(
    char* config_file, char* private_key_file, char* cert_file,
    std::unique_ptr<TransactionManager> executor,
    std::unique_ptr<CustomQuery> query_executor,
    std::function<void(ResDBConfig* config)> config_handler = nullptr);

// ===================================================================
template <typename ConsensusProtocol>
std::unique_ptr<ServiceNetwork> ServerFactory::CustomCreateResDBServer(
    char* config_file, char* private_key_file, char* cert_file,
    std::unique_ptr<TransactionManager> executor, char* logging_dir,
    std::function<void(ResDBConfig* config)> config_handler) {
  std::unique_ptr<ResDBConfig> config =
      GenerateResDBConfig(config_file, private_key_file, cert_file);

  if (config_handler) {
    config_handler(config.get());
  }
  return std::make_unique<ServiceNetwork>(
      *config,
      std::make_unique<ConsensusProtocol>(*config, std::move(executor)));
}

template <typename ConsensusProtocol>
std::unique_ptr<ServiceNetwork> ServerFactory::CustomCreateResDBServer(
    char* config_file, char* private_key_file, char* cert_file,
    std::unique_ptr<TransactionManager> executor,
    std::unique_ptr<CustomQuery> query_executor,
    std::function<void(ResDBConfig* config)> config_handler) {
  std::unique_ptr<ResDBConfig> config =
      GenerateResDBConfig(config_file, private_key_file, cert_file);

  if (config_handler) {
    config_handler(config.get());
  }
  return std::make_unique<ServiceNetwork>(
      *config, std::make_unique<ConsensusProtocol>(*config, std::move(executor),
                                                   std::move(query_executor)));
}

template <typename ConsensusProtocol>
std::unique_ptr<ServiceNetwork> CustomGenerateResDBServer(
    char* config_file, char* private_key_file, char* cert_file,
    std::unique_ptr<TransactionManager> executor, char* logging_dir,
    std::function<void(ResDBConfig* config)> config_handler) {
  return ServerFactory().CustomCreateResDBServer<ConsensusProtocol>(
      config_file, private_key_file, cert_file, std::move(executor),
      logging_dir, config_handler);
}

template <typename ConsensusProtocol>
std::unique_ptr<ServiceNetwork> CustomGenerateResDBServer(
    char* config_file, char* private_key_file, char* cert_file,
    std::unique_ptr<TransactionManager> executor,
    std::unique_ptr<CustomQuery> query_executor,
    std::function<void(ResDBConfig* config)> config_handler) {
  return ServerFactory().CustomCreateResDBServer<ConsensusProtocol>(
      config_file, private_key_file, cert_file, std::move(executor),
      std::move(query_executor), config_handler);
}

}  // namespace resdb

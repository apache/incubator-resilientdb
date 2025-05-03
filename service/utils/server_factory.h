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

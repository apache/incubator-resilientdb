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

#include "service/utils/server_factory.h"

#include <algorithm>
#include <cctype>
#include <string>

#include <glog/logging.h>

namespace resdb {

namespace {

std::string NormalizeConsensusProtocol(const std::string& consensus_protocol) {
  if (consensus_protocol.empty()) {
    return "pbft";
  }
  std::string normalized = consensus_protocol;
  std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return normalized;
}

}  // namespace

std::unique_ptr<ServiceNetwork> ServerFactory::CreateResDBServer(
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
      std::make_unique<ConsensusManagerPBFT>(*config, std::move(executor)));
}

std::unique_ptr<ServiceNetwork> GenerateResDBServer(
    char* config_file, char* private_key_file, char* cert_file,
    std::unique_ptr<TransactionManager> executor, char* logging_dir,
    std::function<void(ResDBConfig* config)> config_handler) {
  return ServerFactory().CreateResDBServer(config_file, private_key_file,
                                           cert_file, std::move(executor),
                                           logging_dir, config_handler);
}

std::unique_ptr<ServiceNetwork> ServerFactory::CreateResDBServerForProtocol(
    const std::string& consensus_protocol, char* config_file,
    char* private_key_file, char* cert_file,
    std::unique_ptr<TransactionManager> executor, char* logging_dir,
    std::function<void(ResDBConfig* config)> config_handler) {
  std::string normalized = NormalizeConsensusProtocol(consensus_protocol);
  if (normalized == "pbft") {
    return CreateResDBServer(config_file, private_key_file, cert_file,
                             std::move(executor), logging_dir, config_handler);
  }
  if (normalized == "raft") {
#if RESDB_HAS_RAFT
    return CustomCreateResDBServer<ConsensusManagerRaft>(
        config_file, private_key_file, cert_file, std::move(executor),
        logging_dir, config_handler);
#else
    LOG(FATAL) << "consensus_protocol=raft requested but RAFT support is not "
                  "compiled in yet. Add ConsensusManagerRaft before enabling "
                  "the raft protocol.";
#endif
  }
  LOG(FATAL) << "Unsupported consensus_protocol: " << consensus_protocol;
  return nullptr;
}

std::unique_ptr<ServiceNetwork> ServerFactory::CreateResDBServerForProtocol(
    const std::string& consensus_protocol, char* config_file,
    char* private_key_file, char* cert_file,
    std::unique_ptr<TransactionManager> executor,
    std::unique_ptr<CustomQuery> query_executor,
    std::function<void(ResDBConfig* config)> config_handler) {
  std::string normalized = NormalizeConsensusProtocol(consensus_protocol);
  if (normalized == "pbft") {
    return CustomCreateResDBServer<ConsensusManagerPBFT>(
        config_file, private_key_file, cert_file, std::move(executor),
        std::move(query_executor), config_handler);
  }
  if (normalized == "raft") {
#if RESDB_HAS_RAFT
    return CustomCreateResDBServer<ConsensusManagerRaft>(
        config_file, private_key_file, cert_file, std::move(executor),
        std::move(query_executor), config_handler);
#else
    LOG(FATAL) << "consensus_protocol=raft requested but RAFT support is not "
                  "compiled in yet. Add ConsensusManagerRaft before enabling "
                  "the raft protocol.";
#endif
  }
  LOG(FATAL) << "Unsupported consensus_protocol: " << consensus_protocol;
  return nullptr;
}

std::unique_ptr<ServiceNetwork> GenerateResDBServerForProtocol(
    const std::string& consensus_protocol, char* config_file,
    char* private_key_file, char* cert_file,
    std::unique_ptr<TransactionManager> executor, char* logging_dir,
    std::function<void(ResDBConfig* config)> config_handler) {
  return ServerFactory().CreateResDBServerForProtocol(
      consensus_protocol, config_file, private_key_file, cert_file,
      std::move(executor), logging_dir, config_handler);
}

}  // namespace resdb

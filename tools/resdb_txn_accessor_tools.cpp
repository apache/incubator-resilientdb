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

#include <glog/logging.h>

#include "interface/common/resdb_txn_accessor.h"
#include "platform/config/resdb_config_utils.h"

using resdb::GenerateReplicaInfo;
using resdb::ReplicaInfo;
using resdb::ResDBConfig;
using resdb::ResDBTxnAccessor;

int main(int argc, char** argv) {
  if (argc < 6) {
    printf(
        "<config path> <private key path> <cert_file> <min_seq> <max_seq>\n");
    return 0;
  }
  std::string config_file = argv[1];
  std::string private_key_file = argv[2];
  std::string cert_file = argv[3];
  uint64_t min_seq = atoi(argv[4]);
  uint64_t max_seq = atoi(argv[5]);

  ReplicaInfo self_info = GenerateReplicaInfo(0, "127.0.0.1", 88888);

  std::unique_ptr<ResDBConfig> config =
      GenerateResDBConfig(config_file, private_key_file, cert_file, self_info);

  ResDBTxnAccessor client(*config);
  auto resp = client.GetTxn(min_seq, max_seq);
  absl::StatusOr<std::vector<std::pair<uint64_t, std::string>>> GetTxn(
      uint64_t min_seq, uint64_t max_seq);
  if (!resp.ok()) {
    LOG(ERROR) << "get replica state fail";
    exit(1);
  }
  for (auto& txn : *resp) {
    LOG(ERROR) << "seq:" << txn.first << " txn:" << txn.second;
  }
}

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

#include "platform/config/resdb_poc_config.h"

namespace resdb {

ResDBPoCConfig::ResDBPoCConfig(const ResDBConfig& bft_config,
                               const ResConfigData& config_data,
                               const ReplicaInfo& self_info,
                               const KeyInfo& private_key,
                               const CertificateInfo& public_key_cert_info)
    : ResDBConfig(config_data, self_info, private_key, public_key_cert_info),
      bft_config_(bft_config) {
  SetHeartBeatEnabled(false);
  SetSignatureVerifierEnabled(false);
}

const ResDBConfig* ResDBPoCConfig::GetBFTConfig() const { return &bft_config_; }

void ResDBPoCConfig::SetMaxNonceBit(uint32_t bit) { max_nonce_bit_ = bit; }

uint32_t ResDBPoCConfig::GetMaxNonceBit() const { return max_nonce_bit_; }

void ResDBPoCConfig::SetDifficulty(uint32_t difficulty) {
  difficulty_ = difficulty;
}

uint32_t ResDBPoCConfig::GetDifficulty() const { return difficulty_; }

uint32_t ResDBPoCConfig::GetTargetValue() const { return target_value_; }

void ResDBPoCConfig::SetTargetValue(uint32_t target_value) {
  target_value_ = target_value;
}

std::vector<ReplicaInfo> ResDBPoCConfig::GetBFTReplicas() {
  return bft_replicas_;
}

void ResDBPoCConfig::SetBFTReplicas(const std::vector<ReplicaInfo>& replicas) {
  bft_replicas_ = replicas;
}

// Batch
uint32_t ResDBPoCConfig::BatchTransactionNum() const { return batch_num_; }

void ResDBPoCConfig::SetBatchTransactionNum(uint32_t batch_num) {
  batch_num_ = batch_num;
}

uint32_t ResDBPoCConfig::GetWokerNum() { return worker_num_; }

void ResDBPoCConfig::SetWorkerNum(uint32_t worker_num) {
  worker_num_ = worker_num;
}

}  // namespace resdb

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

#include "platform/config/resdb_config.h"

namespace resdb {

// TODO read from a proto json file.
class ResDBPoCConfig : public ResDBConfig {
 public:
  ResDBPoCConfig(const ResDBConfig& bft_config,
                 const ResConfigData& config_data, const ReplicaInfo& self_info,
                 const KeyInfo& private_key,
                 const CertificateInfo& public_key_cert_info);

  const ResDBConfig* GetBFTConfig() const;

  void SetMaxNonceBit(uint32_t bit);
  uint32_t GetMaxNonceBit() const;

  void SetDifficulty(uint32_t difficulty);
  uint32_t GetDifficulty() const;

  // If target value is zero, target value will be 1<<difficulty.
  void SetTargetValue(uint32_t target_value);
  uint32_t GetTargetValue() const;

  // BFT cluster.
  std::vector<ReplicaInfo> GetBFTReplicas();
  void SetBFTReplicas(const std::vector<ReplicaInfo>& replicas);

  // Batch
  uint32_t BatchTransactionNum() const;
  void SetBatchTransactionNum(uint32_t batch_num);

  uint32_t GetWokerNum();
  void SetWorkerNum(uint32_t worker_num);

  uint32_t GetMiningTime() const { return mining_time_ms_; }
  void SetMiningTime(uint32_t time_ms) { mining_time_ms_ = time_ms; }

 private:
  uint32_t difficulty_ = 0;
  uint32_t max_nonce_bit_ = 0;
  uint32_t target_value_ = 0;
  uint32_t batch_num_ = 12000;
  uint32_t worker_num_ = 16;
  uint32_t mining_time_ms_ = 60000;  // 60s
  std::vector<ReplicaInfo> bft_replicas_;
  ResDBConfig bft_config_;
};

}  // namespace resdb

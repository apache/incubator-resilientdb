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

#include "config/resdb_config.h"

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

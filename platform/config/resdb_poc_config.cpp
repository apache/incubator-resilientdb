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

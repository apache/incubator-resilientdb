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

 private:
  uint32_t difficulty_ = 0;
  uint32_t max_nonce_bit_ = 0;
  uint32_t target_value_ = 0;
  uint32_t batch_num_ = 100;
  uint32_t worker_num_ = 16;
  std::vector<ReplicaInfo> bft_replicas_;
  ResDBConfig bft_config_;
};

}  // namespace resdb

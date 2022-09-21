#pragma once
#include "absl/status/status.h"
#include "config/resdb_poc_config.h"
#include "ordering/poc/proto/pow.pb.h"

namespace resdb {

// A miner used to mine the block according to its header.
// It contains the slices as the search space.
// It looks for a solution from the nonces inside the slices
// that the hash value of the header within the searching nonce
// (SHA256(SHA256(header))) is less than a target value which is
// defined from the config and can be reset later.
class Miner {
 public:
  Miner(const ResDBPoCConfig& config);

  std::vector<std::pair<uint64_t, uint64_t>> GetMiningSlices();

  absl::Status Mine(Block* new_block);
  void Terminate();
  bool IsValidHash(const Block* block);

  // Obtain the shift idx.
  int32_t GetSliceIdx() const;
  void SetSliceIdx(int slice_idx);

  void SetTargetValue(const HashValue& target_value);

 private:
  std::string CalculatePoWHashDigest(const Block::Header& header);
  HashValue CalculatePoWHash(const Block* new_block);

 private:
  ResDBPoCConfig config_;
  HashValue target_value_;
  int shift_idx_ = 0;
  std::atomic<bool> stop_;
  std::vector<std::pair<uint64_t, uint64_t>> mining_slices_;
  uint32_t difficulty_ = 1;
  uint32_t worker_num_ = 16;
};

}  // namespace resdb

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
#include "absl/status/status.h"
#include "platform/config/resdb_poc_config.h"
#include "platform/consensus/ordering/poc/proto/pow.pb.h"

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

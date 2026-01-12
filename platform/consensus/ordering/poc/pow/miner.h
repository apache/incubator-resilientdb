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

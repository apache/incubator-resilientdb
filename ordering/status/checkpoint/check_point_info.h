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

#include <map>
#include <set>

#include "common/logging/logging.h"
#include "config/resdb_config.h"
#include "proto/checkpoint_info.pb.h"
#include "proto/resdb.pb.h"

namespace resdb {

class CheckPointInfo {
 public:
  CheckPointInfo(const ResDBConfig& config);

  void AddCommitData(const Request& request);

  CheckPointData GetCheckPointData();

  uint64_t GetMaxCheckPointRequestSeq();
  uint64_t GetStableCheckPointSeq();
  void UpdateStableCheckPoint(const std::vector<CheckPointData>& data);

 private:
  void CalculateHash(const Request& request);

 private:
  std::unique_ptr<Logging> logging_;
  ResDBConfig config_;
  std::mutex mutex_;
  uint64_t last_seq_ = 0;  // the max sequence of the latest checkpoint.
  std::string last_hash_;  // the hash value of the latest checkpoint.
  std::string
      last_block_hash_;  // the hash value of lash checkpoint block window.
  uint64_t current_block_seq_ = 0;
  // stable checkpoint.
  uint64_t last_stable_checkpoint_seq_ = 0;  // the seq of stable checkpoint.
  // a set contains the 2f+1 same checkpoint data.
  std::vector<CheckPointData> stable_checkpoints_;
};

}  // namespace resdb

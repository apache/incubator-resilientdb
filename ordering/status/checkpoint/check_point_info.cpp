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

#include "ordering/status/checkpoint/check_point_info.h"

#include <glog/logging.h>

#include <algorithm>

#include "crypto/signature_verifier.h"

namespace resdb {

CheckPointInfo::CheckPointInfo(const ResDBConfig& config) : config_(config) {
  if (!config.GetCheckPointLoggingPath().empty()) {
    logging_ = std::make_unique<Logging>(config.GetCheckPointLoggingPath());
  }
}

uint64_t CheckPointInfo::GetStableCheckPointSeq() {
  return last_stable_checkpoint_seq_;
}

uint64_t CheckPointInfo::GetMaxCheckPointRequestSeq() { return last_seq_; }

void CheckPointInfo::AddCommitData(const Request& request) {
  if (!config_.IsCheckPointEnabled()) {
    return;
  }
  std::lock_guard<std::mutex> lk(mutex_);
  if (request.seq() != last_seq_ + 1) {
    LOG(ERROR) << "checkpoint data invalid:" << request.seq()
               << " last seq:" << last_seq_;
  }
  if (logging_ != nullptr) {
    logging_->Log(request);
  }
  LOG(ERROR) << "add checkpoint data:" << request.seq();
  last_seq_++;
  CalculateHash(request);
}

// TODO as thread worker.
void CheckPointInfo::CalculateHash(const Request& request) {
  std::string request_hash = request.hash();
  if (request_hash.empty()) {
    request_hash = SignatureVerifier::CalculateHash(request.data());
  }
  HashInfo hash_info;
  hash_info.set_last_hash(last_hash_);
  hash_info.set_current_hash(request_hash);
  hash_info.set_last_block_hash(last_block_hash_);
  std::string hash_data;
  hash_info.SerializeToString(&hash_data);
  last_hash_ = hash_data;
  if (last_seq_ == current_block_seq_ + 2) {
    current_block_seq_ = last_seq_;
    last_block_hash_ = hash_data;
  }
  LOG(ERROR) << "last seq:" << last_seq_
             << " last block:" << current_block_seq_;
}

CheckPointData CheckPointInfo::GetCheckPointData() {
  std::lock_guard<std::mutex> lk(mutex_);
  CheckPointData data;
  data.set_seq(current_block_seq_);
  data.set_hash(last_block_hash_);
  return data;
}

void CheckPointInfo::UpdateStableCheckPoint(
    const std::vector<CheckPointData>& datas) {
  if (datas.empty()) {
    return;
  }

  std::lock_guard<std::mutex> lk(mutex_);
  LOG(ERROR) << "update stable checkpoint:" << datas[0].seq()
             << " last:" << last_stable_checkpoint_seq_;
  if (datas[0].seq() <= last_stable_checkpoint_seq_) {
    return;
  }
  last_stable_checkpoint_seq_ = datas[0].seq();
  stable_checkpoints_ = datas;
}

}  // namespace resdb

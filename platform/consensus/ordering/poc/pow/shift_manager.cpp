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

#include "platform/consensus/ordering/poc/pow/shift_manager.h"

#include <glog/logging.h>

namespace resdb {

ShiftManager::ShiftManager(const ResDBPoCConfig& config) : config_(config) {}

void ShiftManager::AddSliceInfo(const SliceInfo& slice_info) {
  std::unique_lock<std::mutex> lk(mutex_);
  cv_.notify_one();
  LOG(INFO) << "add shift info:" << slice_info.DebugString();
  data_[std::make_pair(slice_info.height(), slice_info.shift_idx())].insert(
      slice_info.sender());
}

bool ShiftManager::Check(const SliceInfo& slice_info, int timeout_ms) {
  auto check_done = [&]() {
    size_t n = config_.GetReplicaNum();
    size_t f = config_.GetMaxMaliciousReplicaNum();
    return data_[std::make_pair(slice_info.height(), slice_info.shift_idx())]
               .size() >= n - f;
  };
  std::unique_lock<std::mutex> lk(mutex_);
  cv_.wait_for(lk, std::chrono::microseconds(timeout_ms), [&] {
    size_t n = config_.GetReplicaNum();
    size_t f = config_.GetMaxMaliciousReplicaNum();
    return check_done();
  });
  LOG(INFO)
      << "get shift msg:" << slice_info.height() << " "
      << slice_info.shift_idx() << ":"
      << data_[std::make_pair(slice_info.height(), slice_info.shift_idx())]
             .size();
  return check_done();
}

}  // namespace resdb

#include "platform/consensus/ordering/poc/pow/shift_manager.h"

#include <glog/logging.h>

namespace resdb {

ShiftManager::ShiftManager(const ResDBPoCConfig& config) : config_(config) {}

void ShiftManager::AddSliceInfo(const SliceInfo& slice_info) {
  std::unique_lock<std::mutex> lk(mutex_);
  cv_.notify_one();
  LOG(ERROR) << "add shift info:" << slice_info.DebugString();
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
  cv_.wait_for(lk, std::chrono::microseconds(timeout_ms),
               [&] { return check_done(); });
  LOG(ERROR)
      << "get shift msg:" << slice_info.height() << " "
      << slice_info.shift_idx() << ":"
      << data_[std::make_pair(slice_info.height(), slice_info.shift_idx())]
             .size();
  return check_done();
}

}  // namespace resdb

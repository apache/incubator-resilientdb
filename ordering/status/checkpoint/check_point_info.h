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

#pragma once

#include <map>

#include "config/resdb_config.h"
#include "ordering/pbft/transaction_collector.h"
#include "ordering/pbft/transaction_utils.h"
#include "ordering/status/checkpoint/check_point_info.h"
#include "proto/checkpoint_info.pb.h"

namespace resdb {

class CheckPointCollector {
 public:
  CheckPointCollector(const ResDBConfig& config,
                      CheckPointInfo* checkpoint_info);

  // Add checkpoint messages to calculate the stable checkpoint.
  CollectorResultCode AddCheckPointMsg(const SignatureInfo& signature,
                                       std::unique_ptr<Request> request);

 private:
  bool MayConsensusChangeStatus(int type, int received_count,
                                std::atomic<TransactionStatue>* status);
  void MayNewCollector(
      std::map<uint64_t, std::unique_ptr<TransactionCollector>>* collector,
      uint64_t seq, std::shared_mutex* collector_mutex);

 private:
  ResDBConfig config_;
  std::map<uint64_t, std::unique_ptr<TransactionCollector>> checkpoint_message_;
  CheckPointInfo* checkpoint_info_;
  mutable std::shared_mutex checkpoint_mutex_;
};

}  // namespace resdb

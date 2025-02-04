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

#include <stdint.h>

#include <map>
#include <memory>
#include <queue>
#include <set>

#include "platform/common/queue/lock_free_queue.h"
#include "platform/config/resdb_config.h"
#include "platform/consensus/ordering/common/data_collector.h"
#include "platform/consensus/ordering/poe_mac/poe_lock_free_collector_pool.h"
#include "platform/networkstrate/server_comm.h"
#include "platform/proto/resdb.pb.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace poe {

class MessageManager {
 public:
  MessageManager(const ResDBConfig& config,
                 std::unique_ptr<TransactionManager> data_impl,
                 SystemInfo* system_info);
  ~MessageManager();

  absl::StatusOr<uint64_t> AssignNextSeq();

  uint64_t GetCurrentView() const;

  DataCollector::CollectorResultCode AddConsensusMsg(
      std::unique_ptr<Request> request);

  std::unique_ptr<BatchUserResponse> GetResponseMsg();

 private:
  bool MayConsensusChangeStatus(
      int type, int received_count,
      std::atomic<DataCollector::TransactionStatue>* status);

  bool IsValidMsg(const Request& request);

 private:
  ResDBConfig config_;
  uint64_t next_seq_ = 1;

  SystemInfo* system_info_;
  LockFreeQueue<BatchUserResponse> queue_;
  std::unique_ptr<TransactionExecutor> transaction_executor_;
  std::unique_ptr<POELockFreeCollectorPool> collector_pool_;
  std::mutex seq_mutex_;

  Stats* global_stats_;
};

}  // namespace poe
}  // namespace resdb

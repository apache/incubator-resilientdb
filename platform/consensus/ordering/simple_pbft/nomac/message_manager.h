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
#include "platform/consensus/execution/system_info.h"
#include "platform/consensus/execution/transaction_executor.h"
#include "platform/consensus/ordering/poe/nomac/qc_collector_pool.h"
#include "platform/consensus/ordering/poe/proto/poe.pb.h"

namespace resdb {
namespace poe {

class MessageManager {
 public:
  MessageManager(const ResDBConfig& config,
                 std::unique_ptr<TransactionManager> data_impl,
                 SystemInfo* system_info);
  ~MessageManager();
  int Commit(std::unique_ptr<POERequest> poe_request);
  void SavePreparedData(std::unique_ptr<POERequest> request);
  std::unique_ptr<BatchUserResponse> GetResponseMsg();
  bool IsCommitted(uint64_t seq);
  void SetExexecutedCallBack(std::function<void()> callback);
  CertifyRequests GetCertifyRequests();
  void CommitInternal(const POERequest& poe_request, const std::string& data);

  void RollBack(uint64_t seq);

  absl::StatusOr<uint64_t> AssignNextSeq();

  void LockView(uint64_t view);
  void UnLockView();
  bool IsLockCurrentView();
  uint64_t GetCurrentView();

  bool MayConsensusChangeStatus(int type, int received_count,
                                std::atomic<TransactionStatue>* status);
  CollectorResultCode AddConsensusMsg(std::unique_ptr<Request> request);
  QC GetQC(uint64_t seq);
  void Clear(uint64_t seq);

 private:
  std::string FetchData(const std::string& hash);
  void SetRequest(std::unique_ptr<POERequest> request);
  void Notify();
  void WaitOrStop();
  void Monitor();
  void TriggerViewChange();

 private:
  ResDBConfig config_;
  uint64_t next_seq_ = 1;
  SystemInfo* system_info_;
  LockFreeQueue<BatchUserResponse> queue_;
  std::map<std::string, std::string> data_;
  std::map<uint64_t, std::unique_ptr<POERequest>> commit_req_;
  std::set<uint64_t> committed_seq_;
  std::mutex mutex_;
  std::unique_ptr<TransactionExecutor> transaction_executor_;
  std::function<void()> viewchange_callback_;
  std::atomic<uint64_t> view_locked_;
  std::mutex seq_mutex_;
  std::unique_ptr<QCCollectorPool> collector_pool_;
};

}  // namespace poe
}  // namespace resdb

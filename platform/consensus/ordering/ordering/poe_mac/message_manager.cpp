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

#include "platform/consensus/ordering/poe_mac/message_manager.h"

#include <glog/logging.h>

#include "platform/consensus/ordering/poe_mac/proto/poe.pb.h"

namespace resdb {
namespace poe {

MessageManager::MessageManager(
    const ResDBConfig& config,
    std::unique_ptr<TransactionManager> executor_impl, SystemInfo* system_info)
    : config_(config),
      system_info_(system_info),
      queue_("executed"),
      transaction_executor_(std::make_unique<TransactionExecutor>(
          config,
          [&](std::unique_ptr<Request> request,
              std::unique_ptr<BatchUserResponse> resp_msg) {
            resp_msg->set_proxy_id(request->proxy_id());
            resp_msg->set_seq(request->seq());
            queue_.Push(std::move(resp_msg));
            collector_pool_->Update(request->seq());
          },
          system_info_, std::move(executor_impl))),
      collector_pool_(std::make_unique<POELockFreeCollectorPool>(
          "txn", config_.GetMaxProcessTxn(), transaction_executor_.get())) {
  global_stats_ = Stats::GetGlobalStats();
}

MessageManager::~MessageManager() {
  if (transaction_executor_) {
    transaction_executor_->Stop();
  }
}

uint64_t MessageManager ::GetCurrentView() const {
  return system_info_->GetCurrentView();
}

absl::StatusOr<uint64_t> MessageManager::AssignNextSeq() {
  std::unique_lock<std::mutex> lk(seq_mutex_);
  uint32_t max_executed_seq = transaction_executor_->GetMaxPendingExecutedSeq();
  if (next_seq_ - max_executed_seq >
      static_cast<uint64_t>(config_.GetMaxProcessTxn())) {
    return absl::InvalidArgumentError("Seq has been used up.");
  }
  return next_seq_++;
}

std::unique_ptr<BatchUserResponse> MessageManager::GetResponseMsg() {
  return queue_.Pop();
}

bool MessageManager::MayConsensusChangeStatus(
    int type, int received_count,
    std::atomic<DataCollector::TransactionStatue>* status) {
  switch (type) {
    case POERequest::TYPE_SUPPORT:
      if (*status == DataCollector::TransactionStatue::None) {
        DataCollector::TransactionStatue old_status =
            DataCollector::TransactionStatue::None;
        return status->compare_exchange_strong(
            old_status, DataCollector::TransactionStatue::READY_COMMIT,
            std::memory_order_acq_rel, std::memory_order_acq_rel);
      }
      break;
    case POERequest::TYPE_CERTIFY:
      if (*status == DataCollector::TransactionStatue::READY_COMMIT &&
          config_.GetMinDataReceiveNum() <= received_count) {
        DataCollector::TransactionStatue old_status =
            DataCollector::TransactionStatue::READY_COMMIT;
        return status->compare_exchange_strong(
            old_status, DataCollector::TransactionStatue::READY_EXECUTE,
            std::memory_order_acq_rel, std::memory_order_acq_rel);
      }
      break;
  }
  return false;
}

bool MessageManager::IsValidMsg(const Request& request) {
  // view should be the same as the current one.
  if (static_cast<uint64_t>(request.current_view()) != GetCurrentView()) {
    LOG(ERROR) << "message view :[" << request.current_view()
               << "] is older than the cur view :[" << GetCurrentView() << "]";
    return false;
  }

  if (static_cast<uint64_t>(request.seq()) <
      transaction_executor_->GetMaxPendingExecutedSeq()) {
    return false;
  }

  return true;
}

DataCollector::CollectorResultCode MessageManager::AddConsensusMsg(
    std::unique_ptr<Request> request) {
  if (request == nullptr || !IsValidMsg(*request)) {
    return DataCollector::CollectorResultCode::INVALID;
  }
  int type = request->type();
  uint64_t seq = request->seq();
  int resp_received_count = 0;
  int ret = collector_pool_->GetCollector(seq)->AddRequest(
      std::move(request), type == POERequest::TYPE_SUPPORT,
      [&](const Request& request, int received_count,
          std::atomic<DataCollector::TransactionStatue>* status) {
        if (MayConsensusChangeStatus(type, received_count, status)) {
          resp_received_count = 1;
        }
      });
  if (ret != 0) {
    return DataCollector::CollectorResultCode::INVALID;
  }
  if (resp_received_count > 0) {
    return DataCollector::CollectorResultCode::STATE_CHANGED;
  }
  return DataCollector::CollectorResultCode::OK;
}

}  // namespace poe
}  // namespace resdb

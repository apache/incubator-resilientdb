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

#include "platform/consensus/ordering/poe/nomac/message_manager.h"

#include <glog/logging.h>

#include "platform/consensus/ordering/poe/common/poe_utils.h"

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
            // collector_pool_->Update(request->seq());
            resp_msg->set_proxy_id(request->proxy_id());
            resp_msg->set_seq(request->seq());
            queue_.Push(std::move(resp_msg));
            // new committed req, reset viewchange timer.
            if (viewchange_callback_) {
              viewchange_callback_();
            }
          },
          system_info_, std::move(executor_impl))),
      view_locked_(0),
      collector_pool_(std::make_unique<QCCollectorPool>(
          "txn", config_.GetMaxProcessTxn() * 4)) {}

MessageManager::~MessageManager() {}
absl::StatusOr<uint64_t> MessageManager::AssignNextSeq() {
  std::unique_lock<std::mutex> lk(seq_mutex_);
  uint32_t max_executed_seq = transaction_executor_->GetMaxPendingExecutedSeq();
  if (next_seq_ - max_executed_seq >
      static_cast<uint64_t>(config_.GetMaxProcessTxn())) {
    return absl::InvalidArgumentError("Seq has been used up.");
  }
  return next_seq_++;
}

void MessageManager::LockView(uint64_t view) {
  LOG(ERROR) << "lock view:" << view;
  if (view_locked_ < view) {
    view_locked_ = view;
  }
}

void MessageManager::UnLockView() { view_locked_ = 0; }

bool MessageManager::IsLockCurrentView() {
  return system_info_->GetCurrentView() <= view_locked_;
}

uint64_t MessageManager::GetCurrentView() {
  return system_info_->GetCurrentView();
}

void MessageManager::SetExexecutedCallBack(std::function<void()> callback) {
  viewchange_callback_ = callback;
}

int MessageManager::Commit(std::unique_ptr<POERequest> poe_request) {
  SetRequest(std::move(poe_request));
  return 0;
}

void MessageManager::CommitInternal(const POERequest& poe_request,
                                    const std::string& data) {
  if (system_info_->GetCurrentView() != poe_request.current_view()) {
    LOG(ERROR) << " VIEW NOT MATCH, SKIP COMMIT";
    return;
  }
  if (poe_request.current_view() <= view_locked_) {
    LOG(ERROR) << " view:" << poe_request.current_view()
               << " is locked, rejected:" << view_locked_;
    return;
  }
  std::unique_ptr<Request> execute_request = std::make_unique<Request>();
  execute_request->set_data(data);
  execute_request->set_seq(poe_request.seq());
  execute_request->set_proxy_id(poe_request.proxy_id());
  // LOG(ERROR) << "commit seq:" << poe_request.seq()
  //           << " data size:" << execute_request->data().size();
  committed_seq_.insert(poe_request.seq());
  transaction_executor_->Commit(std::move(execute_request));
}

void MessageManager::SavePreparedData(std::unique_ptr<POERequest> request) {
  SetRequest(std::move(request));
}

void MessageManager::SetRequest(std::unique_ptr<POERequest> poe_request) {
  std::string hash = poe_request->hash();
  int64_t seq = poe_request->seq();

  std::lock_guard<std::mutex> lk(mutex_);
  if (poe_request->type() == POERequest::TYPE_CERTIFY) {
    commit_req_[seq] = std::move(poe_request);
  } else if (poe_request->type() == POERequest::TYPE_PROPOSE) {
    data_[hash] = poe_request->data();
  }
  if (commit_req_.find(seq) != commit_req_.end() &&
      data_.find(hash) != data_.end()) {
    CommitInternal(*commit_req_[seq], data_[hash]);
  }
}

bool MessageManager::IsCommitted(uint64_t seq) {
  std::lock_guard<std::mutex> lk(mutex_);
  return committed_seq_.find(seq) != committed_seq_.end();
}

std::unique_ptr<BatchUserResponse> MessageManager::GetResponseMsg() {
  return queue_.Pop();
}

CertifyRequests MessageManager::GetCertifyRequests() {
  std::lock_guard<std::mutex> lk(mutex_);
  CertifyRequests msg;
  for (uint64_t seq = 1;; seq++) {
    if (commit_req_.find(seq) == commit_req_.end()) {
      break;
    }
    POERequest request = *commit_req_[seq];
    if (data_.find(request.hash()) == data_.end()) {
      break;
    }
    request.set_data(data_[request.hash()]);
    *msg.add_certify_requests() = request;
  }
  LOG(ERROR) << "get data seq:" << msg.certify_requests_size();
  return msg;
}

void MessageManager::RollBack(uint64_t seq) {
  LOG(ERROR) << "rool back seq:" << seq;
}

bool MessageManager::MayConsensusChangeStatus(
    int type, int received_count, std::atomic<TransactionStatue>* status) {
  switch (type) {
    case POERequest::TYPE_SUPPORT:
      if (*status == TransactionStatue::None &&
          config_.GetMinDataReceiveNum() <= received_count) {
        TransactionStatue old_status = TransactionStatue::None;
        return status->compare_exchange_strong(
            old_status, TransactionStatue::EXECUTED, std::memory_order_acq_rel,
            std::memory_order_acq_rel);
      }
      break;
  }
  return false;
}

CollectorResultCode MessageManager::AddConsensusMsg(
    std::unique_ptr<Request> request) {
  if (request == nullptr) {
    return CollectorResultCode::INVALID;
  }
  int type = request->type();
  uint64_t seq = request->seq();
  int resp_received_count = 0;
  int ret = collector_pool_->GetCollector(seq)->AddRequest(
      std::move(request), false,
      [&](const Request& request, int received_count,
          std::atomic<TransactionStatue>* status) {
        if (MayConsensusChangeStatus(type, received_count, status)) {
          resp_received_count = 1;
        }
      });
  if (ret != 0) {
    return CollectorResultCode::INVALID;
  }
  if (resp_received_count > 0) {
    return CollectorResultCode::STATE_CHANGED;
  }
  return CollectorResultCode::OK;
}

void MessageManager::Clear(uint64_t seq) {
  //	LOG(ERROR)<<"update seq:"<<seq;
  collector_pool_->Update(seq);
}

QC MessageManager::GetQC(uint64_t seq) {
  return collector_pool_->GetCollector(seq)->GetQC(seq);
}

}  // namespace poe
}  // namespace resdb

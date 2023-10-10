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

#include "platform/consensus/ordering/hotstuff/message_manager.h"

#include <glog/logging.h>

namespace resdb {
namespace hotstuff {

MessageManager::MessageManager(
    const ResDBConfig& config,
    std::unique_ptr<TransactionManager> executor_impl, SystemInfo* system_info)
    : MessageManagerBasic(config, std::move(executor_impl), system_info) {}

int MessageManager::UpdateNode(const HotStuffRequest& request) {
  if (request.type() == HotStuffRequest::TYPE_COMMIT) {
    SetLockQC(request.qc());
  } else if (request.type() == HotStuffRequest::TYPE_PRECOMMIT) {
    SetPrepareQC(request.qc());
  }
  return 0;
}

void MessageManager::SetPrepareQC(const QC& qc) {
  std::unique_lock<std::mutex> lk(mutex_);
  prepare_qc_ = qc;
}

QC MessageManager::GetPrepareQC() {
  std::unique_lock<std::mutex> lk(mutex_);
  return prepare_qc_;
}

void MessageManager::SetLockQC(const QC& qc) {
  std::unique_lock<std::mutex> lk(mutex_);
  lock_qc_ = qc;
}

bool MessageManager::IsSaveNode(const HotStuffRequest& request) {
  std::unique_lock<std::mutex> lk(mutex_);
  return request.node().pre().hash() == lock_qc_.node().info().hash() ||
         request.qc().node().info().view() > lock_qc_.node().info().view();
}

int MessageManager::Commit(std::unique_ptr<HotStuffRequest> hotstuff_request) {
  std::unique_ptr<Request> execute_request = std::make_unique<Request>();
  execute_request->set_data(hotstuff_request->qc().node().data());
  execute_request->set_seq(hotstuff_request->qc().node().info().view());
  execute_request->set_proxy_id(hotstuff_request->qc().node().proxy_id());
  transaction_executor_->Commit(std::move(execute_request));
  return 0;
}

}  // namespace hotstuff
}  // namespace resdb

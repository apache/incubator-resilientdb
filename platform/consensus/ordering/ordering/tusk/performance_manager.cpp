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

#include "platform/consensus/ordering/tusk/performance_manager.h"

#include <glog/logging.h>

#include "common/utils/utils.h"
#include "platform/consensus/ordering/tusk/proto/tusk.pb.h"

namespace resdb {
namespace tusk {

int PerformanceManager::GetPrimary() {
  return (primary_id_ % config_.GetReplicaInfos().size()) + 1;
}

// =================== request ========================

void PerformanceManager::ConverToRequest(const BatchUserRequest& batch_request,
                                         Request* new_request) {
  TuskRequest tusk_request;
  batch_request.SerializeToString(tusk_request.mutable_data());

  if (verifier_) {
    auto signature_or = verifier_->SignMessage(tusk_request.data());
    if (!signature_or.ok()) {
      LOG(ERROR) << "Sign message fail";
      return;
    }
    *tusk_request.mutable_data_signature() = *signature_or;
  }

  tusk_request.set_type(TuskRequest::TYPE_NEWREQUEST);
  tusk_request.set_sender_id(config_.GetSelfInfo().id());
  tusk_request.set_proxy_id(config_.GetSelfInfo().id());

  tusk_request.SerializeToString(new_request->mutable_data());
}

void PerformanceManager::PostSend() { primary_id_++; }

}  // namespace tusk
}  // namespace resdb

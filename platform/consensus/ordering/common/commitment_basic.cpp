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

#include "platform/consensus/ordering/common/commitment_basic.h"

#include <glog/logging.h>
#include <unistd.h>

#include "common/utils/utils.h"

namespace resdb {

CommitmentBasic::CommitmentBasic(const ResDBConfig& config,
                                 MessageManagerBasic* transaction_manager,
                                 ReplicaCommunicator* replica_communicator,
                                 SignatureVerifier* verifier)
    : config_(config),
      transaction_manager_(transaction_manager),
      stop_(false),
      replica_communicator_(replica_communicator),
      verifier_(verifier) {
  id_ = config_.GetSelfInfo().id();
  global_stats_ = Stats::GetGlobalStats();
  executed_thread_ =
      std::thread(&CommitmentBasic::PostProcessExecutedMsg, this);
}

CommitmentBasic::~CommitmentBasic() {
  stop_ = true;
  if (executed_thread_.joinable()) {
    executed_thread_.join();
  }
}

void CommitmentBasic::SetNodeId(int32_t id) { id_ = id; }

int32_t CommitmentBasic::PrimaryId(int64_t view_num) {
  view_num--;
  return (view_num % config_.GetReplicaInfos().size()) + 1;
}

// =========== private threads ===========================
// If the transaction is executed, send back to the proxy.
int CommitmentBasic::PostProcessExecutedMsg() {
  while (!stop_) {
    auto batch_resp = transaction_manager_->GetResponseMsg();
    if (batch_resp == nullptr) {
      continue;
    }
    Request request;
    request.set_type(Request::TYPE_RESPONSE);
    request.set_sender_id(config_.GetSelfInfo().id());
    request.set_proxy_id(batch_resp->proxy_id());
    request.set_seq(batch_resp->seq());
    batch_resp->SerializeToString(request.mutable_data());
    replica_communicator_->SendMessage(request, request.proxy_id());
  }
  return 0;
}

}  // namespace resdb

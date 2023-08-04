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

#include "platform/consensus/ordering/tendermint/message_manager.h"

#include <glog/logging.h>

namespace resdb {
namespace tendermint {

MessageManager::MessageManager(
    const ResDBConfig& config,
    std::unique_ptr<TransactionManager> executor_impl)
    : config_(config),
      system_info_(config),
      queue_("executed"),
      transaction_executor_(std::make_unique<TransactionExecutor>(
          config,
          [&](std::unique_ptr<Request> request,
              std::unique_ptr<BatchUserResponse> resp_msg) {
            resp_msg->set_proxy_id(request->proxy_id());
            queue_.Push(std::move(resp_msg));
          },
          &system_info_, std::move(executor_impl))) {}

int MessageManager::Commit(
    std::unique_ptr<TendermintRequest> tendermint_request) {
  // LOG(ERROR) << "========= commit ===========";
  std::unique_ptr<Request> execute_request = std::make_unique<Request>();
  execute_request->set_data(tendermint_request->data());
  execute_request->set_seq(tendermint_request->height());
  execute_request->set_proxy_id(tendermint_request->proxy_id());
  transaction_executor_->Commit(std::move(execute_request));
  return 0;
}

std::unique_ptr<BatchUserResponse> MessageManager::GetResponseMsg() {
  return queue_.Pop();
}

}  // namespace tendermint
}  // namespace resdb

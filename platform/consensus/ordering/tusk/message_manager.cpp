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

#include "platform/consensus/ordering/tusk/message_manager.h"

#include <glog/logging.h>

#include "platform/consensus/ordering/tusk/tusk_utils.h"

namespace resdb {
namespace tusk {

MessageManager::MessageManager(
    const ResDBConfig& config,
    std::unique_ptr<TransactionManager> executor_impl, SystemInfo* system_info)
    : MessageManagerBasic(config, std::move(executor_impl), system_info) {}

uint64_t MessageManager::GetCurrentView() {
  return system_info_->GetCurrentView();
}

int MessageManager::Commit(std::unique_ptr<TuskRequest> tusk_request) {
  std::unique_ptr<Request> execute_request = std::make_unique<Request>();
  execute_request->set_data(tusk_request->data());
  execute_request->set_seq(tusk_request->seq());
  execute_request->set_proxy_id(tusk_request->proxy_id());
  // LOG(ERROR) << "====== commit seq:" << tusk_request->seq()
  //           << " data size:" << execute_request->data().size()
  // 	<<" round:"<<tusk_request->round()<<"
  // source:"<<tusk_request->source_id();
  committed_.insert(
      std::make_pair(tusk_request->round(), tusk_request->source_id()));
  return transaction_executor_->Commit(std::move(execute_request));
}

bool MessageManager::IsCommitted(const TuskRequest& request) {
  return committed_.find(std::make_pair(
             request.round(), request.source_id())) != committed_.end();
}

}  // namespace tusk
}  // namespace resdb

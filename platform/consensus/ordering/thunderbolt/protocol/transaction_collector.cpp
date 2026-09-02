/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include "platform/consensus/ordering/thunderbolt/algorithm/transaction_collector.h"

#include <glog/logging.h>

namespace resdb {

TransactionStatue TransactionCollector::GetStatus() const { return status_; }

int TransactionCollector::AddRequest(
    std::unique_ptr<google::protobuf::Message> request, int sender_id, int type,
    std::function<void(const google::protobuf::Message&, int received_count,
                       std::atomic<TransactionStatue>* status)>
        call_back) {
  if (status_.load() == EXECUTED) {
    return -2;
  }

  if (request) {
    request_ = std::move(request);
    call_back(*request, 1, &status_);
  } else {
    senders_[type].insert(sender_id);
    if (request_) {
      call_back(*request_, senders_[type].size(), &status_);
    }
  }
  return 0;
}

}  // namespace resdb

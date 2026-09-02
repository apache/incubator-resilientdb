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
#pragma once

#include <google/protobuf/message.h>

#include "platform/statistic/stats.h"

namespace resdb {

enum TransactionStatue {
  None = 0,
  Prepare = -999,
  READY_PREPARE = 1,
  READY_COMMIT = 2,
  READY_EXECUTE = 3,
  EXECUTED = 4,
};

class TransactionCollector {
 public:
  TransactionCollector(int64_t seq)
      : seq_(seq), status_(TransactionStatue::None) {}

  ~TransactionCollector() = default;

  int AddRequest(
      std::unique_ptr<google::protobuf::Message> request, int sender_id,
      int type,
      std::function<void(const google::protobuf::Message&, int received_count,
                         std::atomic<TransactionStatue>* status)>
          call_back);

  TransactionStatue GetStatus() const;
  int64_t GetSeq() { return seq_; }

  std::unique_ptr<google::protobuf::Message> GetData() {
    return std::move(request_);
  }

 private:
  int64_t seq_;
  std::unique_ptr<google::protobuf::Message> request_;
  std::atomic<TransactionStatue> status_ = TransactionStatue::None;
  std::set<int> senders_[10];
};

}  // namespace resdb

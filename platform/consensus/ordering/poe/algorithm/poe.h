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

#include <deque>
#include <map>
#include <queue>
#include <thread>

#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "platform/consensus/ordering/poe/proto/proposal.pb.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace poe {

class PoE : public common::ProtocolBase {
 public:
  PoE(int id, int f, int total_num, SignatureVerifier* verifier);
  ~PoE();

  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  bool ReceivePropose(std::unique_ptr<Transaction> txn);
  bool ReceivePrepare(std::unique_ptr<Proposal> proposal);

 private:
  bool IsStop();

 private:
  std::mutex mutex_;
  std::map<std::string, std::set<int32_t> > received_;
  std::map<std::string, std::unique_ptr<Transaction> > data_;

  int64_t seq_;
  bool is_stop_;
  SignatureVerifier* verifier_;
  Stats* global_stats_;
};

}  // namespace poe
}  // namespace resdb

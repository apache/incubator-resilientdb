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

#include "executor/common/transaction_manager.h"
#include "platform/consensus/ordering/common/framework/consensus.h"
#include "platform/consensus/ordering/poe/algorithm/poe.h"
#include "platform/networkstrate/consensus_manager.h"

namespace resdb {
namespace poe {

class Consensus : public common::Consensus {
 public:
  Consensus(const ResDBConfig& config,
            std::unique_ptr<TransactionManager> transaction_manager);
  virtual ~Consensus() = default;

 private:
  int ProcessCustomConsensus(std::unique_ptr<Request> request) override;
  int ProcessNewTransaction(std::unique_ptr<Request> request) override;
  int CommitMsg(const google::protobuf::Message& msg) override;
  int CommitMsgInternal(const Transaction& txn);

  int Prepare(const Transaction& txn);

 protected:
  std::unique_ptr<PoE> poe_;
  Stats* global_stats_;
  int64_t start_;
  std::mutex mutex_;
  int send_num_[200];
};

}  // namespace poe
}  // namespace resdb
